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

#ifndef DVBCSA_SSE_H_
#define DVBCSA_SSE_H_

#include <xmmintrin.h>
#include <emmintrin.h>

typedef __m128i dvbcsa_bs_word_t;

#define BS_BATCH_SIZE 128
#define BS_BATCH_BYTES 16

#define BS_VAL(n, m)	_mm_set_epi64x(n, m)
#define BS_VAL64(n)	BS_VAL(0x##n##ULL, 0x##n##ULL)
#define BS_VAL32(n)	BS_VAL64(n##n)
#define BS_VAL16(n)	BS_VAL32(n##n)
#define BS_VAL8(n)	BS_VAL16(n##n)

#define BS_AND(a, b)	_mm_and_si128((a), (b))
#define BS_OR(a, b)	_mm_or_si128((a), (b))
#define BS_XOR(a, b)	_mm_xor_si128((a), (b))
#define BS_NOT(a)	_mm_andnot_si128((a), BS_VAL8(ff))

#define BS_SHL(a, n)	_mm_slli_epi64(a, n)
#define BS_SHR(a, n)	_mm_srli_epi64(a, n)
#define BS_SHL8(a, n)	_mm_slli_si128(a, n)
#define BS_SHR8(a, n)	_mm_srli_si128(a, n)

#define BS_EXTRACT8(a, n) ((uint8_t*)&(a))[n]

#define BS_EMPTY()

#endif
