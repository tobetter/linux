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

SWDMX_Descrambler*
swdmx_descrambler_new (void)
{
	SWDMX_Descrambler *desc;

	desc = swdmx_malloc(sizeof(SWDMX_Descrambler));
	SWDMX_ASSERT(desc);

	swdmx_list_init(&desc->chan_list);
	swdmx_list_init(&desc->cb_list);

	return desc;
}

SWDMX_DescChannel*
swdmx_descrambler_alloc_channel (SWDMX_Descrambler *desc)
{
	SWDMX_DescChannel *chan;

	SWDMX_ASSERT(desc);

	chan = swdmx_malloc(sizeof(SWDMX_DescChannel));
	SWDMX_ASSERT(chan);

	chan->algo   = NULL;
	chan->pid    = 0xffff;
	chan->enable = SWDMX_FALSE;

	swdmx_list_append(&desc->chan_list, &chan->ln);

	return chan;
}

void
swdmx_descrambler_ts_packet_cb (
			SWDMX_TsPacket *pkt,
			SWDMX_Ptr       data)
{
	SWDMX_Descrambler *desc = (SWDMX_Descrambler*)data;
	SWDMX_DescChannel *ch, *nch;
	SWDMX_CbEntry     *ce, *nce;

	SWDMX_ASSERT(pkt && desc);

	if (pkt->scramble && pkt->payload) {
		SWDMX_LIST_FOR_EACH_SAFE(ch, nch, &desc->chan_list, ln) {
			if ((ch->enable) && (ch->pid == pkt->pid)) {
				SWDMX_Result r;

				r = ch->algo->desc_fn(ch->algo, pkt);
				if (r == SWDMX_OK) {
					pkt->scramble   = 0;
					pkt->packet[3] &= 0x3f;
				}

				break;
			}
		}
	}

	SWDMX_LIST_FOR_EACH_SAFE(ce, nce, &desc->cb_list, ln) {
		SWDMX_TsPacketCb cb = ce->cb;
		cb(pkt, ce->data);
	}
}

SWDMX_Result
swdmx_descrambler_add_ts_packet_cb (
			SWDMX_Descrambler *desc,
			SWDMX_TsPacketCb   cb,
			SWDMX_Ptr          data)
{
	SWDMX_ASSERT(desc && cb);

	swdmx_cb_list_add(&desc->cb_list, cb, data);

	return SWDMX_OK;
}

SWDMX_Result
swdmx_descrambler_remove_ts_packet_cb (
			SWDMX_Descrambler *desc,
			SWDMX_TsPacketCb   cb,
			SWDMX_Ptr          data)
{
	SWDMX_ASSERT(desc && cb);

	swdmx_cb_list_remove(&desc->cb_list, cb, data);

	return SWDMX_OK;
}

void
swdmx_descrambler_free (SWDMX_Descrambler *desc)
{
	SWDMX_ASSERT(desc);

	while (!swdmx_list_is_empty(&desc->chan_list)) {
		SWDMX_DescChannel *chan;

		chan = SWDMX_CONTAINEROF(desc->chan_list.next,
					SWDMX_DescChannel, ln);

		swdmx_desc_channel_free(chan);
	}

	swdmx_cb_list_clear(&desc->cb_list);

	swdmx_free(desc);
}

SWDMX_Result
swdmx_desc_channel_set_algo (
			SWDMX_DescChannel *chan,
			SWDMX_DescAlgo    *algo)
{
	SWDMX_ASSERT(chan && algo);

	if (chan->algo && chan->algo->free_fn) {
		chan->algo->free_fn(chan->algo);
	}

	chan->algo = algo;

	return SWDMX_OK;
}

SWDMX_Result
swdmx_desc_channel_set_pid (
			SWDMX_DescChannel *chan,
			SWDMX_UInt16       pid)
{
	SWDMX_ASSERT(chan);

	if (!swdmx_is_valid_pid(pid) || (pid == 0x1fff)) {
		swdmx_log("illegal PID 0x%04x", pid);
		return SWDMX_ERR;
	}

	chan->pid = pid;

	return SWDMX_OK;
}

SWDMX_Result
swdmx_desc_channel_set_param (
			SWDMX_DescChannel *chan,
			SWDMX_Int          type,
			SWDMX_Ptr          param)
{
	SWDMX_ASSERT(chan && chan->algo && chan->algo->set_fn);

	return chan->algo->set_fn(chan->algo, type, param);
}


SWDMX_Result
swdmx_desc_channel_enable (SWDMX_DescChannel *chan)
{
	SWDMX_ASSERT(chan);

	if (!swdmx_is_valid_pid(chan->pid)) {
		swdmx_log("descrambler channel's PID has not been set");
		return SWDMX_ERR;
	}

	if (!chan->algo) {
		swdmx_log("descrambler channel's algorithm has not been set");
		return SWDMX_ERR;
	}

	chan->enable = SWDMX_TRUE;

	return SWDMX_OK;
}

SWDMX_Result
swdmx_desc_channel_disable (SWDMX_DescChannel *chan)
{
	SWDMX_ASSERT(chan);

	chan->enable = SWDMX_FALSE;

	return SWDMX_OK;
}

void
swdmx_desc_channel_free (SWDMX_DescChannel *chan)
{
	SWDMX_ASSERT(chan);

	swdmx_list_remove(&chan->ln);

	if (chan->algo && chan->algo->free_fn)
		chan->algo->free_fn(chan->algo);

	swdmx_free(chan);
}

