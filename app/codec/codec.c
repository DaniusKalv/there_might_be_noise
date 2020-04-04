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
	ret_code_t err_code;
	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);

	m_tlv320aic3106.p_dk_twi_mngr_instance = p_dk_twi_mngr;

	codec_pins_init();

	err_code = tlv320aic3106_init(&m_tlv320aic3106, NULL);;
	VERIFY_SUCCESS(err_code);

/*----------------------------------------------------------------------------*/
	tlv320aic3106_passive_ana_sig_bypass_sel_pd_t bypass;
	memset(&bypass, 0, sizeof(bypass));

	bypass.line1lp_routed_to_left_lop  = true;
	bypass.line1lm_routed_to_left_lom  = true;
	bypass.line1rp_routed_to_right_lop = true;
	bypass.line1rm_routed_to_right_lom = true;

	err_code = tlv320aic3106_set_passive_ana_sig_bypass_sel_pd(&m_tlv320aic3106, &bypass);
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
	tlv320aic3106_pll_prog_reg_a_t pll_prog_reg_a;
	memset(&pll_prog_reg_a, 0, sizeof(pll_prog_reg_a));

	pll_prog_reg_a.pll_p = TLV320AIC3106_PLL_Q_2;

	err_code = tlv320aic3106_set_pll_prog_reg_a(&m_tlv320aic3106, &pll_prog_reg_a);
	VERIFY_SUCCESS(err_code);

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
