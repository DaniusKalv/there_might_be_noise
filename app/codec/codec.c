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

#define NRF_LOG_MODULE_NAME CODEC
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
NRF_LOG_MODULE_REGISTER();

TLV320AIC3106_DEF(m_tlv320aic3106, NULL, DK_BSP_TLV320_I2C_ADDRESS);

#define I2S_DATA_BLOCK_WORDS    512
static uint32_t m_buffer_tx[2][I2S_DATA_BLOCK_WORDS] =
{
	{ 0XCB, 0XA5, 0XA8, 0XA5, 0X59, 0XA7, 0XD7, 0XAA, 0X0E, 0XB0, 0XE4, 0XB6, 0X35, 0XBF, 0XD6, 0XC8, 0X96, 0XD3, 0X3C, 0XDF, 0X8C, 0XEB, 0X46, 0XF8, 0X28, 0X05, 0XEF, 0X11, 0X59, 0X1E, 0X26, 0X2A, 0X19, 0X35, 0XF7, 0X3E, 0X8F, 0X47, 0XB4, 0X4E, 0X41, 0X54, 0X18, 0X58, 0X27, 0X5A, 0X62, 0X5A, 0XC7, 0X58, 0X61, 0X55, 0X3F, 0X50, 0X7D, 0X49, 0X3E, 0X41, 0XAC, 0X37, 0XFA, 0X2C, 0X5E, 0X21, 0X15, 0X15, 0X5F, 0X08, 0X7D, 0XFB, 0XB3, 0XEE, 0X42, 0XE2, 0X6C, 0XD6, 0X6D, 0XCB, 0X80, 0XC1, 0XD6, 0XB8, 0X9E, 0XB1, 0XFC, 0XAB, 0X0E, 0XA8, 0XE9, 0XA5, 0X96, 0XA5, 0X19, 0XA7, 0X69, 0XAA, 0X75, 0XAF, 0X23, 0XB6, 0X50, 0XBE, 0XD2, 0XC7, 0X77, 0XD2, 0X09, 0XDE, 0X4B, 0XEA, 0XFD, 0XF6, 0XDE, 0X03, 0XAB, 0X10, 0X22, 0X1D, 0X01, 0X29, 0X0C, 0X34, 0X09, 0X3E, 0XC3, 0X46, 0X0F, 0X4E, 0XC6, 0X53, 0XCA, 0X57, 0X07, 0X5A, 0X71, 0X5A, 0X05, 0X59, 0XCC, 0X55, 0XD6, 0X50, 0X3C, 0X4A, 0X21, 0X42, 0XAF, 0X38, 0X17, 0X2E, 0X90, 0X22, 0X56, 0X16, 0XA7, 0X09, 0XC7, 0XFC, 0XF7, 0XEF, 0X7B, 0XE3, 0X92, 0XD7, 0X7C, 0XCC, 0X70, 0XC2, 0XA4, 0XB9, 0X45, 0XB2, 0X79, 0XAC, 0X5E, 0XA8, 0X0B, 0XA6, 0X89, 0XA5, 0XDD, 0XA6, 0X00, 0XAA, 0XE1, 0XAE, 0X66, 0XB5, 0X6F, 0XBD, 0XD1, 0XC6, 0X5B, 0XD1, 0XD8, 0XDC, 0X0B, 0XE9, 0XB5, 0XF5, 0X94, 0X02, 0X66, 0X0F, 0XE8, 0X1B, 0XDA, 0X27, 0XFC, 0X32, 0X17, 0X3D, 0XF4, 0X45, 0X66, 0X4D, 0X47, 0X53, 0X78, 0X57, 0XE3, 0X59, 0X7B, 0X5A, 0X3F, 0X59, 0X33, 0X56, 0X68, 0X51, 0XF7, 0X4A, 0X01, 0X43, 0XAF, 0X39, 0X32, 0X2F, 0XC0, 0X23, 0X95, 0X17, 0XEF, 0X0A, 0X11, 0XFE, 0X3D, 0XF1, 0XB5, 0XE4, 0XBB, 0XD8, 0X8C, 0XCD, 0X64, 0XC3, 0X75, 0XBA, 0XF0, 0XB2, 0XFA, 0XAC, 0XB3, 0XA8, 0X31, 0XA6, 0X81, 0XA5, 0XA6, 0XA6, 0X9B, 0XA9, 0X50, 0XAE, 0XAD, 0XB4, 0X91, 0XBC, 0XD2, 0XC5, 0X42, 0XD0, 0XA8, 0XDB, 0XCC, 0XE7, 0X6D, 0XF4, 0X4A, 0X01, 0X21, 0X0E, 0XAE, 0X1A, 0XB0, 0X26, 0XEA, 0X31, 0X21, 0X3C, 0X21, 0X45, 0XB9, 0X4C, 0XC3, 0X52, 0X21, 0X57, 0XBA, 0X59, 0X81, 0X5A, 0X73, 0X59, 0X95, 0X56, 0XF6, 0X51, 0XAE, 0X4B, 0XDD, 0X43, 0XAC, 0X3A, 0X4A, 0X30, 0XEE, 0X24, 0XD3, 0X18, 0X37, 0X0C, 0X5B, 0XFF, 0X83, 0XF2, 0XF0, 0XE5, 0XE5, 0XD9, 0XA0, 0XCE, 0X5A, 0XC4, 0X4A, 0XBB, 0X9F, 0XB3, 0X80, 0XAD, 0X0D, 0XA9, 0X5D, 0XA6, 0X7E, 0XA5, 0X74, 0XA6, 0X3B, 0XA9, 0XC4, 0XAD, 0XF8, 0XB3, 0XB6, 0XBB, 0XD7, 0XC4, 0X2A, 0XCF, 0X7B, 0XDA, 0X8E, 0XE6, 0X26, 0XF3, 0X00, 0X00, 0XDA, 0X0C, 0X72, 0X19, 0X85, 0X25, 0XD6, 0X30, 0X29, 0X3B, 0X4A, 0X44, 0X08, 0X4C, 0X3C, 0X52, 0XC5, 0X56, 0X8C, 0X59, 0X82, 0X5A, 0XA3, 0X59, 0XF3, 0X56, 0X80, 0X52, 0X61, 0X4C, 0XB6, 0X44, 0XA6, 0X3B, 0X60, 0X31, 0X1B, 0X26, 0X10, 0X1A, 0X7D, 0X0D, 0XA5, 0X00, 0XC9, 0XF3, 0X2D, 0XE7, 0X12, 0XDB, 0XB6, 0XCF, 0X54, 0XC5, 0X23, 0XBC, 0X52, 0XB4, 0X0A, 0XAE, 0X6B, 0XA9, 0X8D, 0XA6, 0X7F, 0XA5, 0X46, 0XA6, 0XDF, 0XA8, 0X3D, 0XAD, 0X47, 0XB3, 0XDF, 0XBA, 0XDF, 0XC3, 0X16, 0XCE, 0X50, 0XD9, 0X52, 0XE5, 0XDF, 0XF1, 0XB6, 0XFE, 0X93, 0X0B, 0X34, 0X18, 0X58, 0X24, 0XBE, 0X2F, 0X2E, 0X3A, 0X6F, 0X43, 0X53, 0X4B, 0XB0, 0X51, 0X65, 0X56, 0X5A, 0X59, 0X7F, 0X5A, 0XCF, 0X59, 0X4D, 0X57, 0X06, 0X53, 0X10, 0X4D, 0X8B, 0X45, 0X9C, 0X3C, 0X74, 0X32, 0X45, 0X27, 0X4B, 0X1B, 0XC3, 0X0E, 0XEF, 0X01, 0X11, 0XF5 },
	{ 0XCB, 0XA5, 0XA8, 0XA5, 0X59, 0XA7, 0XD7, 0XAA, 0X0E, 0XB0, 0XE4, 0XB6, 0X35, 0XBF, 0XD6, 0XC8, 0X96, 0XD3, 0X3C, 0XDF, 0X8C, 0XEB, 0X46, 0XF8, 0X28, 0X05, 0XEF, 0X11, 0X59, 0X1E, 0X26, 0X2A, 0X19, 0X35, 0XF7, 0X3E, 0X8F, 0X47, 0XB4, 0X4E, 0X41, 0X54, 0X18, 0X58, 0X27, 0X5A, 0X62, 0X5A, 0XC7, 0X58, 0X61, 0X55, 0X3F, 0X50, 0X7D, 0X49, 0X3E, 0X41, 0XAC, 0X37, 0XFA, 0X2C, 0X5E, 0X21, 0X15, 0X15, 0X5F, 0X08, 0X7D, 0XFB, 0XB3, 0XEE, 0X42, 0XE2, 0X6C, 0XD6, 0X6D, 0XCB, 0X80, 0XC1, 0XD6, 0XB8, 0X9E, 0XB1, 0XFC, 0XAB, 0X0E, 0XA8, 0XE9, 0XA5, 0X96, 0XA5, 0X19, 0XA7, 0X69, 0XAA, 0X75, 0XAF, 0X23, 0XB6, 0X50, 0XBE, 0XD2, 0XC7, 0X77, 0XD2, 0X09, 0XDE, 0X4B, 0XEA, 0XFD, 0XF6, 0XDE, 0X03, 0XAB, 0X10, 0X22, 0X1D, 0X01, 0X29, 0X0C, 0X34, 0X09, 0X3E, 0XC3, 0X46, 0X0F, 0X4E, 0XC6, 0X53, 0XCA, 0X57, 0X07, 0X5A, 0X71, 0X5A, 0X05, 0X59, 0XCC, 0X55, 0XD6, 0X50, 0X3C, 0X4A, 0X21, 0X42, 0XAF, 0X38, 0X17, 0X2E, 0X90, 0X22, 0X56, 0X16, 0XA7, 0X09, 0XC7, 0XFC, 0XF7, 0XEF, 0X7B, 0XE3, 0X92, 0XD7, 0X7C, 0XCC, 0X70, 0XC2, 0XA4, 0XB9, 0X45, 0XB2, 0X79, 0XAC, 0X5E, 0XA8, 0X0B, 0XA6, 0X89, 0XA5, 0XDD, 0XA6, 0X00, 0XAA, 0XE1, 0XAE, 0X66, 0XB5, 0X6F, 0XBD, 0XD1, 0XC6, 0X5B, 0XD1, 0XD8, 0XDC, 0X0B, 0XE9, 0XB5, 0XF5, 0X94, 0X02, 0X66, 0X0F, 0XE8, 0X1B, 0XDA, 0X27, 0XFC, 0X32, 0X17, 0X3D, 0XF4, 0X45, 0X66, 0X4D, 0X47, 0X53, 0X78, 0X57, 0XE3, 0X59, 0X7B, 0X5A, 0X3F, 0X59, 0X33, 0X56, 0X68, 0X51, 0XF7, 0X4A, 0X01, 0X43, 0XAF, 0X39, 0X32, 0X2F, 0XC0, 0X23, 0X95, 0X17, 0XEF, 0X0A, 0X11, 0XFE, 0X3D, 0XF1, 0XB5, 0XE4, 0XBB, 0XD8, 0X8C, 0XCD, 0X64, 0XC3, 0X75, 0XBA, 0XF0, 0XB2, 0XFA, 0XAC, 0XB3, 0XA8, 0X31, 0XA6, 0X81, 0XA5, 0XA6, 0XA6, 0X9B, 0XA9, 0X50, 0XAE, 0XAD, 0XB4, 0X91, 0XBC, 0XD2, 0XC5, 0X42, 0XD0, 0XA8, 0XDB, 0XCC, 0XE7, 0X6D, 0XF4, 0X4A, 0X01, 0X21, 0X0E, 0XAE, 0X1A, 0XB0, 0X26, 0XEA, 0X31, 0X21, 0X3C, 0X21, 0X45, 0XB9, 0X4C, 0XC3, 0X52, 0X21, 0X57, 0XBA, 0X59, 0X81, 0X5A, 0X73, 0X59, 0X95, 0X56, 0XF6, 0X51, 0XAE, 0X4B, 0XDD, 0X43, 0XAC, 0X3A, 0X4A, 0X30, 0XEE, 0X24, 0XD3, 0X18, 0X37, 0X0C, 0X5B, 0XFF, 0X83, 0XF2, 0XF0, 0XE5, 0XE5, 0XD9, 0XA0, 0XCE, 0X5A, 0XC4, 0X4A, 0XBB, 0X9F, 0XB3, 0X80, 0XAD, 0X0D, 0XA9, 0X5D, 0XA6, 0X7E, 0XA5, 0X74, 0XA6, 0X3B, 0XA9, 0XC4, 0XAD, 0XF8, 0XB3, 0XB6, 0XBB, 0XD7, 0XC4, 0X2A, 0XCF, 0X7B, 0XDA, 0X8E, 0XE6, 0X26, 0XF3, 0X00, 0X00, 0XDA, 0X0C, 0X72, 0X19, 0X85, 0X25, 0XD6, 0X30, 0X29, 0X3B, 0X4A, 0X44, 0X08, 0X4C, 0X3C, 0X52, 0XC5, 0X56, 0X8C, 0X59, 0X82, 0X5A, 0XA3, 0X59, 0XF3, 0X56, 0X80, 0X52, 0X61, 0X4C, 0XB6, 0X44, 0XA6, 0X3B, 0X60, 0X31, 0X1B, 0X26, 0X10, 0X1A, 0X7D, 0X0D, 0XA5, 0X00, 0XC9, 0XF3, 0X2D, 0XE7, 0X12, 0XDB, 0XB6, 0XCF, 0X54, 0XC5, 0X23, 0XBC, 0X52, 0XB4, 0X0A, 0XAE, 0X6B, 0XA9, 0X8D, 0XA6, 0X7F, 0XA5, 0X46, 0XA6, 0XDF, 0XA8, 0X3D, 0XAD, 0X47, 0XB3, 0XDF, 0XBA, 0XDF, 0XC3, 0X16, 0XCE, 0X50, 0XD9, 0X52, 0XE5, 0XDF, 0XF1, 0XB6, 0XFE, 0X93, 0X0B, 0X34, 0X18, 0X58, 0X24, 0XBE, 0X2F, 0X2E, 0X3A, 0X6F, 0X43, 0X53, 0X4B, 0XB0, 0X51, 0X65, 0X56, 0X5A, 0X59, 0X7F, 0X5A, 0XCF, 0X59, 0X4D, 0X57, 0X06, 0X53, 0X10, 0X4D, 0X8B, 0X45, 0X9C, 0X3C, 0X74, 0X32, 0X45, 0X27, 0X4B, 0X1B, 0XC3, 0X0E, 0XEF, 0X01, 0X11, 0XF5 }
};

