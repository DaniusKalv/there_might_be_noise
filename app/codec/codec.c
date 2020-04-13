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
#include "nrfx_i2s.h"
#include "codec_buffer.h"

#define NRF_LOG_MODULE_NAME codec
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

TLV320AIC3106_DEF(m_tlv320aic3106, NULL, DK_BSP_TLV320_I2C_ADDRESS);

static codec_event_handler_t m_event_handler = NULL;
static bool m_streaming_audio;

static void codec_pins_init(void)
{
	nrf_gpio_cfg_output(DK_BSP_TLV320_RST);
	nrf_gpio_pin_clear(DK_BSP_TLV320_RST);
	nrf_delay_ms(100);
	nrf_gpio_pin_set(DK_BSP_TLV320_RST);
}

static void i2s_data_handler(nrfx_i2s_buffers_t const * p_released,
                             uint32_t                   status)
{
	VERIFY_PARAM_NOT_NULL_VOID(p_released);
	ret_code_t err_code;

	if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)) // This will get called two times. For each buffer release.
	{
		if(m_streaming_audio == false)
		{
			if(m_event_handler != NULL)
			{
				m_event_handler(CODEC_EVENT_TYPE_AUDIO_STREAM_STOPPED);
			}
			codec_buffer_reset();
		}

		m_streaming_audio = false;
		return;
	}

	if(p_released->p_tx_buffer == NULL)
	{
		if(m_event_handler != NULL)
		{
			m_streaming_audio = true;
			m_event_handler(CODEC_EVENT_TYPE_AUDIO_STREAM_STARTED);
		}
	}

	uint32_t * p_buffer = codec_buffer_get_tx();

	if(p_buffer != NULL)
	{
		nrfx_i2s_buffers_t next_buffers =
		{
			.p_tx_buffer = p_buffer,
			.p_rx_buffer = NULL
		};

		err_code = nrfx_i2s_next_buffers_set(&next_buffers);
		VERIFY_SUCCESS_VOID(err_code);
	}
	else
	{
		// Audio buffer queue empty, stop
		nrfx_i2s_stop();
	}
}

static ret_code_t codec_start_audio_stream(void)
{
	NRF_LOG_INFO("Starting audio stream");

	uint32_t * p_tx_buffer = codec_buffer_get_tx();

	if(p_tx_buffer == NULL)
	{
		return NRF_ERROR_NOT_FOUND;
	}

	nrfx_i2s_buffers_t initial_buffers =
	{
		.p_tx_buffer = p_tx_buffer,
		.p_rx_buffer = NULL
	};

	return nrfx_i2s_start(&initial_buffers, CODEC_BUFFER_SIZE_WORDS, 0);
}

static void codec_buffer_event_handler(codec_buffer_event_type_t event_type)
{
	ret_code_t err_code;

	switch (event_type)
	{
		case CODEC_BUFFER_EVENT_TYPE_LOW_WATERMARK_CROSSED_UP:
			if(!m_streaming_audio)
			{
				NRF_LOG_INFO("Codec buffer watermark crossed.");
				err_code = codec_start_audio_stream();

				if(err_code != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("Could not start audio stream");
				}
			}
			break;
		default:
			break;
	}
}

static ret_code_t i2s_init(void)
{
	nrfx_i2s_config_t config = NRFX_I2S_DEFAULT_CONFIG;

	config.sck_pin   = DK_BSP_I2S_BCLK;
	config.lrck_pin  = DK_BSP_I2S_WCLK;
	config.mck_pin   = DK_BSP_I2S_MCLK;
	config.sdout_pin = DK_BSP_I2S_DOUT;
	config.sdin_pin  = DK_BSP_I2S_DIN;

	return nrfx_i2s_init(&config, i2s_data_handler);
}

