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

#ifndef DVBCSA_MMX_H_
#define DVBCSA_MMX_H_

# include <mmintrin.h>

typedef __m64 dvbcsa_bs_word_t;

#define BS_BATCH_SIZE 64
#define BS_BATCH_BYTES 8

#define BS_VAL(n)	((dvbcsa_bs_word_t)(n))
#define BS_VAL64(n)	BS_VAL(0x##n##ULL)
#define BS_VAL32(n)	BS_VAL64(n##n)
#define BS_VAL16(n)	BS_VAL32(n##n)
#define BS_VAL8(n)	BS_VAL16(n##n)

#define BS_AND(a, b)	_m_pand((a), (b))
#define BS_OR(a, b)	_m_por((a), (b))
#define BS_XOR(a, b)	_m_pxor ((a), (b))
#define BS_NOT(a)	_m_pxor ((a), BS_VAL8(ff))

#define BS_SHL(a, n)	_m_psllqi((a), n)
#define BS_SHR(a, n)	_m_psrlqi((a), n)
#define BS_SHL8(a, n)	BS_SHL(a, 8 * (n))
#define BS_SHR8(a, n)	BS_SHR(a, 8 * (n))
#define BS_EXTRACT8(a, n) _mm_cvtsi64_si32(_m_psrlqi((a), 8 * (n)))

#define BS_EMPTY()	_m_empty()

#endif

