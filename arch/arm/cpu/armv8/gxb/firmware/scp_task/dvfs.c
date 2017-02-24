#include "config.h"
#include "registers.h"
#include "task_apis.h"

#include "dvfs.h"
#include <dvfs_board.c>

void get_dvfs_info(unsigned int domain,
		unsigned char *info_out, unsigned int *size_out)
{
	unsigned int cnt;

	if (domain & 0x80)
		cnt = ARRAY_SIZE(cpu_dvfs_tbl);
	else
		cnt = ARRAY_SIZE(cpu_dvfs_tbl_limited);

	buf_opp.latency = 200;
	buf_opp.count = cnt;
	memset(&buf_opp.opp[0], 0,
	       MAX_DVFS_OPPS * sizeof(struct scpi_opp_entry));

	if (domain & 0x80)
		memcpy(&buf_opp.opp[0], cpu_dvfs_tbl ,
			cnt * sizeof(struct scpi_opp_entry));
	else
		memcpy(&buf_opp.opp[0], cpu_dvfs_tbl_limited,
			cnt * sizeof(struct scpi_opp_entry));

	memcpy(info_out, &buf_opp, sizeof(struct scpi_opp));
	*size_out = sizeof(struct scpi_opp);
	return;
}
