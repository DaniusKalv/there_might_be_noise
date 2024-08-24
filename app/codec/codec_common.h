/**
 * @file        codec_common.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Common codec defines and types.
 * @version     0.1
 * @date        2020-05-03
 *
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 *
 */

#ifndef CODEC_COMMON_H
#define CODEC_COMMON_H

typedef enum
{
    CODEC_MODE_OFF,
    CODEC_MODE_BYPASS,
    CODEC_MODE_I2S
} codec_mode_t;

typedef enum
{
    CODEC_EVT_TYPE_BYPASS_MODE_READY,
    CODEC_EVT_TYPE_I2S_MODE_READY,
    CODEC_EVT_TYPE_AUDIO_STREAM_STARTED,
    CODEC_EVT_TYPE_AUDIO_STREAM_STOPPED
} codec_evt_type_t;

#endif // CODEC_COMMON_H
