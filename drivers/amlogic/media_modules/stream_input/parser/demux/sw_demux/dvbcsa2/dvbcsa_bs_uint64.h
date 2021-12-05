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

#ifndef DVBCSA_UINT64_H_
#define DVBCSA_UINT64_H_

typedef uint64_t dvbcsa_bs_word_t;

#define BS_BATCH_SIZE 64
#define BS_BATCH_BYTES 8

#define BS_VAL(n)	((dvbcsa_bs_word_t)(n))

#if SIZEOF_LONG == 8
#define BS_VAL64(n)	BS_VAL(0x##n##UL)
#else
#define BS_VAL64(n)	BS_VAL(0x##n##ULL)
#endif

#define BS_VAL32(n)	BS_VAL64(n##n)
#define BS_VAL16(n)	BS_VAL32(n##n)
#define BS_VAL8(n)	BS_VAL16(n##n)

#define BS_AND(a, b)	((a) & (b))
#define BS_OR(a, b)	((a) | (b))
#define BS_XOR(a, b)	((a) ^ (b))
#define BS_NOT(a)	(~(a))

#define BS_SHL(a, n)	((a) << (n))
#define BS_SHR(a, n)	((a) >> (n))
#define BS_SHL8(a, n)	((a) << (8 * (n)))
#define BS_SHR8(a, n)	((a) >> (8 * (n)))
#define BS_EXTRACT8(a, n) ((a) >> (8 * (n)))

#define BS_EMPTY()

#endif

