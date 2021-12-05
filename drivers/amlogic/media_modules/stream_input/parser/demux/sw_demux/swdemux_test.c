/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/


#include "swdemux.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


struct thread_args {
	char *fname;
	struct timeval start_tv;
};
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timeval start_tv;

static void prdump(const char* m, const void* data, uint32_t len) {
	if (m)
		printf("%s:\n", m);
	if (data) {
		size_t i = 0;
		const unsigned char *c __attribute__((unused)) = data;
		while (len >= 16) {
			printf("%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3], c[i+4], c[i+5], c[i+6], c[i+7],
					c[i+8], c[i+9], c[i+10], c[i+11], c[i+12], c[i+13], c[i+14], c[i+15]);
			len -= 16;
			i += 16;
		}
		while (len >= 8) {
			printf("%02x %02x %02x %02x %02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3], c[i+4], c[i+5], c[i+6], c[i+7]);
			len -= 8;
			i += 8;
		}
		while (len >= 4) {
			printf("%02x %02x %02x %02x\n",
					c[i], c[i+1], c[i+2], c[i+3]);
			len -= 4;
			i += 4;
		}
		while (len >= 1) {
			printf("%02x ", c[i]);
			len -= 1;
			i += 1;
		}
	}
}
static void
ts_pkt_cb (SWDMX_TsPacket *pkt, SWDMX_Ptr data)
{
	return;
	uint8_t *buf = pkt->packet;
	//prdump("ts_data", pkt->packet, 32);
	if (pkt->packet_len != 188) {
		printf("ts data len wrong:%d\n", pkt->packet_len);
	} else {
		int i;
		for (i = 5; i < 188; i++) {
			if (buf[i] != buf[i-1] + 1) {
				printf("ts data error i:%d, buf:%#x, %#x\n", i, buf[i], buf[i-1]);
				break;
			}
		}
	}
}

static void
sec_cb (SWDMX_UInt8 *sec, SWDMX_Int len, SWDMX_Ptr data)
{
	SWDMX_Int i;

	printf("section:\n");

	for (i = 0; i < len; i ++) {
		printf("%02x%s", sec[i], ((i + 1) & 0xf) ? " " : "\n");
	}

	if (len & 0xf)
		printf("\n");
}

void thread_func(void *args)
{
	FILE *fp = NULL;
	uint8_t buf[64*1024];
	int ret;
	int total = 0, left = 0, n = 0;
	struct timeval tv;
	struct timeval tv_limits;
	struct thread_args targs;
	char *file_name;
	int diff_time;
	int sleep_cnt = 0;
	unsigned int total_ts_cnt = 0;
	unsigned int ts_loop = 0;
	SWDMX_TsFilter       *tsf;
	SWDMX_SecFilter      *secf;
	SWDMX_DescChannel    *dch;
	SWDMX_DescAlgo       *algo;
	SWDMX_TsFilterParams  tsfp;
	SWDMX_SecFilterParams secfp;
	SWDMX_TsParser    *tsp;
	SWDMX_Descrambler *desc;
	SWDMX_Demux       *dmx;
	uint8_t odd_key[16] = {0x79, 0x4B, 0x21, 0x80, 0x13, 0x71, 0x00, 0xFA, 0x1E, 0xBD, 0xCB, 0x13, 0xB0, 0x63, 0xE5, 0x28};
	uint8_t even_key[16] = {0x32, 0x6C, 0xD4, 0xE9, 0xE0, 0xD6, 0x74, 0x81, 0x1A, 0x00, 0xF0, 0xCE, 0x1B, 0x50, 0xBC, 0xD8};
	uint8_t iv[16] = {0x49, 0x72, 0x64, 0x65, 0x74, 0x6F, 0xA9, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74};

	memcpy(&targs, args, sizeof(targs));
	file_name = targs.fname;
	fp = fopen(file_name, "rb");
	if (fp == NULL) {
		printf("thread open %s failed\n", file_name);
		return;
	}
	printf("Begin to process %s, tid:%lu\n", file_name, pthread_self());

	tsp  = swdmx_ts_parser_new();
	desc = swdmx_descrambler_new();
	dmx  = swdmx_demux_new();

	swdmx_ts_parser_add_ts_packet_cb(tsp,
				swdmx_descrambler_ts_packet_cb,
				desc);
	swdmx_descrambler_add_ts_packet_cb(desc,
				swdmx_demux_ts_packet_cb,
				dmx);

	dch = swdmx_descrambler_alloc_channel(desc);
	algo = swdmx_aes_cbc_algo_new();

	swdmx_desc_channel_set_algo(dch, algo);
	swdmx_desc_channel_set_pid(dch, 0x80);
	swdmx_desc_channel_set_param(dch, SWDMX_AES_CBC_PARAM_ALIGN, SWDMX_DESC_ALIGN_HEAD);
	swdmx_desc_channel_set_param(dch, SWDMX_AES_CBC_PARAM_ODD_KEY, odd_key);
	swdmx_desc_channel_set_param(dch, SWDMX_AES_CBC_PARAM_EVEN_KEY, even_key);
	swdmx_desc_channel_set_param(dch, SWDMX_AES_CBC_PARAM_ODD_IV, iv);
	swdmx_desc_channel_set_param(dch, SWDMX_AES_CBC_PARAM_EVEN_IV, iv);

	swdmx_desc_channel_enable(dch);
	tsf = swdmx_demux_alloc_ts_filter(dmx);

	tsfp.pid = 0x80;

	swdmx_ts_filter_set_params(tsf, &tsfp);
	swdmx_ts_filter_enable(tsf);
	swdmx_ts_filter_add_ts_packet_cb(tsf, ts_pkt_cb, NULL);

	secf = swdmx_demux_alloc_sec_filter(dmx);

#if 1
	memset(&secfp, 0, sizeof(secfp));
	secfp.pid   = 0x0;
	secfp.crc32 = SWDMX_TRUE;
	secfp.value[0] = 0x0;
	secfp.mask[0]  = 0xff;
	swdmx_sec_filter_set_params(secf, &secfp);
	swdmx_sec_filter_enable(secf);
	swdmx_sec_filter_add_section_cb(secf, sec_cb, NULL);
#endif
	while (1) {
		n = fread(buf + left, 1, sizeof(buf) - left, fp);
		if (n == 0)
		{
			ts_loop++;
			if (ts_loop > 1)
				break;

			fseek(fp, 0L, SEEK_SET);
		}
		total_ts_cnt += n;

		left += n;

		n = swdmx_ts_parser_run(tsp, buf, left);

		left -= n;
		if (left)
			memmove(buf, buf + n, left);

		gettimeofday(&tv_limits, NULL);
		if (tv_limits.tv_usec < targs.start_tv.tv_usec) {
			diff_time = (tv_limits.tv_sec - 1 - targs.start_tv.tv_sec) * 1000 +
				(tv_limits.tv_usec + 1*1000*1000 - targs.start_tv.tv_usec) / 1000;
		} else {
			diff_time = (tv_limits.tv_sec - targs.start_tv.tv_sec) * 1000 +
				(tv_limits.tv_usec - targs.start_tv.tv_usec) / 1000;
		}
		if ((total_ts_cnt/1024/1024*1000*8)/diff_time >= 20)
		{
				usleep(20*1000);
		}
	}

	gettimeofday(&tv, NULL);
	if (tv.tv_usec < targs.start_tv.tv_usec) {
		diff_time = (tv.tv_sec - 1 - targs.start_tv.tv_sec) * 1000 +
			(tv.tv_usec + 1*1000*1000 - targs.start_tv.tv_usec) / 1000;
	} else {
		diff_time = (tv.tv_sec - targs.start_tv.tv_sec) * 1000 +
			(tv.tv_usec - targs.start_tv.tv_usec) / 1000;
	}

	swdmx_ts_parser_free(tsp);
	swdmx_descrambler_free(desc);
	swdmx_demux_free(dmx);
	fclose(fp);
	printf("----->> thread %s end, ts_pkt_num:%d, time:%d ms, Mbps:%d \n",
			file_name, total_ts_cnt, diff_time, (total_ts_cnt/1024*1000/1024*8)/diff_time);

	total_ts_cnt = 0;
}
int main (int argc, char **argv)
{
	char  buf[64*1024];
	int   n, left = 0, i, ts_num, ret;
	FILE *fp;
	struct timeval tv;
	int diff_time;
	struct thread_args targs;
	pthread_t tid[8] = {0};

	if (argc < 2) {
		fprintf(stderr, "input file no specified\n");
		return 1;
	}

	ts_num = argc - 1;
	printf("Decrypt %d ts\n", ts_num);

	ret = pthread_mutex_init(&mutex, NULL);
	if (ret) {
		printf("mutex init failed\n");
		return -1;
	}
	gettimeofday(&start_tv, NULL);

	for (i = 0; i < ts_num; i++) {
		memset(&targs, 0, sizeof(targs));
		targs.fname = argv[i+1];
		printf("----->> fname:%s\n", targs.fname);
		memcpy(&targs.start_tv, &start_tv, sizeof(start_tv));
		pthread_create(&tid[i], NULL, thread_func, &targs);
		usleep(1*1000);
	}

	for (i = 0; i < ts_num; i++) {
		pthread_join(tid[i], NULL);
	}
	gettimeofday(&tv, NULL);
	if (tv.tv_usec < start_tv.tv_usec) {
		diff_time = (tv.tv_sec - 1 - start_tv.tv_sec) * 1000 +
			(tv.tv_usec + 1*1000*1000 - start_tv.tv_usec) / 1000;
	} else {
		diff_time = (tv.tv_sec - start_tv.tv_sec) * 1000 +
			(tv.tv_usec - start_tv.tv_usec) / 1000;
	}
	printf("All decrypt end, time:%d ms\n", diff_time);
	pthread_mutex_destroy(&mutex);

	return 0;
}

