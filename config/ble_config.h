 /*
  @brief BLE parameters
 */

#ifndef BLE_CONFIG_H__
#define BLE_CONFIG_H__

#include "ble_srv_common.h"
#include "dk_ble_uuids.h"
#include "dk_uicr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_NAME                     "ThereMightBeNoise "                    /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "Danius Kalvaitis"                      /**< Manufacturer. Will be passed to Device Information Service. */

#define DEVICE_MODEL_NUMBER             DK_UICR_DEVICE_MODEL_NO
#define DEVICE_HIGH_SERIAL_NUMBER       DK_UICR_HIGH_DEVICE_ID
#define DEVICE_SHORT_SERIAL_NUMBER      DK_UICR_LOW_DEVICE_ID
#define DEVICE_HW_REV                   DK_UICR_DEVICE_HW_REV

#define MANUFACTURER_ID                 0xFFFF                                  /**< Default Manufacturer ID assigned by Bluetooth SIG. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                0                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(7.5, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              1                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                      /**< Maximum encryption key size. */

#define BLE_OPCODE_LENGTH               1                                       /**< OP-Code indicates the ATT Operation such as Write Command, Notification, Read Response or etc. */
#define BLE_ATTRIBUTE_HANDLE_LENGTH     2                                       /**< When sending a Write, Read and Notification or Indication packets, the associated attribute handle (2 octets) will also need to be included for identification of the data. */

#ifdef __cplusplus
}
#endif

#endif
