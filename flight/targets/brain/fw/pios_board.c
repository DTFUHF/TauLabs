/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup FlyingF4 FlyingF4 support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      The board specific initialization routines
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */

#include "board_hw_defs.c"

#include <pios.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwbrain.h"
#include "modulesettings.h"
#include "manualcontrolsettings.h"
#include "onscreendisplaysettings.h"


/**
 * Configuration for the MPU9250 chip
 */
#if defined(PIOS_INCLUDE_MPU9250_BRAIN)
#include "pios_mpu9250_brain.h"
static const struct pios_exti_cfg pios_exti_mpu9250_cfg __exti_config = {
	.vector = PIOS_MPU9250_IRQHandler,
	.line = EXTI_Line13,
	.pin = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_13,
			.GPIO_Speed = GPIO_Speed_2MHz,  // XXXX
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI15_10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line13, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_mpu9250_cfg pios_mpu9250_cfg = {
	.exti_cfg = &pios_exti_mpu9250_cfg,
	.default_samplerate = 500,  // Note: has to be generated by dividing 1kHz
	.interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,
	.interrupt_en = PIOS_MPU60X0_INTEN_DATA_RDY,
	.User_ctl = 0,
	.Pwr_mgmt_clk = PIOS_MPU60X0_PWRMGMT_PLL_Z_CLK,
	.default_filter = PIOS_MPU9250_LOWPASS_184_HZ,
	.orientation = PIOS_MPU60X0_TOP_0DEG
};
#endif /* PIOS_INCLUDE_MPU9250 */

#if defined(PIOS_INCLUDE_HMC5883)
#include "pios_hmc5883_priv.h"

static const struct pios_hmc5883_cfg pios_hmc5883_external_cfg = {
	.M_ODR = PIOS_HMC5883_ODR_75,
	.Meas_Conf = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain = PIOS_HMC5883_GAIN_1_9,
	.Mode = PIOS_HMC5883_MODE_SINGLE,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif /* PIOS_INCLUDE_HMC5883 */

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611_priv.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
	.oversampling = MS5611_OSR_4096,
	.temperature_interleaving = 1,
};
#endif /* PIOS_INCLUDE_MS5611 */

/** Configuration for the LPS25H baro
 *
 */
#if defined(PIOS_INCLUDE_LPS25H)
#include "pios_lps25h.h"
static const struct pios_lps25h_cfg pios_lps25h_cfg = {
	.i2c_addr = LPS25H_I2C_ADDR_SA0_HIGH,
	.odr = LPS25H_ODR_25HZ
};
#endif /* PIOS_INCLUDE_LPS25H */


/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, SPEKTRUM1, SPEKTRUM2, SBUS
 * NOTE: No slot in this map for NONE.
 */
uintptr_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

#define PIOS_COM_TELEM_RF_RX_BUF_LEN 512
#define PIOS_COM_TELEM_RF_TX_BUF_LEN 512

#define PIOS_COM_GPS_RX_BUF_LEN 32
#define PIOS_COM_GPS_TX_BUF_LEN 16

#define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 65

#define PIOS_COM_BRIDGE_RX_BUF_LEN 65
#define PIOS_COM_BRIDGE_TX_BUF_LEN 12

#define PIOS_COM_MAVLINK_TX_BUF_LEN 128

#define PIOS_COM_HOTT_RX_BUF_LEN 16
#define PIOS_COM_HOTT_TX_BUF_LEN 16

#define PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN 128

#define PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN 19

#define PIOS_COM_PICOC_RX_BUF_LEN 128
#define PIOS_COM_PICOC_TX_BUF_LEN 128

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

bool brain_external_mag_fail;

uintptr_t pios_com_gps_id;
uintptr_t pios_com_telem_usb_id;
uintptr_t pios_com_telem_rf_id;
uintptr_t pios_com_vcp_id;
uintptr_t pios_com_bridge_id;
uintptr_t pios_com_mavlink_id;
uintptr_t pios_com_overo_id;
uintptr_t pios_internal_adc_id;
uintptr_t pios_com_hott_id;
uintptr_t pios_com_frsky_sensor_hub_id;
uintptr_t pios_com_lighttelemetry_id;
uintptr_t pios_com_picoc_id;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_waypoints_settings_fs_id;

/*
 * Setup a com port based on the passed cfg, driver and buffer sizes. rx or tx size of 0 disables rx or tx
 */
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
static void PIOS_Board_configure_com (const struct pios_usart_cfg *usart_port_cfg, size_t rx_buf_len, size_t tx_buf_len,
									  const struct pios_com_driver *com_driver, uintptr_t *pios_com_id)
{
	uintptr_t pios_usart_id;
	if (PIOS_USART_Init(&pios_usart_id, usart_port_cfg)) {
		PIOS_Assert(0);
	}

	uint8_t * rx_buffer;
	if (rx_buf_len > 0) {
		rx_buffer = (uint8_t *) pvPortMalloc(rx_buf_len);
		PIOS_Assert(rx_buffer);
	} else {
		rx_buffer = NULL;
	}

	uint8_t * tx_buffer;
	if (tx_buf_len > 0) {
		tx_buffer = (uint8_t *) pvPortMalloc(tx_buf_len);
		PIOS_Assert(tx_buffer);
	} else {
		tx_buffer = NULL;
	}

	if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
					  rx_buffer, rx_buf_len,
					  tx_buffer, tx_buf_len)) {
		PIOS_Assert(0);
	}
}
#endif	/* PIOS_INCLUDE_USART && PIOS_INCLUDE_COM */

