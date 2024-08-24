/**
 * @file        codec_buffer.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Buffer implementation for transfering audio data to the codec.
 * @version     0.1
 * @date        2020-04-12
 *
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 *
 */

#include <stddef.h>

#include "sdk_errors.h"

#define CODEC_BUFFER_SIZE_WORDS 256

typedef enum
{
    CODEC_BUFFER_EVENT_TYPE_LOW_WATERMARK_CROSSED_UP
} codec_buffer_event_type_t;

typedef void (*codec_buffer_event_handler_t)(codec_buffer_event_type_t event_type);

ret_code_t codec_buffer_init(codec_buffer_event_handler_t event_handler);

void *codec_buffer_get_rx(size_t size);

ret_code_t codec_buffer_release_rx(size_t size);

ret_code_t codec_buffer_release_rx_unfinished(void);

uint32_t *codec_buffer_get_tx(void);

/**
 * @brief Release last TX buffer.
 */
ret_code_t codec_buffer_release_tx(void);

void codec_buffer_reset(void);