ret_code_t codec_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_event_handler_t event_handler)
{
	ret_code_t err_code;

	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);
	VERIFY_PARAM_NOT_NULL(event_handler);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;
	m_event_handler = event_handler;
	m_streaming_audio = false;

	codec_pins_init();

	err_code = codec_buffer_init(codec_buffer_event_handler);
	VERIFY_SUCCESS(err_code);

	err_code = i2s_init();
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_init(&m_tlv320aic3106, NULL);;
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_passive_ana_sig_bypass_sel_pd_t bypass;
	memset(&bypass, 0, sizeof(bypass));

	// bypass.line1lp_routed_to_left_lop  = true;
	// bypass.line1lm_routed_to_left_lom  = true;
	// bypass.line1rp_routed_to_right_lop = true;
	// bypass.line1rm_routed_to_right_lom = true;

	err_code = tlv320aic3106_set_passive_ana_sig_bypass_sel_pd(&m_tlv320aic3106, &bypass);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_pll_prog_reg_a_t pll_prog_reg_a;
	memset(&pll_prog_reg_a, 0, sizeof(pll_prog_reg_a));

	pll_prog_reg_a.pll_enabled = true;
	pll_prog_reg_a.pll_p       = TLV320AIC3106_PLL_P_1;

	err_code = tlv320aic3106_set_pll_prog_reg_a(&m_tlv320aic3106, &pll_prog_reg_a);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_pll_prog_reg_b_t pll_prog_reg_b;
	memset(&pll_prog_reg_b, 0, sizeof(pll_prog_reg_b));

	pll_prog_reg_b.pll_j = 5; // TODO: Create a define

	err_code = tlv320aic3106_set_pll_prog_reg_b(&m_tlv320aic3106, &pll_prog_reg_b);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	err_code = tlv320aic3106_set_pll_d(&m_tlv320aic3106, 6448); // TODO: define
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	err_code = tlv320aic3106_set_pll_r(&m_tlv320aic3106, 1); // TODO: define
	VERIFY_SUCCESS(err_code);

//21 - 26
//47 - 52

