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
#ifdef __KERNEL__
#include <linux/crypto.h>
#else
#include "crypto/aes.h"
#endif

typedef struct {
	SWDMX_DescAlgo algo;
	SWDMX_Int      align;
#ifdef __KERNEL__
	struct crypto_cipher *tfm;
	SWDMX_UInt8 odd_key[16];
	SWDMX_UInt8 even_key[16];
#else
	AES_KEY        odd;
	AES_KEY        even;
#endif
} SWDMX_AesEcbAlgo;

static SWDMX_Result
aes_ecb_set (SWDMX_DescAlgo *p, SWDMX_Int type, SWDMX_Ptr param)
{
	SWDMX_AesEcbAlgo *algo = (SWDMX_AesEcbAlgo*)p;
	SWDMX_UInt8      *key;
	SWDMX_Result      r;

	switch (type) {
	case SWDMX_AES_ECB_PARAM_ODD_KEY:
		key = param;
		r   = SWDMX_OK;
#ifdef __KERNEL__
		memcpy(&algo->odd_key,key,16);
#else
		AES_set_decrypt_key(key, 128, &algo->odd);
#endif
		break;
	case SWDMX_AES_ECB_PARAM_EVEN_KEY:
		key = param;
		r   = SWDMX_OK;
#ifdef __KERNEL__
		memcpy(&algo->even_key,key,16);
#else
		AES_set_decrypt_key(key, 128, &algo->even);
#endif
		break;
	case SWDMX_AES_ECB_PARAM_ALIGN:
		algo->align = SWDMX_PTR2SIZE(param);
		r   = SWDMX_OK;
		break;
	default:
		swdmx_log("illegal DVBCSA2 parameter");
		r = SWDMX_ERR;
		break;
	}

	return r;
}

//key_type:
//0:even_key
//1:odd_key
static void
aes_ecb_desc_pkt (SWDMX_AesEcbAlgo *algo, SWDMX_Int key_type, SWDMX_TsPacket *pkt, SWDMX_Int align)
{
	SWDMX_UInt8 *p    = pkt->payload;
	SWDMX_Int    left = pkt->payload_len, len;
	SWDMX_UInt8  obuf[184];
	SWDMX_UInt8 *in;
	SWDMX_UInt8 *out  = obuf;
#ifdef __KERNEL__
	u8 *key = NULL;
#else
	AES_KEY *key = NULL;
#endif

	if (align == SWDMX_DESC_ALIGN_TAIL) {
		SWDMX_Int head = left & 15;

		p    += head;
		left -= head;
	} else {
		SWDMX_Int tail = left & 15;

		left -= tail;
	}
#ifdef __KERNEL__
	if (key_type == 0) {
		key = (u8*)&algo->even_key;
	}else{
		key = (u8*)&algo->odd_key;
	}
	crypto_cipher_setkey(algo->tfm,key, 16);
#else
	if (key_type == 0) {
		key = &algo->even;
	}else{
		key = &algo->odd;
	}
#endif
	in  = p;
	len = left;

	while (left >= 16) {
#ifdef __KERNEL__
		crypto_cipher_decrypt_one(algo->tfm,out, p);
#else
		AES_ecb_encrypt(p, out, key, 0);
#endif
		p    += 16;
		out  += 16;
		left -= 16;
	}

	memcpy(in, out, len);
}

static SWDMX_Result
aes_ecb_desc (SWDMX_DescAlgo *p, SWDMX_TsPacket *pkt)
{
	SWDMX_AesEcbAlgo *algo = (SWDMX_AesEcbAlgo*)p;

	if (pkt->scramble == 2) {
		aes_ecb_desc_pkt(algo,0, pkt, algo->align);
	} else if (pkt->scramble == 3) {
		aes_ecb_desc_pkt(algo,1, pkt, algo->align);
	} else {
		swdmx_log("illegal scramble control field");
		return SWDMX_ERR;
	}

	return SWDMX_OK;
}

static void
aes_ecb_free (SWDMX_DescAlgo *p)
{
	SWDMX_AesEcbAlgo *algo = (SWDMX_AesEcbAlgo*)p;

	swdmx_free(algo);
}

SWDMX_DescAlgo*
swdmx_aes_ecb_algo_new (void)
{
	SWDMX_AesEcbAlgo *algo;
	SWDMX_UInt8       key[16] = {0};

	algo = swdmx_malloc(sizeof(SWDMX_AesEcbAlgo));
	SWDMX_ASSERT(algo);

	algo->algo.set_fn  = aes_ecb_set;
	algo->algo.desc_fn = aes_ecb_desc;
	algo->algo.free_fn = aes_ecb_free;
	algo->align        = SWDMX_DESC_ALIGN_HEAD;

	memset(key,0,sizeof(key));

#ifdef __KERNEL__
	algo->tfm = crypto_alloc_cipher("ecb(aes)", 0, CRYPTO_ALG_ASYNC);
	crypto_cipher_setkey(algo->tfm,key, sizeof(key));
#else
	AES_set_decrypt_key(key, 128, &algo->odd);
	AES_set_decrypt_key(key, 128, &algo->even);
#endif
	return (SWDMX_DescAlgo*)algo;
}

