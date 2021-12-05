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

    (c) 2013 Alexandre Becoulet <alexandre.becoulet@free.fr>

*/

#ifndef DVBCSA_NEON_H_
#define DVBCSA_NEON_H_

# include <arm_neon.h>

typedef uint32x4_t dvbcsa_bs_word_t __attribute__((aligned(16)));

#define BS_BATCH_SIZE 128
#define BS_BATCH_BYTES 16

#define BS_VAL(n, m)	vreinterpretq_u32_u64(vcombine_u64(vcreate_u64(m), vcreate_u64(n)))
#define BS_VAL64(n)	vreinterpretq_u32_u64(vdupq_n_u64(0x##n##ULL))
#define BS_VAL32(n)	vdupq_n_u32(0x##n##UL)
#define BS_VAL16(n)	BS_VAL32(n##n)
#define BS_VAL8(n)	BS_VAL16(n##n)

#define BS_AND(a, b)	vandq_u32((a), (b))
#define BS_OR(a, b)	vorrq_u32((a), (b))
#define BS_XOR(a, b)	veorq_u32((a), (b))
#define BS_NOT(a)	vmvnq_u32(a)

#define BS_SHL(a, n)	vreinterpretq_u32_u64(vshlq_n_u64 (vreinterpretq_u64_u32(a), n))
#define BS_SHR(a, n)	vreinterpretq_u32_u64(vshrq_n_u64 (vreinterpretq_u64_u32(a), n))
#define BS_SHL8(a, n)	(n == 1 ? vreinterpretq_u32_u8(vrev16q_u8(vreinterpretq_u8_u32(a))) : ( \
                         n == 2 ? vreinterpretq_u32_u16(vrev32q_u16(vreinterpretq_u16_u32(a))) : \
				  vrev64q_u32(a)))
#define BS_SHR8(a, n)   BS_SHL8(a, n)

#define BS_EXTRACT8(a, n) ((uint8_t*)&(a))[n]

#define BS_EMPTY()

#endif

