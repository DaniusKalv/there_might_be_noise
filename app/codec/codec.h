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

#define I2S_AUDIO_BUFFER_SIZE_WORDS 384
#define I2S_AUDIO_BUFFER_SIZE       I2S_AUDIO_BUFFER_SIZE_WORDS * sizeof(uint32_t)

typedef enum
{
	CODEC_MODE_OFF,
	CODEC_MODE_BYPASS,
	CODEC_MODE_I2S
} codec_mode_t;

typedef enum
{
	CODEC_EVENT_TYPE_BUFFER_REQUEST
} codec_event_type_t;

typedef void (* codec_event_handler_t)(codec_event_type_t event_type, uint32_t * p_released_buffer);

ret_code_t codec_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_event_handler_t event_handler);

ret_code_t codec_set_mode(codec_mode_t mode);

ret_code_t codec_start_audio_stream(uint32_t const * p_tx_buffer);

ret_code_t codec_stop_audio_stream(void);

// void * codec_buffer_pointer_get(size_t size);

ret_code_t codec_set_next_buffer(uint32_t const * p_tx_buffer);

#endif // CODEC_H