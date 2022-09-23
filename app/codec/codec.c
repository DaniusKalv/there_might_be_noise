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
#include "codec_hal.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrfx_i2s.h"
#include "codec_buffer.h"

#include "app_timer.h"

#define NRF_LOG_MODULE_NAME codec
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
NRF_LOG_MODULE_REGISTER();

#define CODEC_DEBUG_INTERVAL APP_TIMER_TICKS(1000)

// static int16_t warmup_data[32] = { 0 };

// static int16_t test_data[2][64] =
// {
// 	{
// 		0,
// 		0,
// 		6510,
// 		6510,
// 		12760,
// 		12760,
// 		18502,
// 		18502,
// 		23506,
// 		23506,
// 		27572,
// 		27572,
// 		30540,
// 		30540,
// 		32290,
// 		32290,
// 		32753,
// 		32753,
// 		31910,
// 		31910,
// 		29795,
// 		29795,
// 		26492,
// 		26492,
// 		22133,
// 		22133,
// 		16891,
// 		16891,
// 		10977,
// 		10977,
// 		4624,
// 		4624,
// 		-1913,
// 		-1913,
// 		-8373,
// 		-8373,
// 		-14500,
// 		-14500,
// 		-20049,
// 		-20049,
// 		-24798,
// 		-24798,
// 		-28559,
// 		-28559,
// 		-31181,
// 		-31181,
// 		-32560,
// 		-32560,
// 		-32641,
// 		-32641,
// 		-31421,
// 		-31421,
// 		-28948,
// 		-28948,
// 		-25321,
// 		-25321,
// 		-20685,
// 		-20685,
// 		-15224,
// 		-15224,
// 		-9156,
// 		-9156,
// 		-2723,
// 		-2723
// 	},
// 	{
// 		3819,
// 		3819,
// 		10208,
// 		10208,
// 		16191,
// 		16191,
// 		21527,
// 		21527,
// 		26006,
// 		26006,
// 		29448,
// 		29448,
// 		31716,
// 		31716,
// 		32719,
// 		32719,
// 		32418,
// 		32418,
// 		30825,
// 		30825,
// 		28003,
// 		28003,
// 		24064,
// 		24064,
// 		19166,
// 		19166,
// 		13504,
// 		13504,
// 		7303,
// 		7303,
// 		812,
// 		812,
// 		-5712,
// 		-5712,
// 		-12008,
// 		-12008,
// 		-17826,
// 		-17826,
// 		-22933,
// 		-22933,
// 		-27125,
// 		-27125,
// 		-30237,
// 		-30237,
// 		-32142,
// 		-32142,
// 		-32767,
// 		-32767,
// 		-32085,
// 		-32085,
// 		-30124,
// 		-30124,
// 		-26962,
// 		-26962,
// 		-22725,
// 		-22725,
// 		-17582,
// 		-17582,
// 		-11738,
// 		-11738,
// 		-5426,
// 		-5426,
// 		1102,
// 		1102
// 	}

// };

static codec_event_handler_t m_event_handler = NULL;
static bool m_streaming_audio;
static bool m_i2s_warmup = false;

static void i2s_data_handler(nrfx_i2s_buffers_t const * p_released,
                             uint32_t                   status)
{
	VERIFY_PARAM_NOT_NULL_VOID(p_released);
	ret_code_t err_code;

	if(m_i2s_warmup) {
		nrfx_i2s_stop();
		m_i2s_warmup = false;
	}

	if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)) // This will get called two times. For each buffer release.
	{
		if(m_streaming_audio == false)
		{
			if(m_event_handler != NULL)
			{
				m_event_handler(CODEC_EVT_TYPE_AUDIO_STREAM_STOPPED);
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
			m_event_handler(CODEC_EVT_TYPE_AUDIO_STREAM_STARTED);
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

static void codec_hal_evt_handler(codec_evt_type_t event_type)
{
	if(m_event_handler != NULL)
	{
		m_event_handler(event_type);
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

// static ret_code_t i2c_warmup()
// {
// 	m_i2s_warmup = true;

// 	nrfx_i2s_buffers_t initial_buffers =
// 	{
// 		.p_tx_buffer = (uint32_t *)warmup_data,
// 		.p_rx_buffer = NULL
// 	};

// 	return nrfx_i2s_start(&initial_buffers, (sizeof(warmup_data) / sizeof(uint32_t)), 0);
// }

ret_code_t codec_init(dk_twi_mngr_t const * p_dk_twi_mngr, codec_event_handler_t event_handler)
{
	ret_code_t err_code;

	VERIFY_PARAM_NOT_NULL(p_dk_twi_mngr);
	VERIFY_PARAM_NOT_NULL(event_handler);

	m_event_handler = event_handler;
	m_streaming_audio = false;

	err_code = codec_buffer_init(codec_buffer_event_handler);
	VERIFY_SUCCESS(err_code);

	err_code = i2s_init();
	VERIFY_SUCCESS(err_code);

	err_code = codec_hal_init(p_dk_twi_mngr, codec_hal_evt_handler);
	VERIFY_SUCCESS(err_code);

	// i2c_warmup();

	return NRF_SUCCESS;
}

ret_code_t codec_set_mode(codec_mode_t mode)
{
	return codec_hal_mode_set(mode);
}

ret_code_t codec_mute(bool mute)
{
	return codec_hal_mute(mute);
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

void codec_debug(void)
{
	codec_hal_debug();
}

/*
digital mode has 2 sampling frequency modes
mute
volume

codec_mute
codec_set_volume

codec_event_handler
CODEC_READY
*/