#ifdef PIOS_INCLUDE_DSM
static void PIOS_Board_configure_dsm(const struct pios_usart_cfg *pios_usart_dsm_cfg, const struct pios_dsm_cfg *pios_dsm_cfg,
									 const struct pios_com_driver *pios_usart_com_driver,enum pios_dsm_proto *proto,
									 ManualControlSettingsChannelGroupsOptions channelgroup,uint8_t *bind)
{
	uintptr_t pios_usart_dsm_id;
	if (PIOS_USART_Init(&pios_usart_dsm_id, pios_usart_dsm_cfg)) {
		PIOS_Assert(0);
	}

	uintptr_t pios_dsm_id;
	if (PIOS_DSM_Init(&pios_dsm_id, pios_dsm_cfg, pios_usart_com_driver,
					  pios_usart_dsm_id, *proto, *bind)) {
		PIOS_Assert(0);
	}

	uintptr_t pios_dsm_rcvr_id;
	if (PIOS_RCVR_Init(&pios_dsm_rcvr_id, &pios_dsm_rcvr_driver, pios_dsm_id)) {
		PIOS_Assert(0);
	}
	pios_rcvr_group_map[channelgroup] = pios_dsm_rcvr_id;
}
#endif

#ifdef PIOS_INCLUDE_HSUM
static void PIOS_Board_configure_hsum(const struct pios_usart_cfg *pios_usart_hsum_cfg,
									  const struct pios_com_driver *pios_usart_com_driver,enum pios_hsum_proto *proto,
									  ManualControlSettingsChannelGroupsOptions channelgroup)
{
	uintptr_t pios_usart_hsum_id;
	if (PIOS_USART_Init(&pios_usart_hsum_id, pios_usart_hsum_cfg)) {
		PIOS_Assert(0);
	}
	
	uintptr_t pios_hsum_id;
	if (PIOS_HSUM_Init(&pios_hsum_id, pios_usart_com_driver, pios_usart_hsum_id, *proto)) {
		PIOS_Assert(0);
	}
	
	uintptr_t pios_hsum_rcvr_id;
	if (PIOS_RCVR_Init(&pios_hsum_rcvr_id, &pios_hsum_rcvr_driver, pios_hsum_id)) {
		PIOS_Assert(0);
	}
	pios_rcvr_group_map[channelgroup] = pios_hsum_rcvr_id;
}
#endif


/**
* Initialise PWM Output for black/white level setting
*/

#if defined(PIOS_INCLUDE_VIDEO)
void OSD_configure_bw_levels(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/* --------------------------- System Clocks Configuration -----------------*/
	/* TIM1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Connect TIM1 pins to AF */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 25500000) - 1; // Get clock to 25 MHz on STM32F2/F4
	TIM_TimeBaseStructure.TIM_Period = 255;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* Enable TIM1 Preload register on ARR */
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	/* TIM PWM1 Mode configuration */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 90;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	/* Output Compare PWM1 Mode configuration: Channel1 PA.08 */
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

	/* TIM1 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM1, ENABLE);

	/* TIM1 enable counter */
	TIM_Cmd(TIM1, ENABLE);
	TIM1->CCR1 = 30;
	TIM1->CCR3 = 110;
}
#endif /* PIOS_INCLUDE_VIDEO */

/**************************************************************************************/

/**
 * Indicate a target-specific error code when a component fails to initialize
 * 1 pulse - flash chip
 * 2 pulses - MPU6050
 * 3 pulses - HMC5883
 * 4 pulses - MS5611
 * 5 pulses - gyro I2C bus locked
 * 6 pulses - mag/baro I2C bus locked
 */
