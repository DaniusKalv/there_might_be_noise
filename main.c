#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "app_error.h"
#include "app_timer.h"
#include "app_scheduler.h"

#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "ble_dfu.h"

#include "ble_config.h"
#include "boards.h"

#include "fds.h"

#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_delay.h"
#include "nrf_power.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_bootloader_info.h"

#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrfx_gpiote.h"
#include "nordic_common.h"
#include "peer_manager.h"
#include "dk_ble_dis.h"
#include "dk_twi_mngr.h"

#include "dk_ble_advertising.h"
#include "dk_ble_gap.h"

#include "dk_ble_dis.h"

#include "dk_twi.h"

#include "sh1106.h"
#include "splash.h"

#include "codec.h"
#include "usb.h"

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#ifndef DEBUG
#define DFU_ENABLED
#endif

#define SCHED_EVENT_DATA_SIZE   32
#define SCHED_QUEUE_SIZE        16

#define TWI_MNGR_QUEUE_SIZE     24

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

DK_TWI_MNGR_DEF(m_twi_mngr_codec, TWI_MNGR_QUEUE_SIZE, DK_BSP_TLV320_I2C_ITERFACE);

static nrfx_spi_t m_spi = NRFX_SPI_INSTANCE(DK_BSP_OLED_SPI_INTERFACE);  /**< SPI instance. */

SH1106_DEF(m_display, &m_spi, DK_BSP_OLED_RST, DK_BSP_OLED_CS, DK_BSP_OLED_DC, 128, 64);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
	ret_code_t err_code;

	NRF_LOG_INFO("Sleep mode entero");

	NRF_LOG_FINAL_FLUSH();

	// Go to system-off mode (this function will not return; wakeup will cause a reset).
	err_code = sd_power_system_off();
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
	ret_code_t err_code = NRF_SUCCESS;

	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
			NRF_LOG_INFO("Connected.");
			m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			NRF_LOG_INFO("Disconnected.");
			m_conn_handle = BLE_CONN_HANDLE_INVALID;
			break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
		{
			NRF_LOG_DEBUG("PHY update request.");
			ble_gap_phys_t const phys =
			{
				.rx_phys = BLE_GAP_PHY_AUTO,
				.tx_phys = BLE_GAP_PHY_AUTO,
			};
			err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
			APP_ERROR_CHECK(err_code);
		} break;

		case BLE_GATTC_EVT_TIMEOUT:
			// Disconnect on GATT Client timeout event.
			NRF_LOG_DEBUG("GATT Client Timeout.");
			err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
											 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			APP_ERROR_CHECK(err_code);
			break;

		case BLE_GATTS_EVT_TIMEOUT:
			// Disconnect on GATT Server timeout event.
			NRF_LOG_DEBUG("GATT Server Timeout.");
			err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
											 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			APP_ERROR_CHECK(err_code);
			break;
		default:
			// No implementation needed.
			break;
	}
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	// Configure the BLE stack using the default settings.
	// Fetch the start address of the application RAM.
	uint32_t ram_start = 0;
	err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	APP_ERROR_CHECK(err_code);

	// Enable BLE stack.
	err_code = nrf_sdh_ble_enable(&ram_start);
	APP_ERROR_CHECK(err_code);

	// Register a handler for BLE events.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

	ble_conn_state_init();
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
	ret_code_t err_code;

	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
	{
		err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
		APP_ERROR_CHECK(err_code);
	}
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
	ret_code_t             err_code;
	ble_conn_params_init_t cp_init;

	memset(&cp_init, 0, sizeof(cp_init));

	cp_init.p_conn_params                  = NULL;
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	cp_init.disconnect_on_fail             = false;
	cp_init.evt_handler                    = on_conn_params_evt;
	cp_init.error_handler                  = conn_params_error_handler;

	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
	ret_code_t err_code;

	NRF_LOG_INFO("Erase bonds!");

	err_code = pm_peers_delete();
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
	//ret_code_t err_code;

	switch (ble_adv_evt)
	{
		case BLE_ADV_EVT_FAST:
			NRF_LOG_INFO("Fast advertising.");
			break;

		case BLE_ADV_EVT_IDLE:
			sleep_mode_enter();
			break;

		default:
			break;
	}
}

/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
	if (erase_bonds == true)
	{
		delete_bonds(); // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event
	}
	else
	{
		ret_code_t err_code = dk_ble_advertising_start(&m_advertising);
		APP_ERROR_CHECK(err_code);
	}
}

/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
	NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
	              p_gatt->att_mtu_desired_central,
	              p_gatt->att_mtu_desired_periph);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
	ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	APP_ERROR_CHECK(err_code);
}

