/**
 * @file        usb.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       USB audio device functionality for There Might Be Noise application
 * @version     0.1
 * @date        2020-04-05
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#ifndef USB_H
#define USB_H

#include <stddef.h>
#include <stdbool.h>
#include "sdk_errors.h"

typedef enum
{
	USB_EVENT_TYPE_RX_BUFFER_REQUEST,
	USB_EVENT_TYPE_RX_DONE,
	USB_EVENT_TYPE_RX_TIMEOUT
} usb_event_type_t;

typedef void (* usb_event_handler_t)(usb_event_type_t event_type, size_t size);

ret_code_t usb_init(usb_event_handler_t evt_handler);

void usb_rx_buffer_reply(void * p_buff, size_t size);

bool usb_event_queue_process(void);

void usb_stop(void);

#endif // USB_H
