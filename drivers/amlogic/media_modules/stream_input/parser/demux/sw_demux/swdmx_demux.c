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


#include "swdemux_internal.h"

/*Get the PID filter with the PID.*/
static SWDMX_PidFilter*
pid_filter_get (SWDMX_Demux *dmx, SWDMX_UInt16 pid)
{
	SWDMX_PidFilter *f;

	SWDMX_LIST_FOR_EACH(f, &dmx->pid_filter_list, ln) {
		if (f->pid == pid)
			return f;
	}

	f = swdmx_malloc(sizeof(SWDMX_PidFilter));
	SWDMX_ASSERT(f);

	f->pid      = pid;
	f->sec_data = NULL;
	f->sec_recv = 0;

	swdmx_list_init(&f->sec_filter_list);
	swdmx_list_init(&f->ts_filter_list);

	swdmx_list_append(&dmx->pid_filter_list, &f->ln);

	return f;
}

/*Try to remove a PID filter.*/
static void
pid_filter_remove (SWDMX_PidFilter *f)
{
	if (!swdmx_list_is_empty(&f->ts_filter_list)
				|| !swdmx_list_is_empty(&f->sec_filter_list))
		return;

	swdmx_list_remove(&f->ln);

	if (f->sec_data)
		swdmx_free(f->sec_data);

	swdmx_free(f);
}

/*Section filter match test.*/
static SWDMX_Bool
sec_filter_match (SWDMX_UInt8 *data, SWDMX_Int len, SWDMX_SecFilter *f)
{
	SWDMX_Int   i;
	SWDMX_UInt8 m = 0, n = 0;
	SWDMX_UInt8 has_n = 0;

	for (i = 0; i < SWDMX_SEC_FILTER_LEN; i ++) {
		SWDMX_UInt8 d, v;

		d = data[i] & f->mam[i];
		v = f->value[i] & f->mam[i];
		m |= d ^ v;

		d = data[i] & f->manm[i];
		v = f->value[i] & f->manm[i];
		n |= d ^ v;

		has_n |= f->manm[i];
	}

	if (m)
		return SWDMX_FALSE;

	if (has_n && !n)
		return SWDMX_FALSE;

	return SWDMX_TRUE;
}

/*Section data resolve.*/
static SWDMX_Int
sec_data (SWDMX_PidFilter *pid_filter, SWDMX_UInt8 *p, SWDMX_Int len)
{
	SWDMX_Int    n, left = len, sec_len;
	SWDMX_UInt8 *sec = pid_filter->sec_data;
	SWDMX_Bool   crc = SWDMX_FALSE;

	swdmx_log("%s enter\n",__FUNCTION__);

	if (pid_filter->sec_recv < 3) {
		n = SWDMX_MIN(left, 3 - pid_filter->sec_recv);

		if (n) {
			memcpy(sec + pid_filter->sec_recv, p, n);

			p    += n;
			left -= n;
			pid_filter->sec_recv += n;
		}
	}

	if (pid_filter->sec_recv < 3)
		return len;

	if (sec[0] == 0xff)
		return len;

	sec_len = (((sec[1] & 0xf) << 8) | sec[2]) + 3;
	n       = SWDMX_MIN(sec_len - pid_filter->sec_recv, left);

	if (n) {
		memcpy(sec + pid_filter->sec_recv, p, n);

		p    += n;
		left -= n;
		pid_filter->sec_recv += n;
	}

	if (pid_filter->sec_recv == sec_len) {
		SWDMX_SecFilter *sec_filter, *n_sec_filter;
		SWDMX_CbEntry   *ce, *nce;

		SWDMX_LIST_FOR_EACH_SAFE(sec_filter,
					n_sec_filter,
					&pid_filter->sec_filter_list,
					pid_ln) {

			if (sec_filter->params.crc32) {
				if (!crc) {
					if (swdmx_crc32(pid_filter->sec_data,
								pid_filter->sec_recv)) {
						swdmx_log("section crc error");
						break;
					}

					crc = SWDMX_TRUE;
				}
			}

			if (!sec_filter_match(pid_filter->sec_data,
						pid_filter->sec_recv,
						sec_filter)) {
				  swdmx_log("not sec_filter_match\n");
			      continue;
			}

			SWDMX_LIST_FOR_EACH_SAFE(ce, nce, &sec_filter->cb_list,
						ln) {
				SWDMX_SecCb cb = ce->cb;

				cb(pid_filter->sec_data, pid_filter->sec_recv,
							ce->data);
			}
		}

		pid_filter->sec_recv = 0;
	}

	return len - left;
}