#ifdef DFU_ENABLED

static void disconnect(uint16_t conn_handle, void * p_context)
{
	UNUSED_PARAMETER(p_context);

	ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	if (err_code != NRF_SUCCESS)
	{
		NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
	}
	else
	{
		NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
	}
}

// TODO: Update this code if you want to do anything given a DFU event (optional).
/**@brief Function for handling dfu events from the Buttonless Secure DFU service
 *
 * @param[in]   event   Event from the Buttonless Secure DFU service.
 */
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
	switch (event)
	{
		case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
			NRF_LOG_INFO("Device is preparing to enter bootloader mode.");

			ble_adv_modes_config_t config;
			dk_ble_advertising_config_get(&config);
			config.ble_adv_on_disconnect_disabled = true;
			ble_advertising_modes_config_set(&m_advertising, &config);

			// Disconnect all bonded devices that currently are connected.
			//           This is required to receive a service changed indication
			//           on bootup after a successful (or aborted) Device Firmware Update.

			uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
			NRF_LOG_INFO("Disconnected %d links.", conn_count);
			break;

		case BLE_DFU_EVT_BOOTLOADER_ENTER:
			// TODO: Write app-specific unwritten data to FLASH, control finalization of this
			//           by delaying reset by reporting false in app_shutdown_handler
			NRF_LOG_INFO("Device will enter bootloader mode.");
			break;

		case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
			NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
			// TODO: Take corrective measures to resolve the issue
			//           like calling APP_ERROR_CHECK to reset the device.
			break;

		case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
			NRF_LOG_ERROR("Request to send a response to client failed.");
			// TODO: Take corrective measures to resolve the issue
			//           like calling APP_ERROR_CHECK to reset the device.
			APP_ERROR_CHECK(false);
			break;

		default:
			NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
			break;
	}
}

/**@brief Handler for shutdown preparation.
 *
 * @details During shutdown procedures, this function will be called at a 1 second interval
 *          untill the function returns true. When the function returns true, it means that the
 *          app is ready to reset to DFU mode.
 *
 * @param[in]   event   Power manager event.
 *
 * @retval  True if shutdown is allowed by this power manager handler, otherwise false.
 */
static bool app_shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
	switch (event)
	{
		case NRF_PWR_MGMT_EVT_PREPARE_DFU:
			NRF_LOG_INFO("Power management wants to reset to DFU mode.");
			NRF_LOG_FINAL_FLUSH();
			#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
				// To allow the buffer to be flushed by the host.
				nrf_delay_ms(100);
			#endif
			// TODO: Get ready to reset into DFU mode
			//
			// If you aren't finished with any ongoing tasks, return "false" to
			// signal to the system that reset is impossible at this stage.
			//
			// Here is an example using a variable to delay resetting the device.
			//
			// if (!m_ready_for_reset)
			// {
			//      return false;
			// }
			// else
			//{
			//
			//    // Device ready to enter
			//    uint32_t err_code;
			//    err_code = sd_softdevice_disable();
			//    APP_ERROR_CHECK(err_code);
			//    err_code = app_timer_stop_all();
			//    APP_ERROR_CHECK(err_code);
			//}
			break;

		default:
			// TODO: Implement any of the other events available from the power management module:
			//      -NRF_PWR_MGMT_EVT_PREPARE_SYSOFF
			//      -NRF_PWR_MGMT_EVT_PREPARE_WAKEUP
			//      -NRF_PWR_MGMT_EVT_PREPARE_RESET
			return true;
	}

	NRF_LOG_INFO("Power management allowed to reset to DFU mode.");
	return true;
}

static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void *p_context)
{
	if (state == NRF_SDH_EVT_STATE_DISABLED)
	{
		// Softdevice was disabled before going into reset. Inform bootloader to skip CRC on next boot.
		nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

		//Go to system off.
		nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
	}
}

/**@brief Register application shutdown handler with priority 0.
 */
NRF_PWR_MGMT_HANDLER_REGISTER(app_shutdown_handler, 0);

/* nrf_sdh state observer. */
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) =
{
	.handler = buttonless_dfu_sdh_state_observer,
};

static void bootloader_async_svci_init(void)
{
	ret_code_t err_code;
	err_code = ble_dfu_buttonless_async_svci_init();
	APP_ERROR_CHECK(err_code);
}

#endif

/**@brief Function for initializing services that will be used by the application.
 */