static void panic(int32_t code) {
	while(1){
		for (int32_t i = 0; i < code; i++) {
			PIOS_WDG_Clear();
			PIOS_LED_On(PIOS_LED_HEARTBEAT);
			PIOS_DELAY_WaitmS(200);
			PIOS_WDG_Clear();
			PIOS_LED_Off(PIOS_LED_HEARTBEAT);
			PIOS_DELAY_WaitmS(100);
			PIOS_WDG_Clear();
		}
		PIOS_DELAY_WaitmS(500);
		PIOS_WDG_Clear();
		PIOS_DELAY_WaitmS(500);
		PIOS_WDG_Clear();
	}
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void) {
	bool use_internal_mag = true;
	bool ext_mag_init_ok = false;
	bool use_rxport_usart = false;

	/* Delay system */
	PIOS_DELAY_Init();

#if defined(PIOS_INCLUDE_LED)
	PIOS_LED_Init(&pios_led_cfg);
#endif	/* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		panic(3);
#endif

#if defined(PIOS_INCLUDE_SPI)
	if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		panic(1);
	if (PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_flash_id, 0, &flash_mx25_cfg) != 0)
		panic(1);

	/* Register the partition table */
	PIOS_FLASH_register_partition_table(pios_flash_partition_table, NELEMENTS(pios_flash_partition_table));

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		panic(1);
	if (PIOS_FLASHFS_Logfs_Init(&pios_waypoints_settings_fs_id, &flashfs_waypoints_cfg, FLASH_PARTITION_LABEL_WAYPOINTS) != 0)
		panic(1);;
#endif	/* PIOS_INCLUDE_FLASH */

	RCC_ClearFlag(); // The flags cleared after use

	/* Initialize UAVObject libraries */
	EventDispatcherInitialize();
	UAVObjInitialize();

	HwBrainInitialize();
	ModuleSettingsInitialize();

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

#ifndef ERASE_FLASH
	/* Initialize watchdog as early as possible to catch faults during init
	 * but do it only if there is no debugger connected
	 */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
		PIOS_WDG_Init();
	}
