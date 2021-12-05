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

#ifndef _SWDEMUX_H
#define _SWDEMUX_H

#include <linux/types.h>
//#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**Boolean value.*/
typedef uint8_t                          SWDMX_Bool;
/**Boolean value true.*/
#define SWDMX_TRUE  1
/**Boolean value false.*/
#define SWDMX_FALSE 0

/**Function result.*/
typedef int                              SWDMX_Result;
/**Function result: OK.*/
#define SWDMX_OK   1
/**Function result: no error but do nothing.*/
#define SWDMX_NONE 0
/**Function result: error.*/
#define SWDMX_ERR -1

/**Size.*/
typedef size_t                           SWDMX_Size;
/**Integer.*/
typedef int                              SWDMX_Int;
/**Unsigned 8 bits integer.*/
typedef uint8_t                          SWDMX_UInt8;
/**Unsigned 16 bits integer.*/
typedef uint16_t                         SWDMX_UInt16;
/**Unsigned 32 bits integer.*/
typedef uint32_t                         SWDMX_UInt32;
/**Generic pointer.*/
typedef void*                            SWDMX_Ptr;
/**Transport stream parser.*/
typedef struct SWDMX_TsParser_s          SWDMX_TsParser;
/**Descrambler.*/
typedef struct SWDMX_Descrambler_s       SWDMX_Descrambler;
/**Descrambler channel.*/
typedef struct SWDMX_DescChannel_s       SWDMX_DescChannel;
/**Descramble algorithm.*/
typedef struct SWDMX_DescAlgo_s          SWDMX_DescAlgo;
/**TS packet filter.*/
typedef struct SWDMX_TsFilter_s          SWDMX_TsFilter;
/**Section packet filter.*/
typedef struct SWDMX_SecFilter_s         SWDMX_SecFilter;
/**Demux.*/
typedef struct SWDMX_Demux_s             SWDMX_Demux;
/**Transport stream packet.*/
typedef struct SWDMX_TsPacket_s          SWDMX_TsPacket;
/**TS packet filter's parameters.*/
typedef struct SWDMX_TsFilterParams_s    SWDMX_TsFilterParams;
/**Section filter's parameters.*/
typedef struct SWDMX_SecFilterParams_s   SWDMX_SecFilterParams;

/**Descramble algorithm's set parameter function.*/
typedef SWDMX_Result (*SWDMX_DescAlgoSetFn) (
			SWDMX_DescAlgo *algo,
			SWDMX_Int       type,
			SWDMX_Ptr       param
			);
/**Descramble function.*/
typedef SWDMX_Result (*SWDMX_DescAlgoDescFn) (
			SWDMX_DescAlgo *algo,
			SWDMX_TsPacket *pkt
			);
/**Descramble algorithm data free function.*/
typedef void (*SWDMX_DescAlgoFreeFn) (
			SWDMX_DescAlgo *algo
			);
/**TS packet callback function.*/
typedef void (*SWDMX_TsPacketCb) (
			SWDMX_TsPacket *pkt,
			SWDMX_Ptr       udata);
/**Section data callback function.*/
typedef void (*SWDMX_SecCb) (
			SWDMX_UInt8  *data,
			SWDMX_Int     len,
			SWDMX_Ptr     udata);

/**Length of the section filter.*/
#define SWDMX_SEC_FILTER_LEN 16

/**TS packet filter's parameters.*/
struct SWDMX_TsFilterParams_s {
	SWDMX_UInt16 pid; /**< PID of the stream.*/
};

/**Section filter's parameters.*/
struct SWDMX_SecFilterParams_s {
	SWDMX_UInt16 pid;                         /**< PID of the section.*/
	SWDMX_Bool   crc32;                       /**< CRC32 check.*/
	SWDMX_UInt8  value[SWDMX_SEC_FILTER_LEN]; /**< Value array.*/
	SWDMX_UInt8  mask[SWDMX_SEC_FILTER_LEN];  /**< Mask array.*/
	SWDMX_UInt8  mode[SWDMX_SEC_FILTER_LEN];  /**< Match mode array.*/
};

/**TS packet.*/
struct SWDMX_TsPacket_s {
	SWDMX_UInt16  pid;           /**< PID.*/
	SWDMX_Bool    payload_start; /**< Payload start flag.*/
	SWDMX_Bool    priority;      /**< TS packet priority.*/
	SWDMX_Bool    error;         /**< Error flag.*/
	SWDMX_UInt8   scramble;      /**< Scramble flag.*/
	SWDMX_UInt8   cc;            /**< Continuous counter.*/
	SWDMX_UInt8  *packet;        /**< Packet buffer.*/
	SWDMX_Int     packet_len;    /**< Packet length.*/
	SWDMX_UInt8  *adp_field;     /**< Adaptation field buffer.*/
	SWDMX_Int     adp_field_len; /**< Adaptation field length.*/
	SWDMX_UInt8  *payload;       /**< Payload buffer.*/
	SWDMX_Int     payload_len;   /**< Payload length.*/
};

