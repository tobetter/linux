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
#include "dvbcsa2/dvbcsa/dvbcsa.h"

typedef struct {
	SWDMX_DescAlgo  algo;
	dvbcsa_key_t   *odd_key;
	dvbcsa_key_t   *even_key;
} SWDMX_DvbCsa2Algo;

static SWDMX_Result
dvbcsa2_set (SWDMX_DescAlgo *p, SWDMX_Int type, SWDMX_Ptr param)
{
	SWDMX_DvbCsa2Algo *algo = (SWDMX_DvbCsa2Algo*)p;
	SWDMX_UInt8       *key;
	SWDMX_Result       r;

	switch (type) {
	case SWDMX_DVBCSA2_PARAM_ODD_KEY:
		key = param;
		r   = SWDMX_OK;
		dvbcsa_key_set(key, algo->odd_key);
		break;
	case SWDMX_DVBCSA2_PARAM_EVEN_KEY:
		key = param;
		r   = SWDMX_OK;
		dvbcsa_key_set(key, algo->even_key);
		break;
	default:
		swdmx_log("illegal DVBCSA2 parameter");
		r = SWDMX_ERR;
		break;
	}

	return r;
}

static SWDMX_Result
dvbcsa2_desc (SWDMX_DescAlgo *p, SWDMX_TsPacket *pkt)
{
	SWDMX_DvbCsa2Algo *algo = (SWDMX_DvbCsa2Algo*)p;

	if (pkt->scramble == 2) {
		dvbcsa_decrypt(algo->even_key, pkt->payload, pkt->payload_len);
	} else if (pkt->scramble == 3) {
		dvbcsa_decrypt(algo->odd_key, pkt->payload, pkt->payload_len);
	} else {
		swdmx_log("illegal scramble control field");
		return SWDMX_ERR;
	}

	return SWDMX_OK;
}

static void
dvbcsa2_free (SWDMX_DescAlgo *p)
{
	SWDMX_DvbCsa2Algo *algo = (SWDMX_DvbCsa2Algo*)p;

	dvbcsa_key_free(algo->odd_key);
	dvbcsa_key_free(algo->even_key);

	swdmx_free(algo);
}

SWDMX_DescAlgo*
swdmx_dvbcsa2_algo_new (void)
{
	SWDMX_DvbCsa2Algo *algo;
	SWDMX_UInt8        key[8] = {0};

	algo = swdmx_malloc(sizeof(SWDMX_DvbCsa2Algo));
	SWDMX_ASSERT(algo);

	algo->algo.set_fn  = dvbcsa2_set;
	algo->algo.desc_fn = dvbcsa2_desc;
	algo->algo.free_fn = dvbcsa2_free;

	algo->odd_key  = dvbcsa_key_alloc();
	algo->even_key = dvbcsa_key_alloc();

	dvbcsa_key_set(key, algo->odd_key);
	dvbcsa_key_set(key, algo->even_key);

	return (SWDMX_DescAlgo*)algo;
}

