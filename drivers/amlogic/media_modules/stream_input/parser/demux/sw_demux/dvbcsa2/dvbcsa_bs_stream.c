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

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox1(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2, tmp3;

  tmp0 = BS_XOR (fa, BS_XOR (fb, BS_NOT (BS_OR (BS_XOR (BS_OR (fa, fb), fc), BS_XOR (fc, fd)))));
  tmp1 = BS_XOR (BS_OR (fa, fb), BS_NOT (BS_AND (fc, BS_OR (fa, BS_XOR (fb, fd)))));
  tmp2 = BS_XOR (fa, BS_XOR (BS_AND (fb, fd), BS_OR (BS_AND (fa, fd), fc)));
  tmp3 = BS_XOR (BS_AND (fa, fc), BS_XOR (fa, BS_OR (BS_AND (fa, fb), fd)));

  *sa = BS_XOR (tmp0, BS_AND (fe, tmp1));
  *sb = BS_XOR (tmp2, BS_AND (fe, tmp3));
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox2(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2, tmp3;

  tmp0 = BS_XOR (fa, BS_XOR (BS_AND (fb, BS_OR (fc, fd)), BS_XOR (fc, BS_NOT (fd))));
  tmp1 = BS_OR (BS_AND (fa, BS_XOR (fb, fd)), BS_AND (BS_OR (fa, fb), fc));
  tmp2 = BS_XOR (BS_AND (fb, fd), BS_OR (BS_AND (fa, fd), BS_XOR (fb, BS_NOT (fc))));
  tmp3 = BS_OR (BS_AND (fa, fd), BS_XOR (fa, BS_XOR (fb, BS_AND (fc, fd))));

  *sa = BS_XOR (tmp0, BS_AND (fe, tmp1));
  *sb = BS_XOR (tmp2, BS_AND (fe, tmp3));
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox3(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2;

  tmp0 = BS_XOR (fa, BS_XOR (fb, BS_XOR (BS_AND (fc, BS_OR (fa, fd)), fd)));
  tmp1 = BS_XOR (BS_AND (fa, fc), BS_OR (BS_XOR (fa, fd), BS_XOR (BS_OR (fb, fc), BS_NOT (fd))));
  tmp2 = BS_XOR (fa, BS_XOR (BS_AND (BS_XOR (fb, fc), fd), fc));

  *sa = BS_XOR (tmp0, BS_AND (BS_NOT (fe), tmp1));
  *sb = BS_XOR (tmp2, fe);
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox4(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2;

  tmp0 = BS_XOR (fa, BS_OR (BS_AND (fc, BS_XOR (fa, fd)), BS_XOR (fb, BS_OR (fc, BS_NOT (fd)))));
  tmp1 = BS_XOR (BS_AND (fa, fb), BS_XOR (fb, BS_XOR (BS_AND (BS_OR (fa, fc), fd), fc)));
  tmp2 = BS_XOR (fa, BS_OR (BS_AND (fb, fc), BS_XOR (BS_OR (BS_AND (fa, BS_XOR (fb, fd)), fc), fd)));

  *sa = BS_XOR (tmp0, BS_AND (fe, BS_XOR (tmp1, tmp0)));
  *sb = BS_XOR (BS_XOR (*sa, tmp2), fe);
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox5(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2, tmp3;

  tmp0 = BS_OR (BS_XOR (BS_AND (fa, BS_OR (fb, fc)), fb), BS_XOR (BS_OR (BS_XOR (fa, fc), fd), BS_VAL8(ff)));
  tmp1 = BS_XOR (fb, BS_AND (BS_XOR (fc, fd), BS_XOR (fc, BS_OR (fb, BS_XOR (fa, fd)))));
  tmp2 = BS_XOR (BS_AND (fa, fc), BS_XOR (fb, BS_AND (BS_OR (fb, BS_XOR (fa, fc)), fd)));
  tmp3 = BS_OR (BS_AND (BS_XOR (fa, fb), BS_XOR (fc, BS_VAL8(ff))), fd);

  *sa = BS_XOR (tmp0, BS_AND (fe, tmp1));
  *sb = BS_XOR (tmp2, BS_AND (fe, tmp3));
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox6(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2, tmp3;

  tmp0 = BS_XOR (BS_AND (BS_AND (fa, fc), fd), BS_XOR (BS_AND (fb, BS_OR (fa, fd)), fc));
  tmp1 = BS_NOT (BS_AND (BS_XOR (fa, fc), fd));
  tmp2 = BS_XOR (BS_AND (fa, BS_OR (fb, fc)), BS_XOR (fb, BS_OR (BS_AND (fb, fc), fd)));
  tmp3 = BS_AND (fc, BS_XOR (BS_AND (fa, BS_XOR (fb, fd)), BS_OR (fb, fd)));

  *sa = BS_XOR (tmp0, BS_AND (fe, tmp1));
  *sb = BS_XOR (tmp2, BS_AND (fe, tmp3));
}

static void DVBCSA_INLINE inline
dvbcsa_bs_stream_sbox7(dvbcsa_bs_word_t fa, dvbcsa_bs_word_t fb,
		       dvbcsa_bs_word_t fc, dvbcsa_bs_word_t fd,
		       dvbcsa_bs_word_t fe,
		       dvbcsa_bs_word_t *sa, dvbcsa_bs_word_t *sb)
{
  dvbcsa_bs_word_t tmp0, tmp1, tmp2, tmp3;

  tmp0 = BS_XOR (fb, BS_OR (BS_AND (fc, fd), BS_XOR (fa, BS_XOR (fc, fd))));
  tmp1 = BS_AND (BS_OR (fb, fd), BS_OR (BS_AND (fa, fc), BS_XOR (fb, BS_XOR (fc, fd))));
  tmp2 = BS_XOR (BS_OR (fa, fb), BS_XOR (BS_AND (fc, BS_OR (fb, fd)), fd));
  tmp3 = BS_OR (fd, BS_XOR (BS_AND (fa, fc), BS_VAL8(ff)));

  *sa = BS_XOR (tmp0, BS_AND (fe, tmp1));
  *sb = BS_XOR (tmp2, BS_AND (fe, tmp3));
}

void
dvbcsa_bs_stream_cipher_batch(const struct dvbcsa_bs_key_s *key,
			      const struct dvbcsa_bs_batch_s *pcks,
			      unsigned int maxlen)
{
  dvbcsa_bs_word_t A[10][4];
  dvbcsa_bs_word_t B[10][4];
  dvbcsa_bs_word_t X[4];
  dvbcsa_bs_word_t Y[4];
  dvbcsa_bs_word_t Z[4];
  dvbcsa_bs_word_t D[4];
  dvbcsa_bs_word_t E[4];
  dvbcsa_bs_word_t F[4];
  dvbcsa_bs_word_t p;
  dvbcsa_bs_word_t q;
  dvbcsa_bs_word_t r;
  dvbcsa_bs_word_t in1[4];
  dvbcsa_bs_word_t in2[4];
  dvbcsa_bs_word_t extra_B[4];
  dvbcsa_bs_word_t s1a, s1b, s2a, s2b, s3a, s3b, s4a, s4b, s5a, s5b, s6a, s6b, s7a, s7b;
  dvbcsa_bs_word_t next_E[4];
  dvbcsa_bs_word_t tmp0, tmp1, tmp3, tmp4;
  dvbcsa_bs_word_t sb[64];
  int h, i, j, k, b;
  dvbcsa_bs_word_t	B_next[4];

  dvbcsa_bs_stream_transpose_in(pcks, sb);

  for (b = 0; b < 4; b++)
    {
      for (i = 0; i < 8; i++)
	{
	  A[i][b] = key->stream[b + i * 4];
	  B[i][b] = key->stream[b + i * 4 + 32];
	}

      // all other regs = 0
      A[8][b] = BS_VAL8(00);
      A[9][b] = BS_VAL8(00);
      B[8][b] = BS_VAL8(00);
      B[9][b] = BS_VAL8(00);

      X[b] = BS_VAL8(00);
      Y[b] = BS_VAL8(00);
      Z[b] = BS_VAL8(00);
      D[b] = BS_VAL8(00);
      E[b] = BS_VAL8(00);
      F[b] = BS_VAL8(00);
    }

  p = BS_VAL8(00);
  q = BS_VAL8(00);
  r = BS_VAL8(00);

  /* Stream INIT */

  for (i = 0; i < 8; i++)
    {

      for (b = 0; b < 4; b++)
	{
	  in1[b] = sb[8 * i + 4 + b];
	  in2[b] = sb[8 * i + b];
	}

      for (j = 0; j < 4; j++)
	{
	  dvbcsa_bs_stream_sbox1(A[0][2], A[5][1], A[6][3], A[8][0], A[3][0], &s1a, &s1b);
	  dvbcsa_bs_stream_sbox2(A[2][2], A[5][3], A[6][0], A[8][1], A[1][1], &s2a, &s2b);
	  dvbcsa_bs_stream_sbox3(A[1][0], A[4][1], A[4][3], A[5][2], A[0][3], &s3a, &s3b);
	  dvbcsa_bs_stream_sbox4(A[0][1], A[1][3], A[3][2], A[7][0], A[2][3], &s4a, &s4b);
	  dvbcsa_bs_stream_sbox5(A[3][3], A[5][0], A[7][1], A[8][2], A[4][2], &s5a, &s5b);
	  dvbcsa_bs_stream_sbox6(A[3][1], A[4][0], A[6][2], A[8][3], A[2][1], &s6a, &s6b);
	  dvbcsa_bs_stream_sbox7(A[2][0], A[6][1], A[7][2], A[7][3], A[1][2], &s7a, &s7b);

	  extra_B[3] = BS_XOR (BS_XOR (BS_XOR (B[2][0], B[5][1]), B[6][2]), B[8][3]);
	  extra_B[2] = BS_XOR (BS_XOR (BS_XOR (B[5][0], B[7][1]), B[2][3]), B[3][2]);
	  extra_B[1] = BS_XOR (BS_XOR (BS_XOR (B[4][3], B[7][2]), B[3][0]), B[4][1]);
	  extra_B[0] = BS_XOR (BS_XOR (BS_XOR (B[8][2], B[5][3]), B[2][1]), B[7][0]);

	  for (b = 0; b < 4; b++)
	    {
	      dvbcsa_bs_word_t	A_next;

	      A_next = BS_XOR (A[9][b], X[b]);
	      A_next = BS_XOR (BS_XOR (A_next, D[b]), ((j % 2) ? in2[b] : in1[b]));

	      for (k = 9; k > 0; k--)
		A[k][b] = A[k - 1][b];

	      A[0][b] = A_next;
	    }

	  //dvbcsa_bs_word_t	B_next[4];

	  for (b = 0; b < 4; b++)
	    {
	      B_next[b] = BS_XOR (BS_XOR (B[6][b], B[9][b]), Y[b]);
	      B_next[b] = BS_XOR (B_next[b], ((j % 2) ? in1[b] : in2[b]));
	    }

	  tmp3 = B_next[3];
	  B_next[3] = BS_XOR (B_next[3], BS_AND (BS_XOR (B_next[3], B_next[2]), p));
	  B_next[2] = BS_XOR (B_next[2], BS_AND (BS_XOR (B_next[2], B_next[1]), p));
	  B_next[1] = BS_XOR (B_next[1], BS_AND (BS_XOR (B_next[1], B_next[0]), p));
	  B_next[0] = BS_XOR (B_next[0], BS_AND (BS_XOR (B_next[0], tmp3), p));

	  for (b = 0; b < 4; b++)
	    {
	      for (k = 9; k > 0; k--)
		B[k][b] = B[k - 1][b];

	      B[0][b] = B_next[b];
	    }

	  for (b = 0; b < 4; b++)
	    D[b] = BS_XOR (BS_XOR (E[b], Z[b]), extra_B[b]);

	  for (b = 0; b < 4; b++)
	    next_E[b] = F[b];

	  tmp0 = BS_XOR (Z[0], E[0]);
	  tmp1 = BS_AND (Z[0], E[0]);
	  F[0] = BS_XOR (E[0], BS_AND (q, BS_XOR (Z[0], r)));
	  tmp3 = BS_AND (tmp0, r);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[1], E[1]);
	  tmp1 = BS_AND (Z[1], E[1]);
	  F[1] = BS_XOR (E[1], BS_AND (q, BS_XOR (Z[1], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[2], E[2]);
	  tmp1 = BS_AND (Z[2], E[2]);
	  F[2] = BS_XOR (E[2], BS_AND (q, BS_XOR (Z[2], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[3], E[3]);
	  tmp1 = BS_AND (Z[3], E[3]);
	  F[3] = BS_XOR (E[3], BS_AND (q, BS_XOR (Z[3], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  r = BS_XOR (r, BS_AND (q, BS_XOR (BS_OR (tmp1, tmp3), r)));	// ultimate carry

	  for (b = 0; b < 4; b++)
	    E[b] = next_E[b];

	  X[0] = s1a;
	  X[1] = s2a;
	  X[2] = s3b;
	  X[3] = s4b;
	  Y[0] = s3a;
	  Y[1] = s4a;
	  Y[2] = s5b;
	  Y[3] = s6b;
	  Z[0] = s5a;
	  Z[1] = s6a;
	  Z[2] = s1b;
	  Z[3] = s2b;
	  p = s7a;
	  q = s7b;

	}

    }

  /* Stream GEN */

  for (h = 8; h < maxlen; h++)
    {
      dvbcsa_bs_word_t cb[8];

      for (j = 0; j < 4; j++)
	{
	  dvbcsa_bs_stream_sbox1(A[0][2], A[5][1], A[6][3], A[8][0], A[3][0], &s1a, &s1b);
	  dvbcsa_bs_stream_sbox2(A[2][2], A[5][3], A[6][0], A[8][1], A[1][1], &s2a, &s2b);
	  dvbcsa_bs_stream_sbox3(A[1][0], A[4][1], A[4][3], A[5][2], A[0][3], &s3a, &s3b);
	  dvbcsa_bs_stream_sbox4(A[0][1], A[1][3], A[3][2], A[7][0], A[2][3], &s4a, &s4b);
	  dvbcsa_bs_stream_sbox5(A[3][3], A[5][0], A[7][1], A[8][2], A[4][2], &s5a, &s5b);
	  dvbcsa_bs_stream_sbox6(A[3][1], A[4][0], A[6][2], A[8][3], A[2][1], &s6a, &s6b);
	  dvbcsa_bs_stream_sbox7(A[2][0], A[6][1], A[7][2], A[7][3], A[1][2], &s7a, &s7b);

	  // use 4x4 xor to produce extra nibble for T3

	  extra_B[3] = BS_XOR (BS_XOR (BS_XOR (B[2][0], B[5][1]), B[6][2]), B[8][3]);
	  extra_B[2] = BS_XOR (BS_XOR (BS_XOR (B[5][0], B[7][1]), B[2][3]), B[3][2]);
	  extra_B[1] = BS_XOR (BS_XOR (BS_XOR (B[4][3], B[7][2]), B[3][0]), B[4][1]);
	  extra_B[0] = BS_XOR (BS_XOR (BS_XOR (B[8][2], B[5][3]), B[2][1]), B[7][0]);

	  // T1 = xor all inputs
	  // in1, in2, D are only used in T1 during initialisation, not generation
	  for (b = 0; b < 4; b++)
	    {
	      dvbcsa_bs_word_t	A_next;

	      A_next = BS_XOR (A[9][b], X[b]);

	      for (k = 9; k > 0; k--)
		A[k][b] = A[k - 1][b];

	      A[0][b] = A_next;
	    }

	  // T2 =  xor all inputs
	  // in1, in2 are only used in T1 during initialisation, not generation
	  // if p=0, use this, if p=1, rotate the result left
	  for (b = 0; b < 4; b++)
	    B_next[b] = BS_XOR (BS_XOR (B[6][b], B[9][b]), Y[b]);

	  // if p=1, rotate left (yes, this is what we're doing)
	  tmp3 = B_next[3];
	  B_next[3] = BS_XOR (B_next[3], BS_AND (BS_XOR (B_next[3], B_next[2]), p));
	  B_next[2] = BS_XOR (B_next[2], BS_AND (BS_XOR (B_next[2], B_next[1]), p));
	  B_next[1] = BS_XOR (B_next[1], BS_AND (BS_XOR (B_next[1], B_next[0]), p));
	  B_next[0] = BS_XOR (B_next[0], BS_AND (BS_XOR (B_next[0], tmp3), p));

	  for (b = 0; b < 4; b++)
	    {
	      for (k = 9; k > 0; k--)
		B[k][b] = B[k - 1][b];

	      B[0][b] = B_next[b];
	    }

	  // T3 = xor all inputs
	  for (b = 0; b < 4; b++)
	    D[b] = BS_XOR (BS_XOR (E[b], Z[b]), extra_B[b]);

	  // T4 = sum, carry of Z + E + r
	  for (b = 0; b < 4; b++)
	    next_E[b] = F[b];

	  tmp0 = BS_XOR (Z[0], E[0]);
	  tmp1 = BS_AND (Z[0], E[0]);
	  F[0] = BS_XOR (E[0], BS_AND (q, BS_XOR (Z[0], r)));
	  tmp3 = BS_AND (tmp0, r);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[1], E[1]);
	  tmp1 = BS_AND (Z[1], E[1]);
	  F[1] = BS_XOR (E[1], BS_AND (q, BS_XOR (Z[1], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[2], E[2]);
	  tmp1 = BS_AND (Z[2], E[2]);
	  F[2] = BS_XOR (E[2], BS_AND (q, BS_XOR (Z[2], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  tmp4 = BS_OR (tmp1, tmp3);

	  tmp0 = BS_XOR (Z[3], E[3]);
	  tmp1 = BS_AND (Z[3], E[3]);
	  F[3] = BS_XOR (E[3], BS_AND (q, BS_XOR (Z[3], tmp4)));
	  tmp3 = BS_AND (tmp0, tmp4);
	  r = BS_XOR (r, BS_AND (q, BS_XOR (BS_OR (tmp1, tmp3), r)));	// ultimate carry

	  for (b = 0; b < 4; b++)
	    E[b] = next_E[b];

	  X[0] = s1a;
	  X[1] = s2a;
	  X[2] = s3b;
	  X[3] = s4b;
	  Y[0] = s3a;
	  Y[1] = s4a;
	  Y[2] = s5b;
	  Y[3] = s6b;
	  Z[0] = s5a;
	  Z[1] = s6a;
	  Z[2] = s1b;
	  Z[3] = s2b;

	  p = s7a;
	  q = s7b;

	  // require 4 loops per output byte
	  // 2 output bits are a function of the 4 bits of D
	  // xor 2 by 2
	  cb[7 - 2 * j] = BS_XOR (D[2], D[3]);
	  cb[6 - 2 * j] = BS_XOR (D[0], D[1]);
	}				// EXTERNAL LOOP

      ////////////////////////////////////////////////////////////////////////////////

      dvbcsa_bs_stream_transpose_out(pcks, h, cb);

    }

}

