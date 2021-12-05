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

#include "dvbcsa/dvbcsa.h"
#include "dvbcsa_bs.h"

/***********************************************************************
	Stream cipher transpose
 */

/* 64 rows of 32 bits transposition (bytes transp. - 8x8 rotate counterclockwise)*/

void dvbcsa_bs_stream_transpose_in(const struct dvbcsa_bs_batch_s *pcks, dvbcsa_bs_word_t *row)
{
  int i, j;

  for (i = 0; pcks[i].data; i++)
    if (pcks[i].len >= 8)
      {
	row[i     ] = BS_VAL(dvbcsa_load_le32(pcks[i].data    ));
	row[i + 32] = BS_VAL(dvbcsa_load_le32(pcks[i].data + 4));
      }

  for (j = 0; j < 64; j += 32)
    {
      dvbcsa_bs_word_t t, b;

      for (i = 0; i < 16; i++)
	{
	  t = row[j + i];
	  b = row[j + 16 + i];
	  row[j + i     ] = BS_OR(BS_AND(t, BS_VAL32(0000ffff)), BS_SHL8(BS_AND(b, BS_VAL32(0000ffff)), 2));
	  row[j + 16 + i] = BS_OR(BS_AND(b, BS_VAL32(ffff0000)), BS_SHR8(BS_AND(t, BS_VAL32(ffff0000)), 2));
	}
    }

  for (j = 0; j < 64; j += 16)
    {
      dvbcsa_bs_word_t t, b;

      for (i = 0; i < 8; i++)
	{
	  t = row[j + i];
	  b = row[j + 8 + i];
	  row[j + i    ] = BS_OR(BS_AND(t, BS_VAL16(00ff)), BS_SHL8(BS_AND(b, BS_VAL16(00ff)), 1));
	  row[j + 8 + i] = BS_OR(BS_AND(b, BS_VAL16(ff00)), BS_SHR8(BS_AND(t, BS_VAL16(ff00)), 1));
	}
    }

  for (j = 0; j < 64; j += 8)
    {
      dvbcsa_bs_word_t t, b;

      for (i = 0; i < 4; i++)
	{
	  t = row[j + i];
	  b = row[j + 4 + i];
	  row[j + i    ] = BS_OR(BS_AND(b, BS_VAL8(0f)), BS_SHL(BS_AND(t, BS_VAL8(0f)), 4));
	  row[j + 4 + i] = BS_OR(BS_AND(t, BS_VAL8(f0)), BS_SHR(BS_AND(b, BS_VAL8(f0)), 4));
	}
    }

  for (j = 0; j < 64; j += 4)
    {
      dvbcsa_bs_word_t t, b;

      for (i = 0; i < 2; i++)
	{
	  t = row[j + i];
	  b = row[j + 2 + i];
	  row[j + i    ] = BS_OR(BS_AND(b, BS_VAL8(33)), BS_SHL(BS_AND(t, BS_VAL8(33)), 2));
	  row[j + 2 + i] = BS_OR(BS_AND(t, BS_VAL8(cc)), BS_SHR(BS_AND(b, BS_VAL8(cc)), 2));
	}
    }

  for (j = 0; j < 64; j += 2)
    {
      dvbcsa_bs_word_t t, b;

      t = row[j];
      b = row[j + 1];
      row[j    ] = BS_OR(BS_AND(b, BS_VAL8(55)), BS_SHL(BS_AND(t, BS_VAL8(55)), 1));
      row[j + 1] = BS_OR(BS_AND(t, BS_VAL8(aa)), BS_SHR(BS_AND(b, BS_VAL8(aa)), 1));
    }
}

/* 64 rows of 32 bits transposition (bytes transp. - 8x8 rotate clockwise)*/

void dvbcsa_bs_stream_transpose_out(const struct dvbcsa_bs_batch_s *pcks,
				    unsigned int index, dvbcsa_bs_word_t *row)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      dvbcsa_bs_word_t t, b;

      t = row[i];
      b = row[4 + i];
      row[i    ] = BS_OR(BS_AND(t, BS_VAL32(0000ffff)), BS_SHL8(BS_AND(b, BS_VAL32(0000ffff)), 2));
      row[4 + i] = BS_OR(BS_AND(b, BS_VAL32(ffff0000)), BS_SHR8(BS_AND(t, BS_VAL32(ffff0000)), 2));
    }

  for (j = 0; j < 8; j += 4)
    {
      dvbcsa_bs_word_t t, b;

      for (i = 0; i < 2; i++)
	{
	  t = row[j + i];
	  b = row[j + 2 + i];
	  row[j + i    ] = BS_OR(BS_AND(t, BS_VAL16(00ff)), BS_SHL8(BS_AND(b, BS_VAL16(00ff)), 1));
	  row[j + i + 2] = BS_OR(BS_AND(b, BS_VAL16(ff00)), BS_SHR8(BS_AND(t, BS_VAL16(ff00)), 1));
	}
    }

  for (j = 0; j < 8; j += 2)
    {
      dvbcsa_bs_word_t t, b;

      t = row[j];
      b = row[j + 1];
      row[j + 1] = BS_OR(BS_AND(t, BS_VAL8(0f)), BS_SHL(BS_AND(b, BS_VAL8(0f)), 4)); //(t & 0x0f0f0f0f) | ((b & 0x0f0f0f0f) << 4);
      row[j    ] = BS_OR(BS_AND(b, BS_VAL8(f0)), BS_SHR(BS_AND(t, BS_VAL8(f0)), 4));//((t & 0xf0f0f0f0) >> 4) | (b & 0xf0f0f0f0);
    }

  for (j = 0; j < 8; j++)
    {
      dvbcsa_bs_word_t t;

      t = row[j];

      t = BS_OR(       BS_AND(t, BS_VAL32(cccc3333)),
	  BS_OR(BS_SHR(BS_AND(t, BS_VAL32(33330000)), 14),
		BS_SHL(BS_AND(t, BS_VAL32(0000cccc)), 14)));

      t = BS_OR(       BS_AND(t, BS_VAL16(    aa55)),
          BS_OR(BS_SHR(BS_AND(t, BS_VAL16(    5500)), 7 ),
		BS_SHL(BS_AND(t, BS_VAL16(    00aa)), 7 )));

      t = BS_OR(       BS_AND(t, BS_VAL8 (      81)),

	  BS_OR(BS_SHR(BS_AND(t, BS_VAL8 (      10)), 3 ),
	  BS_OR(BS_SHR(BS_AND(t, BS_VAL8 (      20)), 2 ),
	  BS_OR(BS_SHR(BS_AND(t, BS_VAL8 (      40)), 1 ),

	  BS_OR(BS_SHL(BS_AND(t, BS_VAL8 (      02)), 1 ),
	  BS_OR(BS_SHL(BS_AND(t, BS_VAL8 (      04)), 2 ),
	        BS_SHL(BS_AND(t, BS_VAL8 (      08)), 3 )))))));

      for (i = 0; i < 4; i++)
	{
	  unsigned int k = j * 4 + i;

	  if (!pcks[k].data)
	    return;

	  if (index < pcks[k].len)
	    pcks[k].data[index] ^= BS_EXTRACT8(t, 3 - i);
	}
    }
}

