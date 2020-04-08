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

#define NRF_LOG_MODULE_NAME codec
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

TLV320AIC3106_DEF(m_tlv320aic3106, NULL, DK_BSP_TLV320_I2C_ADDRESS);

// #define I2S_DATA_BLOCK_WORDS    512
// static uint32_t m_buffer_tx[2][I2S_DATA_BLOCK_WORDS] = { 0 };

// static int16_t m_test_data[2][32] =
// {
// {-16384, -13086, -9923,  -7024,  -4509,  -2480,  -1020,  -189, 
//   -21,    -523,   -1674,  -3428,  -5712,  -8433,  -11479, -14726, 
//      -18042, -21289, -24335, -27056, -29340, -31094, -32245, -32747, 
//      -32579, -31748, -30288, -28259, -25744, -22845, -19682, -16384},
// {-16384, -13086, -9923,  -7024,  -4509,  -2480,  -1020,  -189, 
//   -21,    -523,   -1674,  -3428,  -5712,  -8433,  -11479, -14726, 
//      -18042, -21289, -24335, -27056, -29340, -31094, -32245, -32747, 
//      -32579, -31748, -30288, -28259, -25744, -22845, -19682, -16384}
// };

// static uint8_t m_active_buffer = 0;
// static uint16_t m_buffer_index[2] = { 0 };
static codec_event_handler_t m_event_handler = NULL;

#define BUFFER_SIZE sizeof(m_buffer_tx[0])

// Delay time between consecutive I2S transfers performed in the main loop
// (in milliseconds).
#define PAUSE_TIME          500
// Number of blocks of data to be contained in each transfer.
#define BLOCKS_TO_TRANSFER  20

// static uint8_t volatile m_blocks_transferred     = 0;
// static uint8_t          m_zero_samples_to_ignore = 0;
// static uint16_t         m_sample_value_to_send;
// static uint16_t         m_sample_value_expected;
// static bool             m_error_encountered;

// static uint32_t       * volatile mp_block_to_fill  = NULL;
// static uint32_t const * volatile mp_block_to_check = NULL;

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
	// 'nrf_drv_i2s_next_buffers_set' is called directly from the handler
	// each time next buffers are requested, so data corruption is not
	// expected.
	ASSERT(p_released);

	// When the handler is called after the transfer has been stopped
	// (no next buffers are needed, only the used buffers are to be
	// released), there is nothing to do.
	if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED))
	{
		return;
	}

	// First call of this handler occurs right after the transfer is started.
	// No data has been transferred yet at this point, so there is nothing to
	// check. Only the buffers for the next part of the transfer should be
	// provided.

	if(m_event_handler != NULL)
	{
		m_event_handler(CODEC_EVENT_TYPE_BUFFER_REQUEST, p_released->p_tx_buffer);
	}

	// if (!p_released->p_tx_buffer)
	// {
	// 	m_active_buffer = 0;
	// 	nrfx_i2s_buffers_t const next_buffers = {
	// 		.p_rx_buffer = NULL,
	// 		.p_tx_buffer = m_buffer_tx[1],
	// 	};
	// 	APP_ERROR_CHECK(nrfx_i2s_next_buffers_set(&next_buffers));
	// }
	// else
	// {
	// 	m_active_buffer = !m_active_buffer;

	// 	nrfx_i2s_buffers_t next_buffers = {
	// 		.p_rx_buffer = NULL,
	// 		.p_tx_buffer = m_buffer_tx[m_active_buffer]
	// 	};

		// The driver has just finished accessing the buffers pointed by
		// 'p_released'. They can be used for the next part of the transfer
		// that will be scheduled now.
		// APP_ERROR_CHECK(nrfx_i2s_next_buffers_set(&next_buffers));

		// The pointer needs to be typecasted here, so that it is possible to
		// modify the content it is pointing to (it is marked in the structure
		// as pointing to constant data because the driver is not supposed to
		// modify the provided data).
	// }
}

static ret_code_t i2s_init(void)
{
	nrfx_i2s_config_t config = NRFX_I2S_DEFAULT_CONFIG;

	config.sck_pin   = DK_BSP_I2S_BCLK;
	config.lrck_pin  = DK_BSP_I2S_WCLK;
	config.mck_pin   = DK_BSP_I2S_MCLK;
	config.sdout_pin = DK_BSP_I2S_DIN;
	config.sdin_pin  = DK_BSP_I2S_DOUT;

	return nrfx_i2s_init(&config, i2s_data_handler);
}

ret_code_t codec_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_event_handler_t event_handler)
{
	ret_code_t err_code;

	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);
	VERIFY_PARAM_NOT_NULL(event_handler);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;
	m_event_handler = event_handler;

	codec_pins_init();

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

	pll_prog_reg_a.pll_q = TLV320AIC3106_PLL_Q_2;

	err_code = tlv320aic3106_set_pll_prog_reg_a(&m_tlv320aic3106, &pll_prog_reg_a);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_datapath_setup_t datapath_setup;
	memset(&datapath_setup, 0, sizeof(datapath_setup));

	datapath_setup.left_dac_datapath_ctrl  = TLV320AIC3106_LEFT_DAC_DATAPATH_CTRL_LEFT_EN;
	datapath_setup.right_dac_datapath_ctrl = TLV320AIC3106_RIGHT_DAC_DATAPATH_CTRL_RIGHT_EN;

	err_code = tlv320aic3106_set_datapath(&m_tlv320aic3106, &datapath_setup);
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

	gpio_ctrl_b.codec_clkin_src = TLV320AIC3106_CODEC_CLKIN_SRC_CLKDIV_OUT;

	err_code = tlv320aic3106_set_gpio_ctrl_b(&m_tlv320aic3106, &gpio_ctrl_b);
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_clk_gen_ctrl_t clk_gen_ctrl;
	memset(&clk_gen_ctrl, 0, sizeof(clk_gen_ctrl));

	clk_gen_ctrl.clkdiv_in_src = TLV320AIC3106_CLK_IN_SRC_BCLK;

	err_code = tlv320aic3106_set_clk_gen_ctrl(&m_tlv320aic3106, &clk_gen_ctrl);
	VERIFY_SUCCESS(err_code);
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

// void * codec_buffer_pointer_get(size_t size)
// {
// 	uint8_t buffer_index = !m_active_buffer;
// 	void * p_buff = (void *)m_buffer_tx[buffer_index];

// 	return p_buff;
// }

ret_code_t codec_start_audio_stream(uint32_t * p_tx_buffer)
{
	NRF_LOG_INFO("Starting audio stream");

	nrfx_i2s_buffers_t initial_buffers = {
		.p_tx_buffer = p_tx_buffer,
		.p_rx_buffer = NULL
	};

	return nrfx_i2s_start(&initial_buffers, I2S_ADIO_BUFFER_SIZE_WORDS, 0);
}

ret_code_t codec_stop_audio_stream(void)
{
	NRF_LOG_INFO("Stopping audio stream");
	nrfx_i2s_stop();
	return NRF_SUCCESS;
}

ret_code_t codec_set_next_buffer(uint32_t * p_tx_buffer)
{
	nrfx_i2s_buffers_t next_buffers =
	{
		.p_tx_buffer = p_tx_buffer,
		.p_rx_buffer = NULL
	};

	return nrfx_i2s_next_buffers_set(&next_buffers);
}
