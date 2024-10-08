/**
 * @file        codec_hal.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Codec hardware access layer
 * @version     0.1
 * @date        2020-05-03
 *
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 *
 */

#ifndef CODEC_HAL_H
#define CODEC_HAL_H

#include "codec_common.h"
#include "dk_twi_mngr.h"

typedef void (*codec_hal_evt_handler_t)(codec_evt_type_t event_type);

ret_code_t codec_hal_init(dk_twi_mngr_t const *p_dk_twi_mngr, codec_hal_evt_handler_t evt_handler);

ret_code_t codec_hal_mode_set(codec_mode_t mode);

ret_code_t codec_hal_mute(bool mute);

void codec_hal_debug(void);

#endif // CODEC_HAL_H