/*PID filter receive TS data.*/
static void
pid_filter_data (SWDMX_PidFilter *pid_filter, SWDMX_TsPacket *pkt)
{
	SWDMX_TsFilter  *ts_filter, *n_ts_filter;
	SWDMX_CbEntry   *ce, *nce;
	SWDMX_UInt8     *p;
	SWDMX_Int        left;

	/*TS filters.*/
	SWDMX_LIST_FOR_EACH_SAFE(ts_filter,
				n_ts_filter,
				&pid_filter->ts_filter_list,
				pid_ln) {
		SWDMX_LIST_FOR_EACH_SAFE(ce, nce, &ts_filter->cb_list, ln) {
			SWDMX_TsPacketCb cb = ce->cb;

			cb(pkt, ce->data);
		}
	}

	if (swdmx_list_is_empty(&pid_filter->sec_filter_list))
		return;

	if (!pkt->payload || pkt->scramble)
		return;

	swdmx_log("%s handle section \n",__FUNCTION__);

	/*Solve section data.*/
	if (!pid_filter->sec_data) {
		pid_filter->sec_data = swdmx_malloc(4096 + 3);
		SWDMX_ASSERT(pid_filter->sec_data);
	}

	p    = pkt->payload;
	left = pkt->payload_len;
	if (pkt->payload_start) {
		if (left) {
			SWDMX_Int ptr = p[0];

			p    ++;
			left --;

			if (ptr) {
				if (ptr > left) {
					swdmx_log("illegal section pointer field");
					return;
				}

				if (pid_filter->sec_recv) {
					sec_data(pid_filter, p, ptr);
				}

				p    += ptr;
				left -= ptr;
			}
		}

		pid_filter->sec_recv = 0;
	}

	while (left > 0) {
		SWDMX_Int n;

		n = sec_data(pid_filter, p, left);

		p    += n;
		left -= n;
	}
}

SWDMX_Demux*
swdmx_demux_new (void)
{
	SWDMX_Demux *dmx;

	dmx = swdmx_malloc(sizeof(SWDMX_Demux));
	SWDMX_ASSERT(dmx);

	swdmx_list_init(&dmx->pid_filter_list);
	swdmx_list_init(&dmx->ts_filter_list);
	swdmx_list_init(&dmx->sec_filter_list);

	return dmx;
}

SWDMX_TsFilter*
swdmx_demux_alloc_ts_filter (SWDMX_Demux *dmx)
{
	SWDMX_TsFilter  *f;

	SWDMX_ASSERT(dmx);

	f = swdmx_malloc(sizeof(SWDMX_TsFilter));
	SWDMX_ASSERT(f);

	f->dmx        = dmx;
	f->state      = SWDMX_FILTER_STATE_INIT;
	f->pid_filter = NULL;

	swdmx_list_init(&f->cb_list);

	swdmx_list_append(&dmx->ts_filter_list, &f->ln);

	return f;
}

SWDMX_SecFilter*
swdmx_demux_alloc_sec_filter (SWDMX_Demux *dmx)
{
	SWDMX_SecFilter *f;

	SWDMX_ASSERT(dmx);

	f = swdmx_malloc(sizeof(SWDMX_SecFilter));

	f->dmx        = dmx;
	f->state      = SWDMX_FILTER_STATE_INIT;
	f->pid_filter = NULL;

	swdmx_list_init(&f->cb_list);

	swdmx_list_append(&dmx->sec_filter_list, &f->ln);

	return f;
}

void
swdmx_demux_ts_packet_cb (
			SWDMX_TsPacket *pkt,
			SWDMX_Ptr       data)
{
	SWDMX_Demux     *dmx = (SWDMX_Demux*)data;
	SWDMX_PidFilter *pid_filter;

	SWDMX_ASSERT(pkt && dmx);

	SWDMX_LIST_FOR_EACH(pid_filter, &dmx->pid_filter_list, ln) {
		if (pkt->pid == pid_filter->pid) {
			pid_filter_data(pid_filter, pkt);
			break;
		}
	}
}

