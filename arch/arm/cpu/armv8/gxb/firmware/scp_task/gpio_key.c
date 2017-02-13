#include "config.h"
#include "registers.h"
#include "task_apis.h"

#include <aml_gpio.h>
#include <asm/arch/gpio.h>

/* gpio offset between kernel and u-boot */
#define GPIO_OFFSET	122

#define NE 0xffffffff
#define PK(reg, bit) ((reg<<5)|bit)

/*AO REG */
#define AO 0x10
#define AO2 0x11

extern void _udelay(unsigned int i);

/* bank index : Group / offset : number */
static unsigned int bank_index;
static unsigned int offset;
static unsigned int active;

static unsigned int gpio_to_pin[][6] = {
	/* GPIO AO [122 ~ 135] -> [0 ~ 13] */
	[GPIOAO_6] = {PK(AO, 18), PK(AO, 16), NE, NE, NE, NE,},
	[GPIOAO_8] = {PK(AO, 30), NE, NE, NE, NE, NE,},
	[GPIOAO_9] = {PK(AO, 29), NE, NE, NE, NE, NE,},
	[GPIOAO_10] = {PK(AO, 28), NE, NE, NE, NE, NE,},
	[GPIOAO_11] = {PK(AO, 27), PK(AO, 15), PK(AO, 14), PK(AO, 17),
		PK(AO2, 0), NE,},
	/* GPIO Y [211 ~ 227] -> [89 ~ 105] */
	[PIN_GPIOY_3] = {PK(2, 16), PK(3, 4), PK(1, 2), NE, NE, NE,},
	[PIN_GPIOY_5] = {PK(2, 16), PK(3, 5), PK(1, 13), NE, NE, NE,}, /* SW1 */
	[PIN_GPIOY_7] = {PK(2, 16), PK(3, 5), PK(1, 4), NE, NE, NE,},
	[PIN_GPIOY_8] = {PK(2, 16), PK(3, 5), PK(1, 5), NE, NE, NE,},
	[PIN_GPIOY_13] = {PK(1, 17), PK(1, 10), NE, NE, NE, NE,},
	[PIN_GPIOY_14] = {PK(1, 16), PK(1, 11), NE, NE, NE, NE,},
	/* GPIO X [228 ~ 250] -> [106 ~ 128] */
	[PIN_GPIOX_0] = {PK(8, 5), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_1] = {PK(8, 4), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_2] = {PK(8, 3), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_3] = {PK(8, 2), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_4] = {PK(8, 1), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_5] = {PK(8, 0), NE, NE, NE, NE, NE,},
	[PIN_GPIOX_6] = {PK(3, 9), PK(3, 17), NE, NE, NE, NE,},
	[PIN_GPIOX_7] = {PK(8, 11), PK(3, 8), PK(3, 18), NE, NE, NE,},
	[PIN_GPIOX_8] = {PK(3, 10), PK(4, 7), PK(3, 30), NE, NE, NE,},
	[PIN_GPIOX_9] = {PK(3, 7), PK(4, 6), PK(3, 29), NE, NE, NE,},
	[PIN_GPIOX_10] = {PK(3, 13), PK(3, 28), NE, NE, NE, NE,},
	[PIN_GPIOX_11] = {PK(3, 12), PK(3, 27), NE, NE, NE, NE,},
	[PIN_GPIOX_12] = {PK(4, 13), PK(4, 17), PK(3, 12), NE, NE, NE,},
	[PIN_GPIOX_13] = {PK(4, 12), PK(4, 16), PK(3, 12), NE, NE, NE,},
	[PIN_GPIOX_19] = {PK(2, 22), PK(3, 15), PK(2, 30), NE, NE, NE,},
	[PIN_GPIOX_21] = {PK(3, 24), NE, NE, NE, NE, NE,},
};

#define BANK(n, f, l, per, peb, pr, pb, dr, db, or, ob, ir, ib)		\
	{								\
		.name	= n,						\
		.first	= f,						\
		.last	= l,						\
		.regs	= {						\
			[REG_PULLEN]	= { (0xc8834120 + (per<<2)), peb },			\
			[REG_PULL]	= { (0xc88344e8 + (pr<<2)), pb },			\
			[REG_DIR]	= { (0xc8834430 + (dr<<2)), db },			\
			[REG_OUT]	= { (0xc8834430 + (or<<2)), ob },			\
			[REG_IN]	= { (0xc8834430 + (ir<<2)), ib },			\
		},							\
	 }
