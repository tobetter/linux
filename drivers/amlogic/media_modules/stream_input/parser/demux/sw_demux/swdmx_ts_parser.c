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

SWDMX_TsParser*
swdmx_ts_parser_new (void)
{
	SWDMX_TsParser *tsp;

	tsp = swdmx_malloc(sizeof(SWDMX_TsParser));
	SWDMX_ASSERT(tsp);

	tsp->packet_size = 188;

	swdmx_list_init(&tsp->cb_list);

	return tsp;
}

SWDMX_Result
swdmx_ts_parser_set_packet_size (
			SWDMX_TsParser *tsp,
			SWDMX_Int       size)
{
	SWDMX_ASSERT(tsp);

	if (size < 188) {
		swdmx_log("packet size should >= 188");
		return SWDMX_ERR;
	}

	tsp->packet_size = size;

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_parser_add_ts_packet_cb (
			SWDMX_TsParser   *tsp,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data)
{
	SWDMX_ASSERT(tsp && cb);

	swdmx_cb_list_add(&tsp->cb_list, cb, data);

	return SWDMX_OK;
}

SWDMX_Result
swdmx_ts_parser_remove_ts_packet_cb (
			SWDMX_TsParser   *tsp,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data)
{
	SWDMX_ASSERT(tsp && cb);

	swdmx_cb_list_remove(&tsp->cb_list, cb, data);

	return SWDMX_OK;
}
/*Parse the TS packet.*/
static void
ts_packet (SWDMX_TsParser *tsp, SWDMX_UInt8 *data)
{
	SWDMX_TsPacket  pkt;
	SWDMX_UInt8    *p = data;
	SWDMX_Int       len;
	SWDMX_UInt8     afc;
	SWDMX_CbEntry  *e, *ne;

	pkt.pid = ((p[1] & 0x1f) << 8) | p[2];
	if (pkt.pid == 0x1fff)
		return;

	pkt.packet        = p;
	pkt.packet_len    = tsp->packet_size;
	pkt.error         = (p[1] & 0x80) >> 7;
	pkt.payload_start = (p[1] & 0x40) >> 6;
	pkt.priority      = (p[1] & 0x20) >> 5;
	pkt.scramble      = p[3] >> 6;
	afc               = (p[3] >> 4) & 0x03;
	pkt.cc            = p[3] & 0x0f;

	p   += 4;
	len = 184;

	if (afc & 2) {
		pkt.adp_field_len = p[0];

		p   ++;
		len --;

		pkt.adp_field = p;

		p   += pkt.adp_field_len;
		len -= pkt.adp_field_len;

		if (len < 0) {
			swdmx_log("kernel:illegal adaptation field length\n");
			return;
		}
	} else {
		pkt.adp_field     = NULL;
		pkt.adp_field_len = 0;
	}

	if (afc & 1) {
		pkt.payload     = p;
		pkt.payload_len = len;
	} else {
		pkt.payload     = NULL;
		pkt.payload_len = 0;
	}

	SWDMX_LIST_FOR_EACH_SAFE(e, ne, &tsp->cb_list, ln) {
		SWDMX_TsPacketCb cb = e->cb;

		cb(&pkt, e->data);
	}
}

SWDMX_Int
swdmx_ts_parser_run (
			SWDMX_TsParser *tsp,
			SWDMX_UInt8    *data,
			SWDMX_Int       len)
{
	SWDMX_UInt8    *p    = data;
	SWDMX_Int       left = len;

	SWDMX_ASSERT(tsp && data);

	while (left >= tsp->packet_size) {
		if (*p == 0x47) {
			ts_packet(tsp, p);

			p    += tsp->packet_size;
			left -= tsp->packet_size;
		} else {
			p    ++;
			left --;
		}
	}

	return len - left;
}

void
swdmx_ts_parser_free (SWDMX_TsParser *tsp)
{
	SWDMX_ASSERT(tsp);

	swdmx_cb_list_clear(&tsp->cb_list);
	swdmx_free(tsp);
}