//12
/*----------------------------------------------------------------------------*/
	tlv320aic3106_datapath_setup_t datapath_setup;
	memset(&datapath_setup, 0, sizeof(datapath_setup));

	datapath_setup.fsref_setting           = TLV320AIC3106_FSREF_SETTING_44_1KHZ;
	datapath_setup.left_dac_datapath_ctrl  = TLV320AIC3106_LEFT_DAC_DATAPATH_CTRL_LEFT_EN;
	datapath_setup.right_dac_datapath_ctrl = TLV320AIC3106_RIGHT_DAC_DATAPATH_CTRL_RIGHT_EN;

	err_code = tlv320aic3106_set_datapath(&m_tlv320aic3106, &datapath_setup);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_audio_ser_data_interface_ctrl_a_t audio_ser_di_ctrl_a;
	memset(&audio_ser_di_ctrl_a, 0, sizeof(audio_ser_di_ctrl_a));

	audio_ser_di_ctrl_a.bclk_dir_output = true;
	audio_ser_di_ctrl_a.wclk_dir_output = true;

	err_code = tlv320aic3106_set_audio_ser_data_interface_ctrl_a(&m_tlv320aic3106, &audio_ser_di_ctrl_a);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_audio_ser_data_interface_ctrl_b_t audio_ser_di_ctrl_b;
	memset(&audio_ser_di_ctrl_b, 0, sizeof(audio_ser_di_ctrl_b));

	audio_ser_di_ctrl_b.bclk_256_mode          = false;
	audio_ser_di_ctrl_b.re_sync_dac            = true;
	audio_ser_di_ctrl_b.re_sync_with_soft_mute = true;

	err_code = tlv320aic3106_set_audio_ser_data_interface_ctrl_b(&m_tlv320aic3106, &audio_ser_di_ctrl_b);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_ac_pwr_and_out_drv_ctrl_t ac_pwr_and_out_drv_ctrl;
	memset(&ac_pwr_and_out_drv_ctrl, 0, sizeof(ac_pwr_and_out_drv_ctrl));

	ac_pwr_and_out_drv_ctrl.left_dac_powered_up  = true;
	ac_pwr_and_out_drv_ctrl.right_dac_powered_up = true;

	err_code = tlv320aic3106_set_ac_pwr_and_out_drv_ctrl(&m_tlv320aic3106, &ac_pwr_and_out_drv_ctrl);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_dac_out_switch_ctrl_t dac_out_switch_ctrl;
	memset(&dac_out_switch_ctrl, 0, sizeof(dac_out_switch_ctrl));

	dac_out_switch_ctrl.left_dac_out_switch  = TLV320AIC3106_XDAC_OUT_SWITCH_DAC_X3;
	dac_out_switch_ctrl.right_dac_out_switch = TLV320AIC3106_XDAC_OUT_SWITCH_DAC_X3;

	err_code = tlv320aic3106_set_dac_out_switch_ctrl(&m_tlv320aic3106, &dac_out_switch_ctrl);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_dac_dig_volume_ctrl_t dac_dig_vol_ctrl;
	memset(&dac_dig_vol_ctrl, 0, sizeof(dac_dig_vol_ctrl));

	dac_dig_vol_ctrl.dac_muted = false;

	err_code = tlv320aic3106_set_left_dac_dig_volume_ctrl(&m_tlv320aic3106, &dac_dig_vol_ctrl);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_right_dac_dig_volume_ctrl(&m_tlv320aic3106, &dac_dig_vol_ctrl);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_x_out_lvl_ctrl_t out_lvl_ctrl;
	memset(&out_lvl_ctrl, 0, sizeof(out_lvl_ctrl));

	out_lvl_ctrl.not_muted = true;

	err_code = tlv320aic3106_set_left_lop_m_out_lvl_ctrl(&m_tlv320aic3106, &out_lvl_ctrl);
	VERIFY_SUCCESS(err_code);

	err_code = tlv320aic3106_set_right_lop_m_out_lvl_ctrl(&m_tlv320aic3106, &out_lvl_ctrl);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_gpio_ctrl_b_t gpio_ctrl_b;
	memset(&gpio_ctrl_b, 0, sizeof(gpio_ctrl_b));

	gpio_ctrl_b.codec_clkin_src = TLV320AIC3106_CODEC_CLKIN_SRC_PLLDIV_OUT;

	err_code = tlv320aic3106_set_gpio_ctrl_b(&m_tlv320aic3106, &gpio_ctrl_b);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	// tlv320aic3106_clk_gen_ctrl_t clk_gen_ctrl;
	// memset(&clk_gen_ctrl, 0, sizeof(clk_gen_ctrl));

	// clk_gen_ctrl.pllclk_in_src = TLV320AIC3106_CLK_IN_SRC_MCLK;

	// err_code = tlv320aic3106_set_clk_gen_ctrl(&m_tlv320aic3106, &clk_gen_ctrl);
	// VERIFY_SUCCESS(err_code);
/*----------------------------------------------------------------------------*/

	// while(true)
	// {
	// 	while(NRF_LOG_PROCESS());
	// 	__WFE();
	// 	// Clear the event register.
	// 	__SEV();
	// 	__WFE();
	// }

	return NRF_SUCCESS;
	// return codec_set_mode(CODEC_MODE_BYPASS);
}

// ret_code_t codec_set_mode(codec_mode_t mode)
// {
// 	ret_code_t err_code;

// 	switch(mode)
// 	{
// 		case CODEC_MODE_OFF:
// 			return NRF_ERROR_NOT_SUPPORTED;
// 			break;
// 		case CODEC_MODE_BYPASS:
// 			return NRF_SUCCESS;
// 			break;
// 		case CODEC_MODE_I2S:
// 			break;
// 		default:
// 			return NRF_ERROR_NOT_SUPPORTED;
// 			break;
// 	}

// 	return NRF_SUCCESS;
// }

ret_code_t codec_set_next_buffer(uint32_t const * p_tx_buffer)
{
	nrfx_i2s_buffers_t next_buffers =
	{
		.p_tx_buffer = p_tx_buffer,
		.p_rx_buffer = NULL
	};

	return nrfx_i2s_next_buffers_set(&next_buffers);
}

void * codec_get_rx_buffer(size_t size)
{
	return codec_buffer_get_rx(size);
}

ret_code_t codec_release_rx_buffer(size_t size)
{
	return codec_buffer_release_rx(size);
}

ret_code_t codec_release_unfinished_rx_buffer(void)
{
	return codec_buffer_release_rx_unfinished();
}
