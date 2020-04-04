/**
 * @file        codec.c
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Codec control functions.
 * @version     0.1
 * @date        2020-04-04
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#include "codec.h"
#include "boards.h"
#include "tlv320aic3106.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

TLV320AIC3106_DEF(m_tlv320aic3106, NULL, DK_BSP_TLV320_I2C_ADDRESS);

static void codec_pins_init(void)
{
	nrf_gpio_cfg_output(DK_BSP_TLV320_RST);
	nrf_gpio_pin_clear(DK_BSP_TLV320_RST);
	nrf_delay_ms(100);
	nrf_gpio_pin_set(DK_BSP_TLV320_RST);
}

ret_code_t codec_init(dk_twi_mngr_t * p_dk_twi_mngr)
{
	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;

	codec_pins_init();

	return tlv320aic3106_init(&m_tlv320aic3106, NULL);;
}
