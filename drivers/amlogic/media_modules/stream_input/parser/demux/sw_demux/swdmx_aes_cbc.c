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
#include <linux/scatterlist.h>
#include <linux/crypto.h>
#else
#include <crypto/aes.h>
#endif

typedef struct {
	SWDMX_DescAlgo algo;
	SWDMX_Int      align;
#ifdef __KERNEL__
	struct blkcipher_desc desc;
	SWDMX_UInt8 odd_key[16];
	SWDMX_UInt8 even_key[16];
#else
	AES_KEY        odd;
	AES_KEY        even;
#endif
	SWDMX_UInt8    odd_iv[16];
	SWDMX_UInt8    even_iv[16];
} SWDMX_AesCbcAlgo;

static SWDMX_Result
aes_cbc_set (SWDMX_DescAlgo *p, SWDMX_Int type, SWDMX_Ptr param)
{
	SWDMX_AesCbcAlgo *algo = (SWDMX_AesCbcAlgo*)p;
	SWDMX_UInt8      *key;
	SWDMX_Result      r;

	switch (type) {
	case SWDMX_AES_CBC_PARAM_ODD_KEY:
		key = param;
		r   = SWDMX_OK;
#ifdef __KERNEL__
		memcpy(algo->odd_key,key,16);
#else
		AES_set_decrypt_key(key, 128, &algo->odd);
#endif
		break;
	case SWDMX_AES_CBC_PARAM_EVEN_KEY:
		key = param;
		r   = SWDMX_OK;
#ifdef __KERNEL__
		memcpy(algo->even_key,key,16);
#else
		AES_set_decrypt_key(key, 128, &algo->even);
#endif
		break;
	case SWDMX_AES_CBC_PARAM_ALIGN:
		algo->align = SWDMX_PTR2SIZE(param);
		r   = SWDMX_OK;
		break;
	case SWDMX_AES_CBC_PARAM_ODD_IV:
		key = param;
		r   = SWDMX_OK;
		memcpy(algo->odd_iv, key, 16);
		break;
	case SWDMX_AES_CBC_PARAM_EVEN_IV:
		key = param;
		r   = SWDMX_OK;
		memcpy(algo->even_iv, key, 16);
		break;
	default:
		swdmx_log("illegal DVBCSA2 parameter");
		r = SWDMX_ERR;
		break;
	}

	return r;
}

static void
aes_cbc_desc_pkt (SWDMX_AesCbcAlgo *algo, SWDMX_Int key_type, SWDMX_UInt8 *iv, SWDMX_TsPacket *pkt, SWDMX_Int align)
{
	SWDMX_UInt8 *p    = pkt->payload;
	SWDMX_Int    left = pkt->payload_len, len;
	SWDMX_UInt8  obuf[184];
	SWDMX_UInt8  ivbuf[16];
	SWDMX_UInt8 *in;
	SWDMX_UInt8 *out  = obuf;
#ifdef __KERNEL__
	struct scatterlist dst[1];
	struct scatterlist src[1];

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
	crypto_blkcipher_setkey(algo->desc.tfm,key, 16);
#else
	if (key_type == 0) {
		key = &algo->even;
	}else{
		key = &algo->odd;
	}
#endif

	memcpy(ivbuf, iv, 16);

	in  = p;
	len = left;

#ifdef __KERNEL__
	crypto_blkcipher_set_iv(algo->desc.tfm,ivbuf,16);

	sg_init_table(dst,1);
	sg_init_table(src,1);
	sg_set_buf(dst,out,len);
	sg_set_buf(src,in,len);
	crypto_blkcipher_decrypt(&algo->desc,dst,src,len);
#else
	AES_cbc_encrypt(in, out, len, key, ivbuf, 0);
#endif
	memcpy(in, out, len);
}

static SWDMX_Result
aes_cbc_desc (SWDMX_DescAlgo *p, SWDMX_TsPacket *pkt)
{
	SWDMX_AesCbcAlgo *algo = (SWDMX_AesCbcAlgo*)p;

	if (pkt->scramble == 2) {
		aes_cbc_desc_pkt(algo,0, algo->even_iv, pkt, algo->align);
	} else if (pkt->scramble == 3) {
		aes_cbc_desc_pkt(algo,1, algo->odd_iv, pkt, algo->align);
	} else {
		swdmx_log("illegal scramble control field");
		return SWDMX_ERR;
	}

	return SWDMX_OK;
}

static void
aes_cbc_free (SWDMX_DescAlgo *p)
{
	SWDMX_AesCbcAlgo *algo = (SWDMX_AesCbcAlgo*)p;

	swdmx_free(algo);
}

SWDMX_DescAlgo*
swdmx_aes_cbc_algo_new (void)
{
	SWDMX_AesCbcAlgo *algo;
	SWDMX_UInt8       key[16] = {0};

	algo = swdmx_malloc(sizeof(SWDMX_AesCbcAlgo));
	SWDMX_ASSERT(algo);

	algo->algo.set_fn  = aes_cbc_set;
	algo->algo.desc_fn = aes_cbc_desc;
	algo->algo.free_fn = aes_cbc_free;
	algo->align        = SWDMX_DESC_ALIGN_HEAD;

	memset(key,0,sizeof(key));
#ifdef __KERNEL__
	algo->desc.tfm = crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
	algo->desc.flags = 0;
	algo->desc.info = 0;
	crypto_blkcipher_setkey(algo->desc.tfm,key, sizeof(key));
#else
	AES_set_decrypt_key(key, 128, &algo->odd);
	AES_set_decrypt_key(key, 128, &algo->even);
#endif
	memset(algo->odd_iv, 0, 16);
	memset(algo->even_iv, 0, 16);

	return (SWDMX_DescAlgo*)algo;
}
