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

/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Using AltiVec bitslice. */
/* #undef DVBCSA_USE_ALTIVEC */

/* Using MMX bitslice. */
/* #undef DVBCSA_USE_MMX */

/* Using SSE2 bitslice. */
/* #undef DVBCSA_USE_SSE */

#if __SIZEOF_POINTER__ == 4
/* Using 32 bits integer bitslice. */
#define DVBCSA_USE_UINT32  1
#else
/* Using 64 bits integer bitslice. */
#define DVBCSA_USE_UINT64 1
#endif

/* Define to 1 if you have the <assert.h> header file. */
#ifdef __KERNEL__
//#define HAVE_ASSERT_H 0
#else
#define HAVE_ASSERT_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#ifdef __KERNEL__
#define HAVE_INTTYPES_H 0
#else
#define HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* _mm_malloc is available */
#ifdef __KERNEL__
//#define HAVE_MM_MALLOC 0
#else
#define HAVE_MM_MALLOC 1
#endif

/* posix_memalign is available */
#ifdef __KERNEL__
#else
#define HAVE_POSIX_MEMALIGN 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifdef __KERNEL__
#define HAVE_STDINT_H 0
#else
#define HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifdef __KERNEL__
#define HAVE_STDLIB_H 0
#else
#define HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifdef __KERNEL__
#define HAVE_STRINGS_H 0
#else
#define HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libdvbcsa"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libdvbcsa"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libdvbcsa 1.1.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libdvbcsa"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.1.0"

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.1.0"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
