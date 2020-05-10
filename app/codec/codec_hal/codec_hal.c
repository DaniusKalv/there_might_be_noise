/**
 * @file        codec_hal.c
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Codec hardware access layer
 * @version     0.1
 * @date        2020-05-03
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#include "codec_hal.h"
#include "nrf_delay.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "tlv320aic3106.h"
#include "app_timer.h"

#define NRF_LOG_MODULE_NAME codec_hal
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

APP_TIMER_DEF(m_config_timer);

#define CONFIG_TIMER_TIMEOUT APP_TIMER_TICKS(250)

TLV320AIC3106_DEF(m_tlv320aic3106, NULL, DK_BSP_TLV320_I2C_ADDRESS);

static codec_mode_t m_codec_mode;
static codec_hal_evt_handler_t m_evt_handler = NULL;

static void codec_pins_init(void)
{
	nrf_gpio_cfg_output(DK_BSP_TLV320_RST);
	nrf_gpio_pin_clear(DK_BSP_TLV320_RST);
	nrf_delay_ms(100);
	nrf_gpio_pin_set(DK_BSP_TLV320_RST);
}

static ret_code_t codec_clk_init(void)
{
	ret_code_t err_code;

	tlv320aic3106_pll_config_t pll_config;
	memset(&pll_config, 0, sizeof(pll_config));

	pll_config.p      = TLV320AIC3106_PLL_P_1;
	pll_config.j      = 5;
	pll_config.d      = 6448;
	pll_config.r      = 1;

	err_code = tlv320aic3106_pll_init(&m_tlv320aic3106, &pll_config);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_clkin_src(&m_tlv320aic3106, TLV320AIC3106_CODEC_CLKIN_SRC_PLLDIV_OUT);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static ret_code_t codec_dig_if_init(void)
{
	ret_code_t err_code;
	tlv320aic3106_audio_ser_data_interface_ctrl_a_t audio_ser_di_ctrl_a;
	tlv320aic3106_audio_ser_data_interface_ctrl_b_t audio_ser_di_ctrl_b;

	memset(&audio_ser_di_ctrl_a, 0, sizeof(audio_ser_di_ctrl_a));
	memset(&audio_ser_di_ctrl_b, 0, sizeof(audio_ser_di_ctrl_b));

	audio_ser_di_ctrl_a.bclk_dir_output = true;
	audio_ser_di_ctrl_a.wclk_dir_output = true;

	err_code = tlv320aic3106_set_audio_ser_data_interface_ctrl_a(&m_tlv320aic3106, &audio_ser_di_ctrl_a);
	VERIFY_SUCCESS(err_code);

	audio_ser_di_ctrl_b.re_sync_dac            = true;
	audio_ser_di_ctrl_b.re_sync_with_soft_mute = true;

	err_code = tlv320aic3106_set_audio_ser_data_interface_ctrl_b(&m_tlv320aic3106, &audio_ser_di_ctrl_b);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	// tlv320aic3106_audio_codec_digital_filter_ctrl_t dig_filter_ctrl;
	// memset(&dig_filter_ctrl, 0, sizeof(dig_filter_ctrl));

	// dig_filter_ctrl.left_dac_dig_effects_en = true;
	// dig_filter_ctrl.right_dac_dig_effects_en = true;
	// dig_filter_ctrl.left_dac_de_emphasis_en = true;
	// dig_filter_ctrl.right_dac_de_emphasis_en = true;

	// err_code = tlv320aic3106_set_digital_filter_ctrl(&m_tlv320aic3106, &dig_filter_ctrl);
	// VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static ret_code_t codec_dac_init(void)
{
	ret_code_t err_code;

	tlv320aic3106_datapath_setup_t datapath_setup;
	tlv320aic3106_dac_quiescent_current_adj_t dac_quiscient_current;
	tlv320aic3106_dac_out_switch_ctrl_t dac_out_switch_ctrl;
	tlv320aic3106_dac_dig_volume_ctrl_t dac_dig_vol_ctrl;

	memset(&datapath_setup, 0, sizeof(datapath_setup));
	memset(&dac_quiscient_current, 0, sizeof(dac_quiscient_current));
	memset(&dac_out_switch_ctrl, 0, sizeof(dac_out_switch_ctrl));
	memset(&dac_dig_vol_ctrl, 0, sizeof(dac_dig_vol_ctrl));

	datapath_setup.left_dac_datapath_ctrl  = TLV320AIC3106_LEFT_DAC_DATAPATH_CTRL_LEFT_EN;
	datapath_setup.right_dac_datapath_ctrl = TLV320AIC3106_RIGHT_DAC_DATAPATH_CTRL_RIGHT_EN;

	err_code = tlv320aic3106_set_datapath(&m_tlv320aic3106, &datapath_setup);
	VERIFY_SUCCESS(err_code);

	dac_quiscient_current.dac_quiescent_current = TLV320AIC3106_DAC_QUIESCENT_CURRENT_2_DAC_REF;

	err_code = tlv320aic3106_set_dac_quiescient_current(&m_tlv320aic3106, &dac_quiscient_current);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_dac_pwr(&m_tlv320aic3106, false, false);
	VERIFY_SUCCESS(err_code);

	dac_out_switch_ctrl.dac_dig_vol_ctrl     = TLV320AIC3106_DAC_DIG_VOL_CTRL_LEFT_FOLLOWS_RIGHT_CHANNEL;
	dac_out_switch_ctrl.left_dac_out_switch  = TLV320AIC3106_XDAC_OUT_SWITCH_DAC_X1;
	dac_out_switch_ctrl.right_dac_out_switch = TLV320AIC3106_XDAC_OUT_SWITCH_DAC_X1;

	err_code = tlv320aic3106_set_dac_out_switch_ctrl(&m_tlv320aic3106, &dac_out_switch_ctrl);
	VERIFY_SUCCESS(err_code);

	dac_dig_vol_ctrl.dac_muted = true;

	err_code = tlv320aic3106_set_dac_dig_volume_ctrl(&m_tlv320aic3106, &dac_dig_vol_ctrl);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static ret_code_t codec_lop_init(void)
{
	ret_code_t err_code;

	tlv320aic3106_x_to_y_volume_ctrl_t dac_to_lop;
	tlv320aic3106_x_out_lvl_ctrl_t out_lvl_ctrl;

	memset(&dac_to_lop, 0, sizeof(dac_to_lop));
	memset(&out_lvl_ctrl, 0, sizeof(out_lvl_ctrl));

	dac_to_lop.routed_to_y = true;

	err_code = tlv320aic3106_set_dac_x1_to_lop(&m_tlv320aic3106, &dac_to_lop);
	VERIFY_SUCCESS(err_code);

	out_lvl_ctrl.not_muted = false;
	out_lvl_ctrl.power_en  = false;

	err_code = tlv320aic3106_set_lop_m_out_lvl_ctrl(&m_tlv320aic3106, &out_lvl_ctrl);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static ret_code_t codec_bypass_mode_enable(bool bypass)
{
	ret_code_t err_code;

	err_code = tlv320aic3106_set_left_lop_m_out_mute(&m_tlv320aic3106, true);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_right_lop_m_out_mute(&m_tlv320aic3106, true);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_line1_bypass(&m_tlv320aic3106, bypass);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_pll_enable(&m_tlv320aic3106, !bypass);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_dac_pwr(&m_tlv320aic3106, !bypass, !bypass);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_dac_mute(&m_tlv320aic3106, bypass);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_left_lop_m_out_pwr_en(&m_tlv320aic3106, !bypass);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_right_lop_m_out_pwr_en(&m_tlv320aic3106, !bypass);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_left_lop_m_out_mute(&m_tlv320aic3106, bypass);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_right_lop_m_out_mute(&m_tlv320aic3106, bypass);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static bool codec_check_bypass_ready(tlv320aic3106_module_pwr_status_t * p_module_pwr_status)
{
	if(p_module_pwr_status->left_dac_powered_up &&
	   p_module_pwr_status->right_dac_powered_up &&
	   p_module_pwr_status->left_lop_m_powered_up &&
	   p_module_pwr_status->right_lop_m_powered_up)
	{
		return false;
	}

	return true;
}

static void codec_evt_handler(tlv320aic3106_evt_t * p_evt)
{
	switch(p_evt->type)
	{
		case TLV320AIC3106_EVT_TYPE_ERROR:
			NRF_LOG_ERROR("TLV30AIC3106 error %u", p_evt->params.err_code);
			break;
		case TLV320AIC3106_EVT_TYPE_RX_MODULE_PWR_STATUS:
		{
			bool bypass_mode_ready = codec_check_bypass_ready(p_evt->params.p_module_pwr_status);

			if((m_codec_mode == CODEC_MODE_BYPASS) && bypass_mode_ready)
			{
				app_timer_stop(m_config_timer);
				m_evt_handler(CODEC_EVT_TYPE_BYPASS_MODE_READY);
			}
			else if((m_codec_mode == CODEC_MODE_I2S) && !bypass_mode_ready)
			{
				app_timer_stop(m_config_timer);
				m_evt_handler(CODEC_EVT_TYPE_I2S_MODE_READY);
			}
		} break;
		default:
			break;
	}
}

static void codec_config_timer_handler(void * p_context)
{
	tlv320aic3106_get_module_power_status(&m_tlv320aic3106);
}

ret_code_t codec_hal_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_hal_evt_handler_t evt_handler)
{
	ret_code_t err_code;

	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);
	VERIFY_PARAM_NOT_NULL(evt_handler);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;
	m_evt_handler = evt_handler;

	m_codec_mode = CODEC_MODE_BYPASS;

	codec_pins_init();

	err_code = app_timer_create(&m_config_timer, APP_TIMER_MODE_REPEATED, codec_config_timer_handler);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_init(&m_tlv320aic3106, codec_evt_handler);
	VERIFY_SUCCESS(err_code);

	err_code = codec_clk_init();
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_line1_bypass(&m_tlv320aic3106, true);
	VERIFY_SUCCESS(err_code);

	err_code = codec_dig_if_init();
	VERIFY_SUCCESS(err_code);

	err_code = codec_dac_init();
	VERIFY_SUCCESS(err_code);

	err_code = codec_lop_init();
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

ret_code_t codec_hal_mode_set(codec_mode_t mode)
{
	ret_code_t err_code;

	if(m_codec_mode == mode)
	{
		return NRF_SUCCESS;
	}

	switch (mode)
	{
		case CODEC_MODE_BYPASS:
			err_code = codec_bypass_mode_enable(true);
			break;
		case CODEC_MODE_I2S:
			err_code = codec_bypass_mode_enable(false);
			break;
		default:
			return NRF_ERROR_NOT_SUPPORTED;
	}

	VERIFY_SUCCESS(err_code);

	m_codec_mode = mode;

	err_code = app_timer_start(m_config_timer, CONFIG_TIMER_TIMEOUT, NULL);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

ret_code_t codec_hal_mute(bool mute)
{
	ret_code_t err_code;

	err_code = tlv320aic3106_set_left_lop_m_out_mute(&m_tlv320aic3106, mute);
	VERIFY_SUCCESS(err_code);
	
	err_code = tlv320aic3106_set_right_lop_m_out_mute(&m_tlv320aic3106, mute);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

void codec_hal_debug(void)
{
	tlv320aic3106_debug(&m_tlv320aic3106);
}
