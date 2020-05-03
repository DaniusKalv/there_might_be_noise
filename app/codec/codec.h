/**
 * @file        codec.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Codec control functions.
 * @version     0.1
 * @date        2020-04-04
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#ifndef CODEC_H
#define CODEC_H

#include "dk_twi_mngr.h"
#include "codec_common.h"

typedef enum
{
	CODEC_EVENT_TYPE_AUDIO_STREAM_STARTED,
	CODEC_EVENT_TYPE_AUDIO_STREAM_STOPPED
} codec_event_type_t;

typedef void (* codec_event_handler_t)(codec_event_type_t event_type);

ret_code_t codec_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_event_handler_t event_handler);

ret_code_t codec_set_mode(codec_mode_t mode);

void * codec_get_rx_buffer(size_t size);

ret_code_t codec_release_rx_buffer(size_t size);

ret_code_t codec_release_unfinished_rx_buffer(void);

void codec_debug(void);

#endif // CODEC_H