#define AOBANK(n, f, l, per, peb, pr, pb, dr, db, or, ob, ir, ib)		\
	{								\
		.name	= n,						\
		.first	= f,						\
		.last	= l,						\
		.regs	= {						\
			[REG_PULLEN]	= { (0xc810002c + (per<<2)), peb },			\
			[REG_PULL]	= { (0xc810002c + (pr<<2)), pb },			\
			[REG_DIR]	= { (0xc8100024 + (dr<<2)), db },			\
			[REG_OUT]	= { (0xc8100024 + (or<<2)), ob },			\
			[REG_IN]	= { (0xc8100024 + (ir<<2)), ib },			\
		},							\
	 }

static struct meson_bank mesongxbb_banks[] = {
	/*   name    first         last
	 *   pullen  pull       in  */
	AOBANK("GPIOAO_",   GPIOAO_0, GPIOAO_13,
	     0,  0,  0, 16,  0,  0,  0, 16,  1,  0),
	BANK("GPIOY_",    PIN_GPIOY_0,  PIN_GPIOY_16,
	     1,  0,  1,  0,  3,  0,  4,  0,  5,  0),
	BANK("GPIOX_",    PIN_GPIOX_0,  PIN_GPIOX_22,
	     4,  0,  4,  0,  12,  0,  13,  0,  14,  0),
};

static unsigned long domain[]={
	[0] = 0xc88344b0,
	[1] = 0xc8100014,
};

static unsigned long active_tbl[] = {
	[0] = 0x0,
	[1] = 0x0001C000,
	[2] = 0x00100040,
};

int  clear_pinmux(unsigned int pin)
{
	int i;
	int dom, reg, bit;

	for (i = 0; i < 6; i++) {
		if (gpio_to_pin[pin][i] != NE) {
			reg = (GPIO_REG(gpio_to_pin[pin][i]) & 0xf);
			bit = GPIO_BIT(gpio_to_pin[pin][i]);
			dom = (GPIO_REG(gpio_to_pin[pin][i]) >> 4);
			aml_update_bits((domain[dom] + (reg << 2)), BIT(bit), 0);
		}
	}
	return 1;
}

/* pin mux clear */
int init_gpio_key(void)
{
	unsigned int key_index;
	unsigned int reg, bit;
	struct meson_bank bank;

	/* translate key index */
	key_index = gpio_wakeup_keyno - GPIO_OFFSET;

	/* clear_pinmux */
	if (!clear_pinmux(key_index))
		return 0;

	/* bank index */
	if (key_index < 14) {
		/* GPIOAO */
		bank_index = 0;
		offset = key_index;
	} else if ((key_index >= 14) && (key_index < 106)) {
		/* GPIOY */
		bank_index = 1;
		offset = key_index - 89;
	} else if ((key_index >= 106) && (key_index < 129)) {
		/* GPIOX */
		bank_index = 2;
		offset = key_index - 106;
	} else {
		uart_puts("key_index fail\n");
		return 0;
	}

	/* Active Level - 1 : high, 0 : low */
	active = (active_tbl[bank_index] >> offset) & 0x1;

	bank  = mesongxbb_banks[bank_index];

	/* set as input port */
	reg = bank.regs[REG_DIR].reg;
	bit = bank.regs[REG_DIR].bit + offset;
	aml_update_bits(reg, BIT(bit), BIT(bit));

	return 1;
}

int key_chattering(void)
{
	unsigned int count = 0;
	unsigned int key_pressed = 1;

	unsigned int reg, bit;
	unsigned int active_value;
	struct meson_bank bank = mesongxbb_banks[bank_index];

	reg = bank.regs[REG_IN].reg;
	bit = bank.regs[REG_IN].bit + offset;

	if (active)
		active_value = (1 << bit);
	else
		active_value = 0;

	while (key_pressed) {
		_udelay(1000);

		if ((readl(reg) & (1 << bit)) == active_value)
			count++;
		else
			key_pressed = 0;

		if (count == 500)
			return 1;
	}

	return 0;
}

int gpio_detect_key(void)
{
	unsigned int reg, bit;
	unsigned int active_value;
	struct meson_bank bank = mesongxbb_banks[bank_index];

	reg = bank.regs[REG_IN].reg;
	bit = bank.regs[REG_IN].bit + offset;

	if (active)
		active_value = (1 << bit);
	else
		active_value = 0;

	if ((readl(reg) & (1 << bit)) == active_value)
		if (key_chattering())
			return 1;

	return 0;
}