void services_init(void)
{
	ret_code_t                  err_code;
#ifdef DFU_ENABLED
	ble_dfu_buttonless_init_t   dfus_init = {0};
#endif

	err_code = dk_ble_dis_init();
	APP_ERROR_CHECK(err_code);

#ifdef DFU_ENABLED 
	dfus_init.evt_handler = ble_dfu_evt_handler;

	err_code = ble_dfu_buttonless_init(&dfus_init);
	APP_ERROR_CHECK(err_code);
#endif
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
	ret_code_t err_code;

	switch (p_evt->evt_id)
	{
		case PM_EVT_BONDED_PEER_CONNECTED:
		{
			NRF_LOG_INFO("Connected to a previously bonded device.");
		} break;

		case PM_EVT_CONN_SEC_SUCCEEDED:
		{
			NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
			             ble_conn_state_role(p_evt->conn_handle),
			             p_evt->conn_handle,
			             p_evt->params.conn_sec_succeeded.procedure);
		} break;

		case PM_EVT_CONN_SEC_FAILED:
		{
			/* Often, when securing fails, it shouldn't be restarted, for security reasons.
			 * Other times, it can be restarted directly.
			 * Sometimes it can be restarted, but only after changing some Security Parameters.
			 * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
			 * Sometimes it is impossible, to secure the link, or the peer device does not support it.
			 * How to handle this error is highly application dependent. */
		} break;

		case PM_EVT_CONN_SEC_CONFIG_REQ:
		{
			// Accept pairing request from an already bonded peer.
			pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};
			pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
		} break;

		case PM_EVT_STORAGE_FULL:
		{
			// Run garbage collection on the flash.
			err_code = fds_gc();
			if (err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
			{
				// Retry.
			}
			else
			{
				APP_ERROR_CHECK(err_code);
			}
		} break;

		case PM_EVT_PEERS_DELETE_SUCCEEDED:
		{
			advertising_start(false);
		} break;

		case PM_EVT_PEER_DATA_UPDATE_FAILED:
		{
			// Assert.
			APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
		} break;

		case PM_EVT_PEER_DELETE_FAILED:
		{
			// Assert.
			APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
		} break;

		case PM_EVT_PEERS_DELETE_FAILED:
		{
			// Assert.
			APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
		} break;

		case PM_EVT_ERROR_UNEXPECTED:
		{
			// Assert.
			APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
		} break;

		case PM_EVT_CONN_SEC_START:
		case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
		case PM_EVT_PEER_DELETE_SUCCEEDED:
		case PM_EVT_LOCAL_DB_CACHE_APPLIED:
		case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
			// This can happen when the local DB has changed.
		case PM_EVT_SERVICE_CHANGED_IND_SENT:
		case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
		default:
			break;
	}
}

/**@brief Function for the Peer Manager initialization.
 */
