/*

    This file is part of libdvbcsa.

    libdvbcsa is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 2 of the License,
    or (at your option) any later version.

    libdvbcsa is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdvbcsa; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA

    Based on FFdecsa, Copyright (C) 2003-2004  fatih89r

    (c) 2006-2008 Alexandre Becoulet <alexandre.becoulet@free.fr>

*/

#ifndef DVBCSA_ALTIVEC_H_
#define DVBCSA_ALTIVEC_H_

#include <altivec.h>

typedef vector unsigned int dvbcsa_bs_word_t;

#define BS_BATCH_SIZE 128
#define BS_BATCH_BYTES 16

#define BS_VAL_(a,n,m)	({						\
		a uint32_t _v[4] __attribute__((aligned(16))) = {	\
			(uint32_t)((n)>>32), (uint32_t)(n),		\
			(uint32_t)((m)>>32), (uint32_t)(m) };		\
		vec_ld(0, _v);						\
	})

#define BS_VAL(n, m)	BS_VAL_(, n, m)
#define BS_VAL64(n)	BS_VAL_(static const, 0x##n##ULL, 0x##n##ULL)
#define BS_VAL32(n)	BS_VAL64(n##n)
#define BS_VAL16(n)	BS_VAL32(n##n)

static DVBCSA_INLINE dvbcsa_bs_word_t get_bs_val8( uint8_t val, dvbcsa_bs_word_t def_val )
{
	if ( (val & 0xf) == val ) {
		return (dvbcsa_bs_word_t)vec_splat_u8(val);
	}

	switch (val) {
	case 0x02:
		return (dvbcsa_bs_word_t)vec_splat_u8(2);
	case 0x03:
		return (dvbcsa_bs_word_t)vec_splat_u8(3);
	case 0x04:
		return (dvbcsa_bs_word_t)vec_splat_u8(4);
	case 0x10: {
		dvbcsa_bs_word_t x = vec_splat_u8(8);
		return (dvbcsa_bs_word_t)vec_add(x,x);
	}
	case 0x29: {
		dvbcsa_bs_word_t x = vec_splat_u8(10);
		return (dvbcsa_bs_word_t)vec_add(vec_splat_u8(0x1),vec_rl(x,x));
	}
	case 0x40: {
		dvbcsa_bs_word_t x = vec_splat_u8(4);
		return (dvbcsa_bs_word_t)vec_rl(x,x);
	}
	case 0x80: {
		vector unsigned char x = vec_splat_u8(6);
		return (dvbcsa_bs_word_t)vec_sl(x,x);
	}
	case 0xff:
		return (dvbcsa_bs_word_t)vec_splat_s8(-1);
	default:
		return def_val;
	}
}

#define BS_VAL8(x) get_bs_val8(0x##x, BS_VAL16(x##x))

#define BS_AND(a, b)	vec_and((a), (b))
#define BS_OR(a, b)	vec_or((a), (b))
#define BS_XOR(a, b)	vec_xor((a), (b))
#define BS_NOT(a)	vec_nor((a), (a))

#define SHVAL_1 BS_VAL8(01)
#define SHVAL_2 BS_VAL8(02)
#define SHVAL_3 BS_VAL8(03)
#define SHVAL_4 BS_VAL8(04)
#define SHVAL_6 BS_VAL8(06)
#define SHVAL_28 BS_VAL8(1C)
#define SHVAL_13 BS_VAL8(0d)
#define SHVAL_14 BS_VAL8(0e)
#define SHVAL_7 BS_VAL8(07)

#define BS_SHL(a, n)	({ dvbcsa_bs_word_t x = SHVAL_##n; vec_sll(vec_slo((a), x), x); })
#define BS_SHR(a, n)	({ dvbcsa_bs_word_t x = SHVAL_##n; vec_srl(vec_sro((a), x), x); })
#define BS_SHL8(a, n)	({ dvbcsa_bs_word_t x = vec_splat_u8(n*2); x = vec_add(x,x); x = vec_add(x,x); vec_slo((a), x); })
#define BS_SHR8(a, n)	({ dvbcsa_bs_word_t x = vec_splat_u8(n*2); x = vec_add(x,x); x = vec_add(x,x); vec_sro((a), x); })

#define BS_EXTRACT8(a, n) ((uint8_t*)&(a))[15 - (n)]

#define BS_EMPTY()

#endif

