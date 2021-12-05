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
#include <linux/slab.h>
#include <linux/printk.h>

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)

#define BS_XOREQ(a, b)	do { dvbcsa_bs_word_t *_t = &(a); *_t = BS_XOR(*_t, (b)); } while (0)

DVBCSA_INLINE static inline void
dvbcsa_bs_block_decrypt_register (const dvbcsa_bs_word_t *block, dvbcsa_bs_word_t *r)
{
  int	i, j, g;
  dvbcsa_bs_word_t sbox_out;
  dvbcsa_bs_word_t in;
  dvbcsa_bs_word_t w;
  dvbcsa_bs_word_t *r6_N;

  r += 8 * 56;

  // loop over kk[55]..kk[0]
  for (i = 55; i >= 0; i--)
    {
      r6_N = r + 8 * 6;

      r -= 8;	/* virtual shift of registers */

      for (g = 0; g < 8; g++)
	{
	  union {
	    dvbcsa_bs_word_t so;
	    uint8_t si[BS_BATCH_BYTES];
	  } u;

	  u.so = BS_XOR(block[i], r6_N[g]);

	  for (j = 0; j < BS_BATCH_BYTES; j++)
	    u.si[j] = dvbcsa_block_sbox[u.si[j]];

	  sbox_out = u.so;

	  // bit permutation

	  in = BS_OR(
				      BS_OR(
					    BS_OR (BS_SHL (BS_AND (sbox_out, BS_VAL8(29)), 1),
						   BS_SHL (BS_AND (sbox_out, BS_VAL8(02)), 6)),
					    BS_OR (BS_SHL (BS_AND (sbox_out, BS_VAL8(04)), 3),
						   BS_SHR (BS_AND (sbox_out, BS_VAL8(10)), 2))),
				      BS_OR(       BS_SHR (BS_AND (sbox_out, BS_VAL8(40)), 6),
					           BS_SHR (BS_AND (sbox_out, BS_VAL8(80)), 4)));

	  w = BS_XOR(r[8 * 8 + g], sbox_out);

	  r[8 * 0 + g] = w;
	  BS_XOREQ(r[8 * 2 + g], w);
	  BS_XOREQ(r[8 * 3 + g], w);
	  BS_XOREQ(r[8 * 4 + g], w);
	  BS_XOREQ(r[8 * 6 + g], in);
	}
    }
}

DVBCSA_INLINE static inline void
dvbcsa_bs_block_decrypt_block(const struct dvbcsa_bs_key_s *key,
			      const struct dvbcsa_bs_batch_s *pcks,
			      unsigned int offset)
{
  unsigned int size=8*(8+56);
  dvbcsa_bs_word_t	*r;

  r = kmalloc(size*sizeof(dvbcsa_bs_word_t), GFP_KERNEL);
  if (r == NULL) {
	  pr_error("dvbcsa_bs_block_decrypt_block fail\n");
      return ;
  }

  dvbcsa_bs_block_transpose_in(r + 8 * 56, pcks, offset);
  dvbcsa_bs_block_decrypt_register(key->block, r);
  dvbcsa_bs_block_transpose_out(r, pcks, offset);
  kfree(r);
}

void dvbcsa_bs_block_decrypt_batch(const struct dvbcsa_bs_key_s *key,
				   const struct dvbcsa_bs_batch_s *pcks,
				   unsigned int maxlen)
{
  unsigned int	i;
  unsigned int	g;

  /* decrypt first block */
  dvbcsa_bs_block_decrypt_block(key, pcks, 0);

  for (i = 8; i < maxlen; i += 8)
    {

      /* chained cipher XOR */
      for (g = 0; pcks[g].data; g++)
	if (i < (pcks[g].len & (unsigned)~0x7))
	  dvbcsa_xor_64(pcks[g].data + i - 8, pcks[g].data + i);

      /* decrypt other blocks */
      dvbcsa_bs_block_decrypt_block(key, pcks, i);
    }
}

DVBCSA_INLINE static inline void
dvbcsa_bs_block_encrypt_register (const dvbcsa_bs_word_t *block, dvbcsa_bs_word_t *r)
{
  int	i, j, g;
  dvbcsa_bs_word_t sbox_out;
  dvbcsa_bs_word_t in;
  dvbcsa_bs_word_t w;
  dvbcsa_bs_word_t *r7_N;

  // loop over kk[55]..kk[0]
  for (i = 0; i < 56; i++)
    {
      r7_N = r + 8 * 7;

      r += 8;	/* virtual shift of registers */

      for (g = 0; g < 8; g++)
	{
	  union {
	    dvbcsa_bs_word_t so;
	    uint8_t si[BS_BATCH_BYTES];
	  } u;

	  u.so = BS_XOR(block[i], r7_N[g]);

	  for (j = 0; j < BS_BATCH_BYTES; j++)
	    u.si[j] = dvbcsa_block_sbox[u.si[j]];

	  sbox_out = u.so;

	  // bit permutation

	  in = BS_OR(
				      BS_OR(
					    BS_OR (BS_SHL (BS_AND (sbox_out, BS_VAL8(29)), 1),
						   BS_SHL (BS_AND (sbox_out, BS_VAL8(02)), 6)),
					    BS_OR (BS_SHL (BS_AND (sbox_out, BS_VAL8(04)), 3),
						   BS_SHR (BS_AND (sbox_out, BS_VAL8(10)), 2))),
				      BS_OR(       BS_SHR (BS_AND (sbox_out, BS_VAL8(40)), 6),
					           BS_SHR (BS_AND (sbox_out, BS_VAL8(80)), 4)));

	  w = r[-8 * 1 + g];

	  r[8 * 7 + g] = BS_XOR(w, sbox_out);
	  BS_XOREQ(r[8 * 1 + g], w);
	  BS_XOREQ(r[8 * 2 + g], w);
	  BS_XOREQ(r[8 * 3 + g], w);
	  BS_XOREQ(r[8 * 5 + g], in);
	}
    }
}

DVBCSA_INLINE static inline void
dvbcsa_bs_block_encrypt_block(const struct dvbcsa_bs_key_s *key,
			      const struct dvbcsa_bs_batch_s *pcks,
			      unsigned int offset)
{
  unsigned int size=8*(8+56);
  dvbcsa_bs_word_t	*r;

  r = kmalloc(size*sizeof(dvbcsa_bs_word_t), GFP_KERNEL);
  if (r == NULL) {
	  pr_error("dvbcsa_bs_block_decrypt_block fail\n");
      return ;
  }
  dvbcsa_bs_block_transpose_in(r, pcks, offset);
  dvbcsa_bs_block_encrypt_register(key->block, r);
  dvbcsa_bs_block_transpose_out(r + 8 * 56, pcks, offset);
  kfree(r);
}

void dvbcsa_bs_block_encrypt_batch(const struct dvbcsa_bs_key_s *key,
				   const struct dvbcsa_bs_batch_s *pcks,
				   unsigned int maxlen)
{
  int	i;
  unsigned int	g;

  dvbcsa_bs_block_encrypt_block(key, pcks, maxlen - 8);

  for (i = maxlen - 16; i >= 0; i -= 8)
    {
      /* chained cipher XOR */
      for (g = 0; pcks[g].data; g++)
	if (i + 8 < (pcks[g].len & (unsigned)~0x7))
	  dvbcsa_xor_64(pcks[g].data + i, pcks[g].data + i + 8);

      /* encrypt other blocks */
      dvbcsa_bs_block_encrypt_block(key, pcks, i);
    }
}

