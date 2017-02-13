#ifndef __GPIO_KEY_H__
#define __GPIO_KEY_H__

#ifdef CONFIG_GPIO_WAKEUP
int gpio_detect_key(void);
int init_gpio_key(void);

unsigned int gpio_wakeup_keyno;
#endif
#endif

