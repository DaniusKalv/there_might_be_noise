/**
 * @file        usb.c
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       USB audio device functionality for There Might Be Noise application
 * @version     0.1
 * @date        2020-04-05
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#include "usb.h"

#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"

#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_audio.h"

#include "app_timer.h"

#define NRF_LOG_MODULE_NAME usb
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define USB_RX_PACKET_SIZE 192

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

/**
 * @brief Audio class user event handler
 */
static void spkr_audio_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                       app_usbd_audio_user_event_t   event);


/* Channels and feature controls configuration */

/**
 * @brief   Input terminal channel configuration
 */
#define SPKR_TERMINAL_CH_CONFIG()                                                                   \
        (APP_USBD_AUDIO_IN_TERM_CH_CONFIG_LEFT_FRONT | APP_USBD_AUDIO_IN_TERM_CH_CONFIG_RIGHT_FRONT)

/**
 * @brief   Feature controls
 *
 *      general
 *      channel 0
 *      channel 1
 */
#define SPKR_FEATURE_CONTROLS()                                                                                         \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE), \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE), \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE)

/**
 * @brief   Audio class specific format III descriptor
 */
APP_USBD_AUDIO_FORMAT_DESCRIPTOR(m_spkr_form_desc, 
                                 APP_USBD_AUDIO_AS_FORMAT_III_DSC( /* Format type 3 descriptor */
                                 2,                                /* Number of channels */
                                 2,                                /* Subframe size */
                                 16,                               /* Bit resolution */
                                 1,                                /* Frequency type */
                                 APP_USBD_U24_TO_RAW_DSC(44100))   /* Frequency */
                                );

/**
 * @brief   Audio class input terminal descriptor
 */
APP_USBD_AUDIO_INPUT_DESCRIPTOR(m_spkr_inp_desc, 
                                APP_USBD_AUDIO_INPUT_TERMINAL_DSC(
                                1,                                     /* Terminal ID */
                                APP_USBD_AUDIO_TERMINAL_USB_STREAMING, /* Terminal type */
                                2,                                     /* Number of channels */
                                SPKR_TERMINAL_CH_CONFIG())             /* Channels config */
                               );

/**
 * @brief   Audio class output terminal descriptor
 */
APP_USBD_AUDIO_OUTPUT_DESCRIPTOR(m_spkr_out_desc, 
                                 APP_USBD_AUDIO_OUTPUT_TERMINAL_DSC(
                                 3,                                      /* Terminal ID */
                                 APP_USBD_AUDIO_TERMINAL_OUT_SPEAKER,    /* Terminal type */
                                 2)                                      /* Source ID */
                                );

/**
 * @brief   Audio class feature unit descriptor
 */
APP_USBD_AUDIO_FEATURE_DESCRIPTOR(m_spkr_fea_desc, 
                                  APP_USBD_AUDIO_FEATURE_UNIT_DSC(
                                  2,                       /* Unit ID */
                                  1,                       /* Source ID */
                                  SPKR_FEATURE_CONTROLS()) /* List of controls */
                                 );

/* Interfaces lists */

/**
 * @brief Interfaces list passed to @ref APP_USBD_AUDIO_GLOBAL_DEF
 */
#define SPKR_INTERFACES_CONFIG() APP_USBD_AUDIO_CONFIG_OUT(0, 1)

/**
 * @brief Speaker Audio class instance
 */
APP_USBD_AUDIO_GLOBAL_DEF(m_app_audio_speakers,
                          SPKR_INTERFACES_CONFIG(),
                          spkr_audio_user_ev_handler,
                          &m_spkr_form_desc,
                          &m_spkr_inp_desc,
                          &m_spkr_out_desc,
                          &m_spkr_fea_desc,
                          0,
                          APP_USBD_AUDIO_AS_IFACE_FORMAT_PCM,
                          USB_RX_PACKET_SIZE,
                          APP_USBD_AUDIO_SUBCLASS_AUDIOSTREAMING,
                          1
                         );


APP_TIMER_DEF(m_rx_timeout_timer);

#define USB_RX_TIMEOUT  APP_TIMER_TICKS(3)

/**
 * @brief The size of last received block from
 */
static size_t m_rx_packet_size;

/**
 * @brief Actual speaker mute
 */
static uint8_t m_mute_spkr;

/**
 * @brief Actual sampling frequency
 */
static uint32_t m_freq_spkr;

static usb_event_handler_t m_usb_event_handler = NULL;


/**
 * @brief Audio class specific request handle (speakers)
 */
static void spkr_audio_user_class_req(app_usbd_class_inst_t const * p_inst)
{
	app_usbd_audio_t const * p_audio = app_usbd_audio_class_get(p_inst);
	app_usbd_audio_req_t * p_req = app_usbd_audio_class_request_get(p_audio);

	UNUSED_VARIABLE(m_mute_spkr);
	UNUSED_VARIABLE(m_freq_spkr);

	switch (p_req->req_target)
	{
		case APP_USBD_AUDIO_CLASS_REQ_IN:
			if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
			{
				//Only mute control is defined
				p_req->payload[0] = m_mute_spkr;
				NRF_LOG_INFO("Mute spkr 1 0x%x", m_mute_spkr);
			}
			break;
		case APP_USBD_AUDIO_CLASS_REQ_OUT:

			if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
			{
				//Only mute control is defined
				m_mute_spkr = p_req->payload[0];
				NRF_LOG_INFO("Mute spkr 2 0x%x", m_mute_spkr);
			}
			break;
		case APP_USBD_AUDIO_EP_REQ_IN:
			break;
		case APP_USBD_AUDIO_EP_REQ_OUT:
			if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
			{
				//Only set frequency is supported
				m_freq_spkr = uint24_decode(p_req->payload);
			}
			break;
		default:
			break;
	}
}