void
swdmx_demux_free (SWDMX_Demux *dmx)
{
	SWDMX_ASSERT(dmx);

	while (!swdmx_list_is_empty(&dmx->ts_filter_list)) {
		SWDMX_TsFilter *f;

		f = SWDMX_CONTAINEROF(dmx->ts_filter_list.next, SWDMX_TsFilter, ln);

		swdmx_ts_filter_free(f);
	}

	while (!swdmx_list_is_empty(&dmx->sec_filter_list)) {
		SWDMX_SecFilter *f;

		f = SWDMX_CONTAINEROF(dmx->sec_filter_list.next, SWDMX_SecFilter, ln);

		swdmx_sec_filter_free(f);
	}

	while (!swdmx_list_is_empty(&dmx->pid_filter_list)) {
		SWDMX_PidFilter *f;

		f = SWDMX_CONTAINEROF(dmx->pid_filter_list.next, SWDMX_PidFilter, ln);

		pid_filter_remove(f);
	}

	swdmx_free(dmx);
}

SWDMX_Result
swdmx_ts_filter_set_params (
			SWDMX_TsFilter       *filter,
			SWDMX_TsFilterParams *params)
{
	SWDMX_ASSERT(filter && params);

	if (!swdmx_is_valid_pid(params->pid) || (params->pid == 0x1fff)) {
		swdmx_log("illegal PID 0x%04x", params->pid);
		return SWDMX_ERR;
	}

	if (filter->state == SWDMX_FILTER_STATE_RUN) {
		SWDMX_ASSERT(filter->pid_filter);

		if (filter->params.pid != params->pid) {
			swdmx_list_remove(&filter->pid_ln);
			pid_filter_remove(filter->pid_filter);

			filter->pid_filter = NULL;
		}
	}

	filter->params = *params;
	if (filter->state == SWDMX_FILTER_STATE_INIT)
		filter->state = SWDMX_FILTER_STATE_SET;

	if ((filter->state == SWDMX_FILTER_STATE_RUN) && !filter->pid_filter) {
		SWDMX_PidFilter *pid_filter;

		pid_filter = pid_filter_get(filter->dmx, params->pid);

		filter->pid_filter = pid_filter;

		swdmx_list_append(&pid_filter->ts_filter_list, &filter->pid_ln);
	}

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_filter_add_ts_packet_cb (
			SWDMX_TsFilter   *filter,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data)
{
	SWDMX_ASSERT(filter && cb);

	swdmx_cb_list_add(&filter->cb_list, cb, data);

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_filter_remove_ts_packet_cb (
			SWDMX_TsFilter   *filter,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data)
{
	SWDMX_ASSERT(filter && cb);

	swdmx_cb_list_remove(&filter->cb_list, cb, data);

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_filter_enable (SWDMX_TsFilter *filter)
{
	SWDMX_ASSERT(filter);

	if (filter->state == SWDMX_FILTER_STATE_INIT) {
		swdmx_log("the ts filter's parameters has not been set");
		return SWDMX_ERR;
	}

	if (filter->state == SWDMX_FILTER_STATE_SET) {
		SWDMX_PidFilter *pid_filter;

		pid_filter = pid_filter_get(filter->dmx, filter->params.pid);

		filter->pid_filter = pid_filter;
		filter->state      = SWDMX_FILTER_STATE_RUN;

		swdmx_list_append(&pid_filter->ts_filter_list, &filter->pid_ln);
	}

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_filter_disable (SWDMX_TsFilter *filter)
{
	SWDMX_ASSERT(filter);

	if (filter->state == SWDMX_FILTER_STATE_RUN) {
		SWDMX_ASSERT(filter->pid_filter);

		swdmx_list_remove(&filter->pid_ln);
		pid_filter_remove(filter->pid_filter);

		filter->pid_filter = NULL;
		filter->state      = SWDMX_FILTER_STATE_SET;
	}

	return SWDMX_OK;
}

void
swdmx_ts_filter_free (SWDMX_TsFilter *filter)
{
	SWDMX_ASSERT(filter);

	swdmx_ts_filter_disable(filter);

	swdmx_list_remove(&filter->ln);
	swdmx_cb_list_clear(&filter->cb_list);

	swdmx_free(filter);
}

SWDMX_Result
swdmx_sec_filter_set_params (
			SWDMX_SecFilter       *filter,
			SWDMX_SecFilterParams *params)
{
	SWDMX_Int i;

	SWDMX_ASSERT(filter && params);

	if (!swdmx_is_valid_pid(params->pid) || (params->pid == 0x1fff)) {
		swdmx_log("illegal PID 0x%04x", params->pid);
		return SWDMX_ERR;
	}

	if (filter->state == SWDMX_FILTER_STATE_RUN) {
		SWDMX_ASSERT(filter->pid_filter);

		if (filter->params.pid != params->pid) {
			swdmx_list_remove(&filter->pid_ln);
			pid_filter_remove(filter->pid_filter);

			filter->pid_filter = NULL;
		}
	}

	filter->params = *params;

	for (i = 0; i < SWDMX_SEC_FILTER_LEN; i ++) {
		SWDMX_Int   j = i ? i + 2 : i;
		SWDMX_UInt8 v, mask, mode;

		v    = params->value[i];
		mask = params->mask[i];
		mode = ~params->mode[i];

		filter->value[j] = v;
		filter->mam[j]   = mask & mode;
		filter->manm[j]  = mask & ~mode;
	}

	filter->value[1] = 0;
	filter->mam[1]   = 0;
	filter->manm[1]  = 0;
	filter->value[2] = 0;
	filter->mam[2]   = 0;
	filter->manm[2]  = 0;

	if (filter->state == SWDMX_FILTER_STATE_INIT)
		filter->state = SWDMX_FILTER_STATE_SET;

	if ((filter->state == SWDMX_FILTER_STATE_RUN) && !filter->pid_filter) {
		SWDMX_PidFilter *pid_filter;

		pid_filter = pid_filter_get(filter->dmx, params->pid);

		filter->pid_filter = pid_filter;

		swdmx_list_append(&pid_filter->sec_filter_list, &filter->pid_ln);
	}

	return SWDMX_OK;
}

SWDMX_Result
swdmx_sec_filter_add_section_cb (
			SWDMX_SecFilter *filter,
			SWDMX_SecCb      cb,
			SWDMX_Ptr        data)
{
	SWDMX_ASSERT(filter && cb);

	swdmx_cb_list_add(&filter->cb_list, cb, data);

	return SWDMX_OK;
}


SWDMX_Result
swdmx_sec_filter_remove_section_cb (
			SWDMX_SecFilter *filter,
			SWDMX_SecCb      cb,
			SWDMX_Ptr        data)
{
	SWDMX_ASSERT(filter && cb);

	swdmx_cb_list_remove(&filter->cb_list, cb, data);

	return SWDMX_OK;
}


SWDMX_Result
swdmx_sec_filter_enable (SWDMX_SecFilter *filter)
{
	SWDMX_ASSERT(filter);

	if (filter->state == SWDMX_FILTER_STATE_INIT) {
		swdmx_log("the section filter's parameters has not been set");
		return SWDMX_ERR;
	}

	if (filter->state == SWDMX_FILTER_STATE_SET) {
		SWDMX_PidFilter *pid_filter;

		pid_filter = pid_filter_get(filter->dmx, filter->params.pid);

		filter->pid_filter = pid_filter;
		filter->state      = SWDMX_FILTER_STATE_RUN;

		swdmx_list_append(&pid_filter->sec_filter_list, &filter->pid_ln);
	}

	return SWDMX_OK;
}


SWDMX_Result
swdmx_sec_filter_disable (SWDMX_SecFilter *filter)
{
	SWDMX_ASSERT(filter);

	if (filter->state == SWDMX_FILTER_STATE_RUN) {
		SWDMX_ASSERT(filter->pid_filter);

		swdmx_list_remove(&filter->pid_ln);
		pid_filter_remove(filter->pid_filter);

		filter->pid_filter = NULL;
		filter->state      = SWDMX_FILTER_STATE_SET;
	}

	return SWDMX_OK;
}

void
swdmx_sec_filter_free (SWDMX_SecFilter *filter)
{
	SWDMX_ASSERT(filter);
	swdmx_sec_filter_disable(filter);

	swdmx_list_remove(&filter->ln);
	swdmx_cb_list_clear(&filter->cb_list);

	swdmx_free(filter);
}