/**DVBCSA2 parameter.*/
enum {
	SWDMX_DVBCSA2_PARAM_ODD_KEY,  /**< 8 bytes odd key.*/
	SWDMX_DVBCSA2_PARAM_EVEN_KEY  /**< 8 bytes even key.*/
};

/**Scrambled data alignment.*/
enum {
	SWDMX_DESC_ALIGN_HEAD, /**< Align to payload head.*/
	SWDMX_DESC_ALIGN_TAIL  /**< Align to payload tail.*/
};

/**AES ECB parameter.*/
enum {
	SWDMX_AES_ECB_PARAM_ODD_KEY,  /**< 16 bytes odd key.*/
	SWDMX_AES_ECB_PARAM_EVEN_KEY, /**< 16 bytes even key.*/
	SWDMX_AES_ECB_PARAM_ALIGN     /**< Alignment.*/
};

/**AES CBC parameter.*/
enum {
	SWDMX_AES_CBC_PARAM_ODD_KEY,  /**< 16 bytes odd key.*/
	SWDMX_AES_CBC_PARAM_EVEN_KEY, /**< 16 bytes even key.*/
	SWDMX_AES_CBC_PARAM_ALIGN,    /**< Align to head.*/
	SWDMX_AES_CBC_PARAM_ODD_IV,   /**< 16 bytes odd key's IV data.*/
	SWDMX_AES_CBC_PARAM_EVEN_IV   /**< 16 bytes even key's ID data.*/
};

/**
 * Create a new TS packet parser.
 * \return The new TS parser.
 */
extern SWDMX_TsParser*
swdmx_ts_parser_new (void);

/**
 * Set the TS packet size.
 * \param tsp The TS parser.
 * \param size The packet size.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_parser_set_packet_size (
			SWDMX_TsParser *tsp,
			SWDMX_Int       size);

/**
 * Add a TS packet callback function to the TS parser.
 * \param tsp The TS parser.
 * \param cb The callback function.
 * \param data The user defined data used as the callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_parser_add_ts_packet_cb (
			SWDMX_TsParser   *tsp,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data);

/**
 * Remove a TS packet callback function from the TS parser.
 * \param tsp The TS parser.
 * \param cb The callback function.
 * \param data The user defined data used as the callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_parser_remove_ts_packet_cb (
			SWDMX_TsParser   *tsp,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data);

/**
 * Parse TS data.
 * \param tsp The TS parser.
 * \param data Input TS data.
 * \param len Input data length in bytes.
 * \return Parse data length in bytes.
 */
extern SWDMX_Int
swdmx_ts_parser_run (
			SWDMX_TsParser *tsp,
			SWDMX_UInt8    *data,
			SWDMX_Int       len);

/**
 * Free an unused TS parser.
 * \param tsp The TS parser to be freed.
 */
extern void
swdmx_ts_parser_free (SWDMX_TsParser *tsp);

/**
 * Create a new descrambler.
 * \return The new descrambler.
 */
extern SWDMX_Descrambler*
swdmx_descrambler_new (void);

/**
 * Allocate a channel from the descrambler.
 * \param desc The descrambler.
 * \return The new channel.
 */
extern SWDMX_DescChannel*
swdmx_descrambler_alloc_channel (SWDMX_Descrambler *desc);

/**
 * The TS packet input function of the descrambler.
 * \param pkt The input packet.
 * \param desc The descrambler.
 */
extern void
swdmx_descrambler_ts_packet_cb (
			SWDMX_TsPacket *pkt,
			SWDMX_Ptr       desc);

/**
 * Add a TS packet callback to the descrambler.
 * \param desc The descrambler.
 * \param cb The callback function.
 * \param data The user defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_descrambler_add_ts_packet_cb (
			SWDMX_Descrambler *desc,
			SWDMX_TsPacketCb   cb,
			SWDMX_Ptr          data);

/**
 * Remove a TS packet callback from the descrambler.
 * \param desc The descrambler.
 * \param cb The callback function.
 * \param data The user defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_descrambler_remove_ts_packet_cb (
			SWDMX_Descrambler *desc,
			SWDMX_TsPacketCb   cb,
			SWDMX_Ptr          data);

/**
 * Free an unused descrambler.
 * \param desc The descrambler to be freed.
 */
extern void
swdmx_descrambler_free (SWDMX_Descrambler *desc);

/**
 * Set the descrambler channel's algorithm.
 * \param chan The descrambler channel.
 * \param algo The algorithm data.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_desc_channel_set_algo (
			SWDMX_DescChannel *chan,
			SWDMX_DescAlgo    *algo);

/**
 * Set the descrambler channel's related PID.
 * \param chan The descrambler channel.
 * \param pid The PID.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_desc_channel_set_pid (
			SWDMX_DescChannel *chan,
			SWDMX_UInt16       pid);

/**
 * Set the parameter of a descrambler channel.
 * \param chan The descrambler channel.
 * \param type Parameter type.
 * \param param Parameter value.
 */