void peer_manager_init(void)
{
	ble_gap_sec_params_t sec_param;
	ret_code_t           err_code;

	err_code = pm_init();
	APP_ERROR_CHECK(err_code);

	memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

	// Security parameters to be used for all security procedures.
	sec_param.bond           = SEC_PARAM_BOND;
	sec_param.mitm           = SEC_PARAM_MITM;
	sec_param.lesc           = SEC_PARAM_LESC;
	sec_param.keypress       = SEC_PARAM_KEYPRESS;
	sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
	sec_param.oob            = SEC_PARAM_OOB;
	sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
	sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
	sec_param.kdist_own.enc  = 1;
	sec_param.kdist_own.id   = 1;
	sec_param.kdist_peer.enc = 1;
	sec_param.kdist_peer.id  = 1;

	err_code = pm_sec_params_set(&sec_param);
	APP_ERROR_CHECK(err_code);

	err_code = pm_register(pm_evt_handler);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
void idle_state_handle(void)
{
	if (NRF_LOG_PROCESS() == false)
	{
		nrf_pwr_mgmt_run();
	}
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

ret_code_t twi_mngr_init(dk_twi_mngr_t const * p_dk_twi_mngr,
                         uint32_t scl_pin,
                         uint32_t sda_pin)
{
	nrfx_twi_config_t twi_config =
	{
		.frequency          = (nrf_twi_frequency_t)NRFX_TWI_DEFAULT_CONFIG_FREQUENCY,
		.scl                = scl_pin,
		.sda                = sda_pin,
		.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
		.hold_bus_uninit    = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,
	};

	return dk_twi_mngr_init(p_dk_twi_mngr, &twi_config);
}

#ifdef DEBUG
/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
	ret_code_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}
#endif

static void usb_event_handler(usb_event_type_t event_type, size_t size)
{
	ret_code_t err_code;

	switch (event_type)
	{
		case USB_EVENT_TYPE_RX_BUFFER_REQUEST:
		{
			void * p_buffer = codec_get_rx_buffer(size);

			if(p_buffer != NULL)
			{
				usb_rx_buffer_reply(p_buffer, size);
			}
			else
			{
				NRF_LOG_ERROR("Could not allocate audio rx buffer");
				APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
			}
			
		} break;
		case USB_EVENT_TYPE_RX_DONE:
			err_code = codec_release_rx_buffer(size);
			APP_ERROR_CHECK(err_code);
			break;
		case USB_EVENT_TYPE_RX_TIMEOUT:
			NRF_LOG_INFO("USB rx timeout");
			err_code = codec_release_unfinished_rx_buffer();
			APP_ERROR_CHECK(err_code);
			break;
		default:
			break;
	}
}

static void codec_event_handler(codec_event_type_t event_type)
{
	switch(event_type)
	{
		case CODEC_EVENT_TYPE_AUDIO_STREAM_STARTED:
			NRF_LOG_INFO("Codec audio stream started");
			break;
		case CODEC_EVENT_TYPE_AUDIO_STREAM_STOPPED:
			NRF_LOG_INFO("Codec audio stream stopped");
			break;
		default:
			break;
	}
}

/**@brief Function for application main entry.
 */
int main(void)
{
#ifdef DFU_ENABLED
	bootloader_async_svci_init(); // Initialize the async SVCI interface to bootloader before any interrupts are enabled.
#endif

	ret_code_t err_code;
	bool erase_bonds = false;

#ifdef DEBUG
	log_init();
#endif

	NRF_LOG_DEBUG("Log initialised.");
	NRF_LOG_PROCESS();

	err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	nrf_gpio_cfg_output(DK_BSP_TPA3220_RST);
	nrf_gpio_pin_clear(DK_BSP_TPA3220_RST);
	// nrf_gpio_cfg(DK_BSP_TPA3220_MUTE, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
	nrf_gpio_cfg_output(DK_BSP_TPA3220_HEAD);
	nrf_gpio_pin_clear(DK_BSP_TPA3220_HEAD);

	nrf_gpio_cfg_input(DK_BSP_TPA3220_FAULT, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(DK_BSP_TPA3220_OTW_CLIP, NRF_GPIO_PIN_NOPULL);

	nrf_gpio_pin_set(DK_BSP_TPA3220_RST);

	// nrf_gpio_pin_set(DK_BSP_TPA3220_RST);
	// nrf_gpio_pin_set(DK_BSP_TPA3220_MUTE);
	// while(true)
	// {
	// 	nrf_delay_ms(10000);
	// }

	nrfx_spi_config_t spi_config = NRFX_SPI_DEFAULT_CONFIG;
	spi_config.mosi_pin = DK_BSP_OLED_MOSI;
	spi_config.sck_pin  = DK_BSP_OLED_SCLK;
	err_code = nrfx_spi_init(&m_spi, &spi_config, NULL, NULL);
	APP_ERROR_CHECK(err_code);

	err_code = sh1106_init(&m_display);
	APP_ERROR_CHECK(err_code);

	sh1106_write_data(&m_display, splash_image, sizeof(splash_image));

	err_code = nrfx_gpiote_init();
	APP_ERROR_CHECK(err_code);

	err_code = twi_mngr_init(&m_twi_mngr_codec, DK_BSP_I2C_SCL0, DK_BSP_I2C_SDA0);
	APP_ERROR_CHECK(err_code);

	err_code = codec_init(&m_twi_mngr_codec, codec_event_handler);
	APP_ERROR_CHECK(err_code);

	err_code = usb_init(usb_event_handler);
	APP_ERROR_CHECK(err_code);

	APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);

	NRF_LOG_FLUSH();

	ble_stack_init();

	err_code = sd_clock_hfclk_request();
	APP_ERROR_CHECK(err_code);

	sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE); // Enable DC to DC converter right after the softdevice is enabled

	peer_manager_init();

	err_code = dk_ble_gap_init();
	APP_ERROR_CHECK(err_code);

	gatt_init();

	err_code = dk_ble_advertising_init(&m_advertising, on_adv_evt);
	APP_ERROR_CHECK(err_code);

	services_init();
	conn_params_init();

#ifdef DEBUG
	NRF_LOG_FLUSH();
	nrf_delay_ms(100);
#endif

	// advertising_start(erase_bonds);

	// Enter main loop.
	for (;;)
	{
		while(usb_event_queue_process())
		{
			NRF_LOG_PROCESS();
		}

		app_sched_execute();
		idle_state_handle();
	}
}