// Delay time between consecutive I2S transfers performed in the main loop
// (in milliseconds).
#define PAUSE_TIME          500
// Number of blocks of data to be contained in each transfer.
#define BLOCKS_TO_TRANSFER  20

static uint8_t volatile m_blocks_transferred     = 0;
static uint8_t          m_zero_samples_to_ignore = 0;
static uint16_t         m_sample_value_to_send;
static uint16_t         m_sample_value_expected;
static bool             m_error_encountered;

static uint32_t       * volatile mp_block_to_fill  = NULL;
static uint32_t const * volatile mp_block_to_check = NULL;

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
	if (!p_released->p_tx_buffer)
	{
		nrfx_i2s_buffers_t const next_buffers = {
			.p_rx_buffer = NULL,
			.p_tx_buffer = m_buffer_tx[1],
		};
		APP_ERROR_CHECK(nrfx_i2s_next_buffers_set(&next_buffers));
	}
	else
	{
		nrfx_i2s_buffers_t next_buffers = {
			.p_rx_buffer = NULL
		};

		if(p_released->p_tx_buffer == m_buffer_tx[0])
		{
			next_buffers.p_tx_buffer = m_buffer_tx[1];
		}
		else
		{
			next_buffers.p_tx_buffer = m_buffer_tx[0];
		}
		// The driver has just finished accessing the buffers pointed by
		// 'p_released'. They can be used for the next part of the transfer
		// that will be scheduled now.
		APP_ERROR_CHECK(nrfx_i2s_next_buffers_set(&next_buffers));

		// The pointer needs to be typecasted here, so that it is possible to
		// modify the content it is pointing to (it is marked in the structure
		// as pointing to constant data because the driver is not supposed to
		// modify the provided data).
	}
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

ret_code_t codec_init(dk_twi_mngr_t * p_dk_twi_mngr)
{
	ret_code_t err_code;
	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;

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

	nrfx_i2s_buffers_t const initial_buffers = {
		.p_tx_buffer = m_buffer_tx[0],
		.p_rx_buffer = NULL
	};

	err_code = nrfx_i2s_start(&initial_buffers, I2S_DATA_BLOCK_WORDS, 0);
	VERIFY_SUCCESS(err_code);

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