/**
 * @brief User event handler @ref app_usbd_audio_user_ev_handler_t (speaker)
 */
static void spkr_audio_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                       app_usbd_audio_user_event_t   event)
{
	app_usbd_audio_t const * p_audio = app_usbd_audio_class_get(p_inst);
	UNUSED_VARIABLE(p_audio);
	switch (event)
	{
		case APP_USBD_AUDIO_USER_EVT_CLASS_REQ:
			spkr_audio_user_class_req(p_inst);
			break;
		case APP_USBD_AUDIO_USER_EVT_RX_DONE:
		{
			app_timer_stop(m_rx_timeout_timer); // TODO
			// app_timer_start(m_rx_timeout_timer, USB_RX_TIMEOUT, NULL); // TODO

			if(m_usb_event_handler != NULL)
			{
				m_usb_event_handler(USB_EVENT_TYPE_RX_DONE, m_rx_packet_size);
			}
		} break;
		default:
			break;
	}
}

static void spkr_sof_ev_handler(uint16_t framecnt)
{
	UNUSED_VARIABLE(framecnt);
	if (APP_USBD_STATE_Configured != app_usbd_core_state_get())
	{
		return;
	}
	m_rx_packet_size = app_usbd_audio_class_rx_size_get(&m_app_audio_speakers.base);

	if (m_rx_packet_size > 0)
	{
		ASSERT(m_rx_packet_size <= USB_RX_PACKET_SIZE);

		if(m_usb_event_handler != NULL)
		{
			m_usb_event_handler(USB_EVENT_TYPE_RX_BUFFER_REQUEST, m_rx_packet_size);
		}
		else
		{
			NRF_LOG_ERROR("USB event handler NULL");
		}
	}
}

/**
 * @brief USBD library specific event handler.
 *
 * @param event     USBD library event.
 */
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
	switch (event)
	{
		case APP_USBD_EVT_DRV_SOF:
			break;
		case APP_USBD_EVT_DRV_SUSPEND:
			// bsp_board_leds_off();
			break;
		case APP_USBD_EVT_DRV_RESUME:
			// bsp_board_led_on(LED_USB_RESUME);
			break;
		case APP_USBD_EVT_STARTED:
			// bsp_board_led_on(LED_USB_START);
			break;
		case APP_USBD_EVT_STOPPED:
			app_usbd_disable();
			// bsp_board_leds_off();
			break;
		case APP_USBD_EVT_POWER_DETECTED:
			NRF_LOG_INFO("USB power detected");
			if (!nrf_drv_usbd_is_enabled())
			{
				app_usbd_enable();
			}
			break;
		case APP_USBD_EVT_POWER_REMOVED:
			NRF_LOG_INFO("USB power removed");
			app_usbd_stop();
			break;
		case APP_USBD_EVT_POWER_READY:
			NRF_LOG_INFO("USB ready");
			app_usbd_start();
			break;
		default:
			break;
	}
}

static void usb_rx_timeout_handler(void * p_context)
{
	if(m_usb_event_handler != NULL)
	{
		m_usb_event_handler(USB_EVENT_TYPE_RX_TIMEOUT, 0);
	}
}

ret_code_t usb_init(usb_event_handler_t evt_handler)
{
	ret_code_t ret;

	static const app_usbd_config_t usbd_config = {
		.ev_state_proc = usbd_user_ev_handler,
		.enable_sof = true
	};

	VERIFY_PARAM_NOT_NULL(evt_handler);

	m_usb_event_handler = evt_handler;

	ret = app_timer_create(&m_rx_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, usb_rx_timeout_handler);
	VERIFY_SUCCESS(ret);

	nrf_drv_clock_init();

	ret = app_usbd_init(&usbd_config);
	VERIFY_SUCCESS(ret);

	app_usbd_class_inst_t const * class_inst_spkr =
	    app_usbd_audio_class_inst_get(&m_app_audio_speakers);

	ret = app_usbd_audio_sof_interrupt_register(class_inst_spkr, spkr_sof_ev_handler);
	VERIFY_SUCCESS(ret);

	ret = app_usbd_class_append(class_inst_spkr);
	VERIFY_SUCCESS(ret);

	return app_usbd_power_events_enable();
}

void usb_rx_buffer_reply(void * p_buff, size_t size)
{
	ret_code_t ret;
	ret = app_usbd_audio_class_rx_start(&m_app_audio_speakers.base, p_buff, size);

	if(ret != NRF_SUCCESS)
	{
		NRF_LOG_ERROR("Could not start an RX transfer");
	}
}

bool usb_event_queue_process(void)
{
	return app_usbd_event_queue_process();
}

void usb_stop(void)
{
	// app_usbd_stop();
	app_usbd_disable();
}