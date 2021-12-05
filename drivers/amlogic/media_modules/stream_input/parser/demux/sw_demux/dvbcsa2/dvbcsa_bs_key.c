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

    (c) 2006-2011 Alexandre Becoulet <alexandre.becoulet@free.fr>

*/

#include "dvbcsa/dvbcsa.h"
#include "dvbcsa_bs.h"

void
dvbcsa_bs_key_set (const dvbcsa_cw_t cw, struct dvbcsa_bs_key_s *key)
{
  dvbcsa_keys_t kk;
  int i;

  /* precalculations for stream */

  uint64_t ck = dvbcsa_load_le64(cw);

  for (i = 0; i < DVBCSA_CWBITS_SIZE; i++)
    key->stream[i] = (ck >> (i^4)) & 1 ? BS_VAL8(ff) : BS_VAL8(00);

  /* precalculations for block */

  dvbcsa_key_schedule_block(cw, kk);

  for (i = 0; i < DVBCSA_KEYSBUFF_SIZE; i++)
    {
#if BS_BATCH_SIZE == 32
      *(uint32_t*)(key->block + i) = kk[i] * 0x01010101;

#elif BS_BATCH_SIZE == 64
      *(uint64_t*)(key->block + i) = kk[i] * 0x0101010101010101ULL;

#elif BS_BATCH_SIZE > 64 && BS_BATCH_SIZE % 64 == 0
      uint64_t v = kk[i] * 0x0101010101010101ULL;
      int j;

      for (j = 0; j < BS_BATCH_BYTES / 8; j++)
	*((uint64_t*)(key->block + i) + j) = v;
#else
# error
#endif
    }
}

