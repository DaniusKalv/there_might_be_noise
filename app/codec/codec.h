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

typedef enum
{
	CODEC_MODE_OFF,
	CODEC_MODE_BYPASS,
	CODEC_MODE_I2S
} codec_mode_t;

ret_code_t codec_init(dk_twi_mngr_t * p_dk_twi_mngr);

ret_code_t codec_set_mode(codec_mode_t mode);

#endif // CODEC_H