/**
 * @file        dk_config.h
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       DK C lib and project configuration
 * @version     0.1
 * @date        2019-11-23
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2019 All rights reserved
 * 
 */

#ifndef DK_CONFIG_H
#define DK_CONFIG_H
// <<< Use Configuration Wizard in Context Menu >>>\n

// <h> DK UICR - UICR register configuration

// <o> DK_UICR_REGOUT0 - GPIO reference voltage / external output supply voltage in high voltage mode

// <0=> 1.8V
// <1=> 2.1V 
// <2=> 2.4V 
// <3=> 2.7V 
// <4=> 3.0V 
// <5=> 3.3V 
// <7=> Default - 1.8V 

#ifndef DK_UICR_REGOUT0
#define DK_UICR_REGOUT0 5
#endif

// </h> 
//==========================================================

// <h> DK Modules

//==========================================================
// <e> DK_BATTERY_LVL_ENABLED - Enable battery level measurement module

#ifndef DK_BATTERY_LVL_ENABLED
#define DK_BATTERY_LVL_ENABLED 1
#endif

//SAADC initialized by app
// <q> DK_BATTERY_LVL_SAADC_INIT - Initialize SAADC internaly
// <i> If selected initializes SAADC upon module initialization.
// <i> SAADC is initialized with configuration defined in sdk_config.h
// <i> If this functionality is not desired, disable it and initialize
// <i> SAADC before initializing BATTERY_LVL module.
//==========================================================

#ifndef DK_BATTERY_LVL_SAADC_INIT
#define DK_BATTERY_LVL_SAADC_INIT 1
#endif

// <o> DK_BATTERY_LVL_BATTERY_TYPE - Battery type
 
// <0=> LiPo (Connected to VDDH)
// <1=> Coin Cell (Connected to VDD)

#ifndef DK_BATTERY_LVL_BATTERY_TYPE
#define DK_BATTERY_LVL_BATTERY_TYPE 0
#endif

//Measurement frequency
// </e>

//==========================================================
// <e> DK_TWI_MNGR_ENABLED - Enable DK TWI manager module

#ifndef DK_TWI_MNGR_ENABLED
#define DK_TWI_MNGR_ENABLED 1
#endif

// </e>

// </h> 
//==========================================================

// <h> External drivers

// <h> IS31FL3206

// <e> DK_IS31FL3206_GAMMA_ENABLED - Enable gamma correction in IS31FL3206 driver

#ifndef DK_IS31FL3206_GAMMA_ENABLED
#define DK_IS31FL3206_GAMMA_ENABLED 1
#endif

// <o> DK_IS31FL3206_GAMMA_STEPS - Ammount of steps for gamma correction
 
// <32=> 32 steps
// <64=> 64 steps

#ifndef DK_IS31FL3206_GAMMA_STEPS
#define DK_IS31FL3206_GAMMA_STEPS 64
#endif

// </e>

// </h> 
//==========================================================

// </h> 
//==========================================================

#endif
