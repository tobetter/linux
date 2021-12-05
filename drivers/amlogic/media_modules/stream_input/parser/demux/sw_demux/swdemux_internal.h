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


#ifndef _SWDEMUX_INTERNAL_H
#define _SWDEMUX_INTERNAL_H

#include "swdemux.h"

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#else
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**Double linked list node.*/
typedef struct SWDMX_List_s SWDMX_List;

/**Double linked list node.*/
struct SWDMX_List_s {
	SWDMX_List *prev; /**< The previous node in the list.*/
	SWDMX_List *next; /**< The next node in the list.*/
};

/**Callback entry.*/
typedef struct {
	SWDMX_List ln;    /**< List node data.*/
	SWDMX_Ptr  cb;    /**< Callback function.*/
	SWDMX_Ptr  data;  /**< User defined data.*/
} SWDMX_CbEntry;

/**TS packet parser.*/
struct SWDMX_TsParser_s {
	SWDMX_Int  packet_size; /**< Packet size.*/
	SWDMX_List cb_list;     /**< Callback list.*/
};

/**Descrambler.*/
struct SWDMX_Descrambler_s {
	SWDMX_List chan_list; /**< Descrambler channel list.*/
	SWDMX_List cb_list;   /**< Callback list.*/
};

/**Descrambler channel.*/
struct SWDMX_DescChannel_s {
	SWDMX_List      ln;     /**< List node data.*/
	SWDMX_Bool      enable; /**< The channel is enabled.*/
	SWDMX_UInt16    pid;    /**< PID of the stream.*/
	SWDMX_DescAlgo *algo;   /**< Descrambler algorithm functions.*/
};

/**Descrambler algorithm.*/
struct SWDMX_DescAlgo_s {
	SWDMX_DescAlgoSetFn  set_fn;  /**< Set parameter function.*/
	SWDMX_DescAlgoDescFn desc_fn; /**< Descramble function.*/
	SWDMX_DescAlgoFreeFn free_fn; /**< Free function.*/
};

/**Demux PID filter.*/
typedef struct {
	SWDMX_List      ln;              /**< List node data.*/
	SWDMX_UInt16    pid;             /**< PID.*/
	SWDMX_List      ts_filter_list;  /**< TS filter list.*/
	SWDMX_List      sec_filter_list; /**< Section filter list.*/
	SWDMX_UInt8    *sec_data;        /**< Section data buffer.*/
	SWDMX_Int       sec_recv;        /**< Section data received*/
} SWDMX_PidFilter;

/**Demux.*/
struct SWDMX_Demux_s {
	SWDMX_List pid_filter_list; /**< PID filter list.*/
	SWDMX_List ts_filter_list;  /**< TS filter list.*/
	SWDMX_List sec_filter_list; /**< Section filter list.*/
};

/**Filter's state.*/
typedef enum {
	SWDMX_FILTER_STATE_INIT, /**< Initialized.*/
	SWDMX_FILTER_STATE_SET,  /**< Parameters has been set.*/
	SWDMX_FILTER_STATE_RUN   /**< Running.*/
} SWDMX_FilterState;

/**TS filter.*/
struct SWDMX_TsFilter_s {
	SWDMX_Demux          *dmx;        /**< The demux contains this filter.*/
	SWDMX_List            ln;         /**< List node data.*/
	SWDMX_List            pid_ln;     /**< List node used in PID filter.*/
	SWDMX_PidFilter      *pid_filter; /**< The PID filter contains this TS filter.*/
	SWDMX_FilterState     state;      /**< State of the filter.*/
	SWDMX_TsFilterParams  params;     /**< Parameters.*/
	SWDMX_List            cb_list;    /**< Callback list.*/
};

/**Section filter.*/
struct SWDMX_SecFilter_s {
	SWDMX_Demux           *dmx;        /**< The demux contains this filter.*/
	SWDMX_List             ln;         /**< List node data.*/
	SWDMX_List             pid_ln;     /**< List node used in PID filter.*/
	SWDMX_PidFilter       *pid_filter; /**< The PID filter contains this section filter.*/
	SWDMX_FilterState      state;      /**< State of the filter.*/
	SWDMX_SecFilterParams  params;     /**< Parameters.*/
	SWDMX_UInt8            value[SWDMX_SEC_FILTER_LEN + 2]; /**< Value array.*/
	SWDMX_UInt8            mam[SWDMX_SEC_FILTER_LEN + 2];   /**< Mask & mode array.*/
	SWDMX_UInt8            manm[SWDMX_SEC_FILTER_LEN + 2];  /**< Mask & ~mode array.*/
	SWDMX_List             cb_list;    /**< Callback list.*/
};