#endif

	/* Initialize the alarms library */
	AlarmsInitialize();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Set up pulse timers */
	//inputs
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_12_cfg);
	//outputs
	PIOS_TIM_InitClock(&tim_5_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwBrainSetDefaults(HwBrainHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(),0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}

#if defined(PIOS_INCLUDE_USB)
	/* Initialize board specific USB data */
	PIOS_USB_BOARD_DATA_Init();

	/* Flags to determine if various USB interfaces are advertised */
	bool usb_hid_present = false;
	bool usb_cdc_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
	if (PIOS_USB_DESC_HID_CDC_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
	usb_cdc_present = true;
#else
	if (PIOS_USB_DESC_HID_ONLY_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
#endif

	uintptr_t pios_usb_id;
	PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg);

#if defined(PIOS_INCLUDE_USB_CDC)

	uint8_t hw_usb_vcpport;
	/* Configure the USB VCP port */
	HwBrainUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWBRAIN_USB_VCPPORT_DISABLED;
	}

	uintptr_t pios_usb_cdc_id;
	if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
		PIOS_Assert(0);
	}

	//hw_usb_vcpport = HWBRAIN_USB_VCPPORT_DISABLED;
	switch (hw_usb_vcpport) {
	case HWBRAIN_USB_VCPPORT_DISABLED:
		break;
	case HWBRAIN_USB_VCPPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
	{
		uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
		uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
		PIOS_Assert(rx_buffer);
		PIOS_Assert(tx_buffer);
		if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						  rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
						  tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
#endif	/* PIOS_INCLUDE_COM */
		break;
	case HWBRAIN_USB_VCPPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_COM)
	{
		uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
		uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
		PIOS_Assert(rx_buffer);
		PIOS_Assert(tx_buffer);
		if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						  rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
						  tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
#endif	/* PIOS_INCLUDE_COM */
		break;
	case HWBRAIN_USB_VCPPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
	{
		uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN);
		PIOS_Assert(tx_buffer);
		if (PIOS_COM_Init(&pios_com_debug_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						  NULL, 0,
						  tx_buffer, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */
#endif	/* PIOS_INCLUDE_COM */

		break;
	}
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwBrainUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWBRAIN_USB_HIDPORT_DISABLED;
	}

	uintptr_t pios_usb_hid_id;
	if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
		PIOS_Assert(0);
	}

	hw_usb_hidport = HWBRAIN_USB_HIDPORT_USBTELEMETRY;
	switch (hw_usb_hidport) {
	case HWBRAIN_USB_HIDPORT_DISABLED:
		break;
	case HWBRAIN_USB_HIDPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
	{
		uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
		uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
		PIOS_Assert(rx_buffer);
		PIOS_Assert(tx_buffer);
		if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
						  rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
						  tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
#endif	/* PIOS_INCLUDE_COM */
		break;
	}

#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	uint8_t hw_DSMxBind;
	HwBrainDSMxBindGet(&hw_DSMxBind);

	/* init sensor queue registration */
	PIOS_SENSORS_Init();

	/* Main Port */
	uint8_t hw_mainport;
	HwBrainMainPortGet(&hw_mainport);
	switch (hw_mainport) {
	case HWBRAIN_MAINPORT_DISABLED:
		break;
	case HWBRAIN_MAINPORT_GPS:
#if defined(PIOS_INCLUDE_GPS) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
#endif
		break;
	case HWBRAIN_MAINPORT_SBUS:
		//hardware signal inverter required
#if defined(PIOS_INCLUDE_SBUS) && defined(PIOS_INCLUDE_USART)
	{
		uintptr_t pios_usart_sbus_id;
		if (PIOS_USART_Init(&pios_usart_sbus_id, &pios_mainport_sbus_cfg)) {
			PIOS_Assert(0);
		}
		uintptr_t pios_sbus_id;
		if (PIOS_SBus_Init(&pios_sbus_id, &pios_mainport_sbus_aux_cfg, &pios_usart_com_driver, pios_usart_sbus_id)) {
			PIOS_Assert(0);
		}
		uintptr_t pios_sbus_rcvr_id;
		if (PIOS_RCVR_Init(&pios_sbus_rcvr_id, &pios_sbus_rcvr_driver, pios_sbus_id)) {
			PIOS_Assert(0);
		}
		pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS] = pios_sbus_rcvr_id;
	}
#endif	/* PIOS_INCLUDE_SBUS */
		break;
	case HWBRAIN_MAINPORT_DSM2:
	case HWBRAIN_MAINPORT_DSMX10BIT:
	case HWBRAIN_MAINPORT_DSMX11BIT:
#if defined(PIOS_INCLUDE_DSM)
	{
		enum pios_dsm_proto proto;
		switch (hw_mainport) {
		case HWBRAIN_MAINPORT_DSM2:
			proto = PIOS_DSM_PROTO_DSM2;
			break;
		case HWBRAIN_MAINPORT_DSMX10BIT:
			proto = PIOS_DSM_PROTO_DSMX10BIT;
			break;
		case HWBRAIN_MAINPORT_DSMX11BIT:
			proto = PIOS_DSM_PROTO_DSMX11BIT;
			break;
		default:
			PIOS_Assert(0);
			break;
		}
		PIOS_Board_configure_dsm(&pios_mainport_dsm_hsum_cfg, &pios_mainport_dsm_aux_cfg, &pios_usart_com_driver,
								 &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hw_DSMxBind);
	}
#endif	/* PIOS_INCLUDE_DSM */
		break;
	case HWBRAIN_MAINPORT_HOTTSUMD:
	case HWBRAIN_MAINPORT_HOTTSUMH:
#if defined(PIOS_INCLUDE_HSUM)
	{
		enum pios_hsum_proto proto;
		switch (hw_uart1) {
		case HWBRAIN_MAINPORT_HOTTSUMD:
			proto = PIOS_HSUM_PROTO_SUMD;
			break;
		case HWBRAIN_MAINPORT_HOTTSUMH:
			proto = PIOS_HSUM_PROTO_SUMH;
			break;
		default:
			PIOS_Assert(0);
			break;
		}
		PIOS_Board_configure_hsum(&pios_mainport_dsm_hsum_cfg, &pios_usart_com_driver,
								  &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTTSUM);
	}
#endif	/* PIOS_INCLUDE_HSUM */
		break;
	case HWBRAIN_MAINPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_debug_id);
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */
		break;
	case HWBRAIN_MAINPORT_TELEMETRY:
#if defined(PIOS_INCLUDE_TELEMETRY_RF) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
#endif /* PIOS_INCLUDE_TELEMETRY_RF */
		break;
	case HWBRAIN_MAINPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
#endif
		break;
	case HWBRAIN_MAINPORT_MAVLINKTX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK)
		PIOS_Board_configure_com(&pios_mainport_cfg, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_mavlink_id);