extern SWDMX_Result
swdmx_desc_channel_set_param (
			SWDMX_DescChannel *chan,
			SWDMX_Int          type,
			SWDMX_Ptr          param);

/**
 * Enable a descrambler channel.
 * \param chan The descrambler channel.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_desc_channel_enable (SWDMX_DescChannel *chan);

/**
 * Disable a descrambler channel.
 * \param chan The descrambler channel.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_desc_channel_disable (SWDMX_DescChannel *chan);

/**
 * Free an unused descrambler channel.
 * \param chan The descrambler channel to be freed.
 */
extern void
swdmx_desc_channel_free (SWDMX_DescChannel *chan);

/**
 * Create a new demux.
 * \return The new demux.
 */
extern SWDMX_Demux*
swdmx_demux_new (void);

/**
 * Allocate a new TS packet filter from the demux.
 * \param dmx The demux.
 * \return The new TS packet filter.
 */
extern SWDMX_TsFilter*
swdmx_demux_alloc_ts_filter (SWDMX_Demux *dmx);

/**
 * Allocate a new section filter from the demux.
 * \param dmx The demux.
 * \return The new section filter.
 */
extern SWDMX_SecFilter*
swdmx_demux_alloc_sec_filter (SWDMX_Demux *dmx);

/**
 * TS packet input function of the demux.
 * \param pkt Input TS packet.
 * \param dmx The demux.
 */
extern void
swdmx_demux_ts_packet_cb (
			SWDMX_TsPacket *pkt,
			SWDMX_Ptr       dmx);

/**
 * Free an unused demux.
 * \param dmx The demux to be freed.
 */
extern void
swdmx_demux_free (SWDMX_Demux *dmx);

/**
 * Set the TS filter's parameters.
 * \param filter The filter.
 * \param params Parameters of the filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_filter_set_params (
			SWDMX_TsFilter       *filter,
			SWDMX_TsFilterParams *params);

/**
 * Add a TS packet callback to the TS filter.
 * \param filter The TS filter.
 * \param cb The callback function.
 * \param data User defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_filter_add_ts_packet_cb (
			SWDMX_TsFilter   *filter,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data);

/**
 * Remove a TS packet callback from the TS filter.
 * \param filter The TS filter.
 * \param cb The callback function.
 * \param data User defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_filter_remove_ts_packet_cb (
			SWDMX_TsFilter   *filter,
			SWDMX_TsPacketCb  cb,
			SWDMX_Ptr         data);

/**
 * Enable the TS filter.
 * \param filter The TS filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_filter_enable (SWDMX_TsFilter *filter);

/**
 * Disable the TS filter.
 * \param filter The filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_ts_filter_disable (SWDMX_TsFilter *filter);

/**
 * Free an unused TS filter.
 * \param filter The ts filter to be freed.
 */
extern void
swdmx_ts_filter_free (SWDMX_TsFilter *filter);

/**
 * Set the section filter's parameters.
 * \param filter The section filter.
 * \param params Parameters of the filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_sec_filter_set_params (
			SWDMX_SecFilter       *filter,
			SWDMX_SecFilterParams *params);

/**
 * Add a section callback to the section filter.
 * \param filter The section filter.
 * \param cb The callback function.
 * \param data User defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_sec_filter_add_section_cb (
			SWDMX_SecFilter *filter,
			SWDMX_SecCb      cb,
			SWDMX_Ptr        data);

/**
 * Remove a section callback from the section filter.
 * \param filter The section filter.
 * \param cb The callback function.
 * \param data User defined data used as callback's parameter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_sec_filter_remove_section_cb (
			SWDMX_SecFilter *filter,
			SWDMX_SecCb      cb,
			SWDMX_Ptr        data);

/**
 * Enable the section filter.
 * \param filter The section filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_sec_filter_enable (SWDMX_SecFilter *filter);

/**
 * Disable the section filter.
 * \param filter The section filter.
 * \retval SWDMX_OK On success.
 * \retval SWDMX_ERR On error.
 */
extern SWDMX_Result
swdmx_sec_filter_disable (SWDMX_SecFilter *filter);

/**
 * Free an unused section filter.
 * \param filter The section filter to be freed.
 */
extern void
swdmx_sec_filter_free (SWDMX_SecFilter *filter);

/**
 * Create a new DVB-CSA2 descramble algorithm data.
 * \return The new descramble algorithm data.
 */
extern SWDMX_DescAlgo*
swdmx_dvbcsa2_algo_new (void);

/**
 * Create a new AES ECB descramble algorithm data.
 * \return The new descramble algorithm data.
 */
extern SWDMX_DescAlgo*
swdmx_aes_ecb_algo_new (void);

/**
 * Create a new AES CBC descramble algorithm data.
 * \return The new descramble algorithm data.
 */
extern SWDMX_DescAlgo*
swdmx_aes_cbc_algo_new (void);

#ifdef __cplusplus
}
#endif

#endif