#define SWDMX_MAX(a, b)   ((a) > (b) ? (a) : (b))
#define SWDMX_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define SWDMX_ASSERT(a)   while(!a);//assert(a)
#define SWDMX_PTR2SIZE(p) ((SWDMX_Size)(p))
#define SWDMX_SIZE2PTR(s) ((SWDMX_Ptr)(SWDMX_Size)(s))
#define SWDMX_OFFSETOF(s, m)\
	SWDMX_PTR2SIZE(&((s*)0)->m)
#define SWDMX_CONTAINEROF(p, s, m)\
	((s*)(SWDMX_PTR2SIZE(p) - SWDMX_OFFSETOF(s, m)))

/**
 * Output log message.
 * \param fmt Output format string.
 * \param ... Variable arguments.
 */
#if 0
static inline void
swdmx_log (const char *fmt, ...)
{
#if 0
	va_list ap;

	fprintf(stderr, "SWDMX: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
#endif
}
#else
#define swdmx_log(f, a...) printk("%s:" f, __func__, ## a);
#endif
/**
 * Check if the PID is valid.
 * \param pid The PID.
 * \retval SWDMX_TRUE The PID is valid.
 * \retval SWDMX_FALSE The PID is invalid.
 */
static inline SWDMX_Bool
swdmx_is_valid_pid (SWDMX_UInt16 pid)
{
	return (pid <= 0x1fff);
}

/**
 * Allocate a new memory buffer.
 * \param size The size of the buffer.
 * \return The new buffer's pointer.
 */
static inline SWDMX_Ptr
swdmx_malloc (SWDMX_Size size)
{
	return kmalloc(size,GFP_KERNEL);
}

/**
 * Free an unused buffer.
 * \param ptr The buffer to be freed.
 */
static inline void
swdmx_free (SWDMX_Ptr ptr)
{
	kfree(ptr);
}

/**
 * Check if the list is empty.
 * \param l The list.
 * \retval SWDMX_TRUE The list is empty.
 * \retval SWDMX_FALSE The list is not empty.
 */
static inline SWDMX_Bool
swdmx_list_is_empty (SWDMX_List *l)
{
	return l->next == l;
}

/**
 * Initialize a list node.
 * \param n The node to be initalize.
 */
static inline void
swdmx_list_init (SWDMX_List *n)
{
	n->prev = n;
	n->next = n;
}

/**
 * Append a node to the list.
 * \param l The list.
 * \param n The node to be added.
 */
static inline void
swdmx_list_append (SWDMX_List *l, SWDMX_List *n)
{
	n->prev = l->prev;
	n->next = l;
	l->prev->next = n;
	l->prev = n;
}

/**
 * Remove a node from the list.
 * \param n The node to be removed.
 */
static inline void
swdmx_list_remove (SWDMX_List *n)
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
}

/*Traverse the nodes in the list.*/
#define SWDMX_LIST_FOR_EACH(i, l, m)\
	for ((i) = SWDMX_CONTAINEROF((l)->next, typeof(*i), m);\
				(i) != SWDMX_CONTAINEROF(l, typeof(*i), m);\
				(i) = SWDMX_CONTAINEROF((i)->m.next, typeof(*i), m))

/*Traverse the nodes in the list safely.*/
#define SWDMX_LIST_FOR_EACH_SAFE(i, n, l, m)\
	for ((i) = SWDMX_CONTAINEROF((l)->next, typeof(*i), m),\
				(n) = SWDMX_CONTAINEROF((i)->m.next, typeof(*i), m);\
				(i) != SWDMX_CONTAINEROF(l, typeof(*i), m);\
				(i) = (n),\
				(n) = SWDMX_CONTAINEROF((i)->m.next, typeof(*i), m))

/**
 * Add a callback to the list.
 * \param l The callback list.
 * \param cb The callback function.
 * \param data The user defined parameter.
 */
extern void
swdmx_cb_list_add (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data);

/**
 * Remove a callback from the list.
 * \param l The callback list.
 * \param cb The callback function to be removed.
 * \param data The user defined parameter.
 */
extern void
swdmx_cb_list_remove (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data);

/**
 * Clear the callback list.
 * \param l The callback list to be cleared.
 */
extern void
swdmx_cb_list_clear (SWDMX_List *l);

/**
 * CRC32.
 */
extern SWDMX_UInt32
swdmx_crc32 (const SWDMX_UInt8 *data, SWDMX_Int len);

#ifdef __cplusplus
}
#endif

#endif