#endif	/* PIOS_INCLUDE_MAVLINK */
		break;
	case HWBRAIN_MAINPORT_MAVLINKTX_GPS_RX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK) && defined(PIOS_INCLUDE_GPS)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
		pios_com_mavlink_id = pios_com_gps_id;
#endif	/* PIOS_INCLUDE_MAVLINK */
		break;
	case HWBRAIN_MAINPORT_HOTTTELEMETRY:
#if defined(PIOS_INCLUDE_HOTT) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_HOTT_RX_BUF_LEN, PIOS_COM_HOTT_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hott_id);
#endif /* PIOS_INCLUDE_HOTT */
		break;
	case HWBRAIN_MAINPORT_FRSKYSENSORHUB:
#if defined(PIOS_INCLUDE_FRSKY_SENSOR_HUB) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, 0, PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_frsky_sensor_hub_id);
#endif /* PIOS_INCLUDE_FRSKY_SENSOR_HUB */
		break;
	case HWBRAIN_MAINPORT_LIGHTTELEMETRYTX:
#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
		PIOS_Board_configure_com(&pios_mainport_cfg, 0, PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_lighttelemetry_id);
#endif
		break;
	case HWBRAIN_MAINPORT_PICOC:
#if defined(PIOS_INCLUDE_PICOC) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_mainport_cfg, PIOS_COM_PICOC_RX_BUF_LEN, PIOS_COM_PICOC_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_picoc_id);
#endif /* PIOS_INCLUDE_PICOC */
		break;
	}

	/* Flx Port */
	uint8_t hw_flxport;
	HwBrainFlxPortGet(&hw_flxport);

	switch (hw_flxport) {
	case HWBRAIN_FLXPORT_DISABLED:
		break;
#if defined(PIOS_INCLUDE_I2C)
	case HWBRAIN_FLXPORT_I2C:
		if (PIOS_I2C_Init(&pios_i2c_flexi_id, &pios_i2c_flexi_cfg)) {
			PIOS_DEBUG_Assert(0);
		}
		//if (PIOS_I2C_CheckClear(pios_i2c_flexi_id) != 0)
		//	panic(6);

#if defined(PIOS_INCLUDE_HMC5883)
		uint8_t Magnetometer;
		HwBrainMagnetometerGet(&Magnetometer);

		if (Magnetometer == HWBRAIN_MAGNETOMETER_EXTERNALFLXPORT) {
			// init sensor
			if (PIOS_HMC5883_Init(pios_i2c_flexi_id, &pios_hmc5883_external_cfg) == 0) {

				if (PIOS_HMC5883_Test() == 0) {
					// sensor configuration was successful: external mag is attached and powered

					// setup sensor orientation
					uint8_t ExtMagOrientation;
					HwBrainExtMagOrientationGet(&ExtMagOrientation);

					enum pios_hmc5883_orientation hmc5883_orientation = \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_TOP0DEGCW) ? PIOS_HMC5883_TOP_0DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_TOP90DEGCW) ? PIOS_HMC5883_TOP_90DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_TOP180DEGCW) ? PIOS_HMC5883_TOP_180DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_TOP270DEGCW) ? PIOS_HMC5883_TOP_270DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_BOTTOM0DEGCW) ? PIOS_HMC5883_BOTTOM_0DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_BOTTOM90DEGCW) ? PIOS_HMC5883_BOTTOM_90DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_BOTTOM180DEGCW) ? PIOS_HMC5883_BOTTOM_180DEG : \
						(ExtMagOrientation == HWBRAIN_EXTMAGORIENTATION_BOTTOM270DEGCW) ? PIOS_HMC5883_BOTTOM_270DEG : \
						pios_hmc5883_external_cfg.Default_Orientation;
					PIOS_HMC5883_SetOrientation(hmc5883_orientation);
					ext_mag_init_ok = true;
				}
			}
			use_internal_mag = false;
		}
#endif /* PIOS_INCLUDE_HMC5883 */
		break;
#endif /* PIOS_INCLUDE_I2C */
	case HWBRAIN_FLXPORT_GPS:
#if defined(PIOS_INCLUDE_GPS) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
#endif
		break;
	case HWBRAIN_FLXPORT_DSM2:
	case HWBRAIN_FLXPORT_DSMX10BIT:
	case HWBRAIN_FLXPORT_DSMX11BIT:
#if defined(PIOS_INCLUDE_DSM)
	{
		enum pios_dsm_proto proto;
		switch (hw_flxport) {
		case HWBRAIN_FLXPORT_DSM2:
			proto = PIOS_DSM_PROTO_DSM2;
			break;
		case HWBRAIN_FLXPORT_DSMX10BIT:
			proto = PIOS_DSM_PROTO_DSMX10BIT;
			break;
		case HWBRAIN_FLXPORT_DSMX11BIT:
			proto = PIOS_DSM_PROTO_DSMX11BIT;
			break;
		default:
			PIOS_Assert(0);
			break;
		}
		PIOS_Board_configure_dsm(&pios_flxport_dsm_hsum_cfg, &pios_flxport_dsm_aux_cfg, &pios_usart_com_driver,
								 &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hw_DSMxBind);
	}
#endif	/* PIOS_INCLUDE_DSM */
		break;
	case HWBRAIN_FLXPORT_HOTTSUMD:
	case HWBRAIN_FLXPORT_HOTTSUMH:
#if defined(PIOS_INCLUDE_HSUM)
	{
		enum pios_hsum_proto proto;
		switch (hw_uart1) {
		case HWBRAIN_FLXPORT_HOTTSUMD:
			proto = PIOS_HSUM_PROTO_SUMD;
			break;
		case HWBRAIN_FLXPORT_HOTTSUMH:
			proto = PIOS_HSUM_PROTO_SUMH;
			break;
		default:
			PIOS_Assert(0);
			break;
		}
		PIOS_Board_configure_hsum(&pios_flxport_dsm_hsum_cfg, &pios_usart_com_driver,
								  &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTTSUM);
	}
#endif	/* PIOS_INCLUDE_HSUM */
		break;
	case HWBRAIN_FLXPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_debug_id);
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */
		break;
	case HWBRAIN_FLXPORT_TELEMETRY:
#if defined(PIOS_INCLUDE_TELEMETRY_RF) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
#endif /* PIOS_INCLUDE_TELEMETRY_RF */
		break;
	case HWBRAIN_FLXPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
#endif
		break;
	case HWBRAIN_FLXPORT_MAVLINKTX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK)
		PIOS_Board_configure_com(&pios_flxport_cfg, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_mavlink_id);
#endif	/* PIOS_INCLUDE_MAVLINK */
		break;
	case HWBRAIN_FLXPORT_MAVLINKTX_GPS_RX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK) && defined(PIOS_INCLUDE_GPS)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
		pios_com_mavlink_id = pios_com_gps_id;
#endif	/* PIOS_INCLUDE_MAVLINK */
		break;
	case HWBRAIN_FLXPORT_HOTTTELEMETRY:
#if defined(PIOS_INCLUDE_HOTT) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_HOTT_RX_BUF_LEN, PIOS_COM_HOTT_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hott_id);
#endif /* PIOS_INCLUDE_HOTT */
		break;
	case HWBRAIN_FLXPORT_FRSKYSENSORHUB:
#if defined(PIOS_INCLUDE_FRSKY_SENSOR_HUB) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, 0, PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_frsky_sensor_hub_id);
#endif /* PIOS_INCLUDE_FRSKY_SENSOR_HUB */
		break;
	case HWBRAIN_FLXPORT_LIGHTTELEMETRYTX:
#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
		PIOS_Board_configure_com(&pios_flxport_cfg, 0, PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_lighttelemetry_id);
#endif
		break;
	case HWBRAIN_FLXPORT_PICOC:
#if defined(PIOS_INCLUDE_PICOC) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
		PIOS_Board_configure_com(&pios_flxport_cfg, PIOS_COM_PICOC_RX_BUF_LEN, PIOS_COM_PICOC_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_picoc_id);
#endif /* PIOS_INCLUDE_PICOC */
		break;
	}


	/* Configure the rcvr port */
	uint8_t hw_rxport;
	HwBrainRxPortGet(&hw_rxport);

	switch (hw_rxport) {
	case HWBRAIN_RXPORT_DISABLED:
		break;
	case HWBRAIN_RXPORT_PWM:
#if defined(PIOS_INCLUDE_PWM)
	{
		uintptr_t pios_pwm_id;
		PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_cfg);

		uintptr_t pios_pwm_rcvr_id;
		if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
			PIOS_Assert(0);
		}
		pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
	}
#endif	/* PIOS_INCLUDE_PWM */
		break;
	case HWBRAIN_RXPORT_PPM:
	case HWBRAIN_RXPORT_PPMOUTPUTS:
#if defined(PIOS_INCLUDE_PPM)
	{
		uintptr_t pios_ppm_id;
		PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_cfg);

		uintptr_t pios_ppm_rcvr_id;
		if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
			PIOS_Assert(0);
		}
		pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
	}
#endif	/* PIOS_INCLUDE_PPM */
		break;
	case HWBRAIN_RXPORT_USART:
		use_rxport_usart = true;
		break;
	case HWBRAIN_RXPORT_PPMUSART:
		use_rxport_usart = true;
		uintptr_t pios_ppm_id;
		PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_cfg);

		uintptr_t pios_ppm_rcvr_id;
		if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
			PIOS_Assert(0);
		}
		break;
	}


	/* Configure the RxPort USART */
	if (use_rxport_usart) {
		uint8_t hw_rxportusart;
		HwBrainRxPortUsartGet(&hw_rxportusart);

		switch (hw_rxportusart) {
		case HWBRAIN_RXPORTUSART_DISABLED:
			break;
		case HWBRAIN_RXPORTUSART_GPS:
#if defined(PIOS_INCLUDE_GPS) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
#endif
			break;
		case HWBRAIN_RXPORTUSART_DSM2:
		case HWBRAIN_RXPORTUSART_DSMX10BIT:
		case HWBRAIN_RXPORTUSART_DSMX11BIT:
#if defined(PIOS_INCLUDE_DSM)
		{
			enum pios_dsm_proto proto;
			switch (hw_flxport) {
			case HWBRAIN_RXPORTUSART_DSM2:
				proto = PIOS_DSM_PROTO_DSM2;
				break;
			case HWBRAIN_RXPORTUSART_DSMX10BIT:
				proto = PIOS_DSM_PROTO_DSMX10BIT;
				break;
			case HWBRAIN_RXPORTUSART_DSMX11BIT:
				proto = PIOS_DSM_PROTO_DSMX11BIT;
				break;
			default:
				PIOS_Assert(0);
				break;
			}
			PIOS_Board_configure_dsm(&pios_rxportusart_dsm_hsum_cfg, &pios_rxportusart_dsm_aux_cfg, &pios_usart_com_driver,
									 &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hw_DSMxBind);
		}
#endif	/* PIOS_INCLUDE_DSM */
			break;
		case HWBRAIN_RXPORTUSART_HOTTSUMD:
		case HWBRAIN_RXPORTUSART_HOTTSUMH:
#if defined(PIOS_INCLUDE_HSUM)
		{
			enum pios_hsum_proto proto;
			switch (hw_uart1) {
			case HWBRAIN_RXPORTUSART_HOTTSUMD:
				proto = PIOS_HSUM_PROTO_SUMD;
				break;
			case HWBRAIN_RXPORTUSART_HOTTSUMH:
				proto = PIOS_HSUM_PROTO_SUMH;
				break;
			default:
				PIOS_Assert(0);
				break;
			}
			PIOS_Board_configure_hsum(&pios_rxportusart_dsm_hsum_cfg, &pios_usart_com_driver,
									  &proto, MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTTSUM);
		}
#endif	/* PIOS_INCLUDE_HSUM */
			break;
		case HWBRAIN_RXPORTUSART_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_debug_id);
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */
			break;
		case HWBRAIN_RXPORTUSART_TELEMETRY:
#if defined(PIOS_INCLUDE_TELEMETRY_RF) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
#endif /* PIOS_INCLUDE_TELEMETRY_RF */
			break;
		case HWBRAIN_RXPORTUSART_COMBRIDGE:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
#endif
			break;
		case HWBRAIN_RXPORTUSART_MAVLINKTX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_mavlink_id);
#endif	/* PIOS_INCLUDE_MAVLINK */
			break;
		case HWBRAIN_RXPORTUSART_MAVLINKTX_GPS_RX:
#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM) && defined(PIOS_INCLUDE_MAVLINK) && defined(PIOS_INCLUDE_GPS)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
			pios_com_mavlink_id = pios_com_gps_id;
#endif	/* PIOS_INCLUDE_MAVLINK */
			break;
		case HWBRAIN_RXPORTUSART_HOTTTELEMETRY:
#if defined(PIOS_INCLUDE_HOTT) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_HOTT_RX_BUF_LEN, PIOS_COM_HOTT_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hott_id);
#endif /* PIOS_INCLUDE_HOTT */
			break;
		case HWBRAIN_RXPORTUSART_FRSKYSENSORHUB:
#if defined(PIOS_INCLUDE_FRSKY_SENSOR_HUB) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, 0, PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_frsky_sensor_hub_id);
#endif /* PIOS_INCLUDE_FRSKY_SENSOR_HUB */
			break;
		case HWBRAIN_RXPORTUSART_LIGHTTELEMETRYTX:
#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, 0, PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_lighttelemetry_id);
#endif
			break;
		case HWBRAIN_RXPORTUSART_PICOC:
#if defined(PIOS_INCLUDE_PICOC) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
			PIOS_Board_configure_com(&pios_rxportusart_cfg, PIOS_COM_PICOC_RX_BUF_LEN, PIOS_COM_PICOC_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_picoc_id);
#endif /* PIOS_INCLUDE_PICOC */
			break;
		}
	}

#if defined(PIOS_INCLUDE_GCSRCVR)
	GCSReceiverInitialize();
	uintptr_t pios_gcsrcvr_id;
	PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
	uintptr_t pios_gcsrcvr_rcvr_id;
	if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
		PIOS_Assert(0);
	}
	pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
#endif	/* PIOS_INCLUDE_GCSRCVR */

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
	switch (hw_rxport) {
	case HWBRAIN_RXPORT_DISABLED:
	case HWBRAIN_RXPORT_PWM:
	case HWBRAIN_RXPORT_PPM:
		/* Set up the servo outputs */
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_cfg);
#endif
		break;
	case HWBRAIN_RXPORT_PPMOUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_cfg);
#endif
		break;
	case HWBRAIN_RXPORT_OUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_all_cfg);
#endif
		break;
	}
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

	uint8_t hw_baro;
	HwBrainBarometerGet(&hw_baro);
	switch (hw_baro) {
	case HWBRAIN_BAROMETER_LPS25H:
#if defined(PIOS_INCLUDE_LPS25H)
		if (PIOS_LPS25H_Init(&pios_lps25h_cfg, pios_i2c_internal_id) != 0)
			panic(5);
#endif
		break;
	case HWBRAIN_BAROMETER_MS5611:
#if defined(PIOS_INCLUDE_MS5611)
		PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id);
		if (PIOS_MS5611_Test() != 0)
			panic(4);
#endif
		break;
	}

	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MPU9250_BRAIN)
#if defined(PIOS_INCLUDE_MPU6050)
	// Enable autoprobing when both 6050 and 9050 compiled in
	bool mpu9250_found = false;
	if (PIOS_MPU9250_Probe(pios_i2c_internal_id, PIOS_MPU9250_I2C_ADD_A0_LOW) == 0) {
		mpu9250_found = true;
#else
	{
#endif /* PIOS_INCLUDE_MPU6050 */

		int retval;
		retval = PIOS_MPU9250_Init(pios_i2c_internal_id, PIOS_MPU9250_I2C_ADD_A0_LOW, use_internal_mag, &pios_mpu9250_cfg);
		if (retval == -10)
			panic(1); // indicate missing IRQ separately
		if (retval != 0)
			panic(2);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwBrainGyroRangeGet(&hw_gyro_range);
		switch(hw_gyro_range) {
			case HWBRAIN_GYRORANGE_250:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
				break;
			case HWBRAIN_GYRORANGE_500:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
				break;
			case HWBRAIN_GYRORANGE_1000:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
				break;
			case HWBRAIN_GYRORANGE_2000:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
				break;
		}

		uint8_t hw_accel_range;
		HwBrainAccelRangeGet(&hw_accel_range);
		switch(hw_accel_range) {
			case HWBRAIN_ACCELRANGE_2G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
				break;
			case HWBRAIN_ACCELRANGE_4G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
				break;
			case HWBRAIN_ACCELRANGE_8G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
				break;
			case HWBRAIN_ACCELRANGE_16G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
				break;
		}
	}
#endif /* PIOS_INCLUDE_MPU9250_BRAIN */

	PIOS_WDG_Clear();


#if defined(PIOS_INCLUDE_ADC)
	uint32_t internal_adc_id;
	PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
#endif

#if defined(PIOS_INCLUDE_VIDEO)
	// make sure the mask pin is low
	GPIO_Init(pios_video_cfg.mask.miso.gpio, (GPIO_InitTypeDef*)&pios_video_cfg.mask.miso.init);
	GPIO_ResetBits(pios_video_cfg.mask.miso.gpio, pios_video_cfg.mask.miso.init.GPIO_Pin);

	// Initialize settings
	OnScreenDisplaySettingsInitialize();

	uint8_t osd_state;
	OnScreenDisplaySettingsOSDEnabledGet(&osd_state);
	if (osd_state == ONSCREENDISPLAYSETTINGS_OSDENABLED_ENABLED) {
		OSD_configure_bw_levels();
		PIOS_Video_Init(&pios_video_cfg);
	}
#endif

	// set variable so the Sensors task sets an alarm
	brain_external_mag_fail = !use_internal_mag && !ext_mag_init_ok;

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_rf_id || pios_com_telem_usb_id);
}

/**
 * @}
 */