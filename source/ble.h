#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "app.h"
#include "stack/ble/ble.h"

extern uint8_t ota_is_working;
extern uint8_t ble_connected; // bit 0 - connected, bit 1 - conn_param_update, bit 2 - paring success, bit 7 - reset device on disconnect
#define ADV_BUFFER_SIZE		28
typedef struct __attribute__((packed)) _adv_buf_t {
	uint8_t flag[3];
	uint8_t data[ADV_BUFFER_SIZE];
}adv_buf_t;
extern adv_buf_t adv_buf;
extern u8 batteryValueInCCC[2];
extern u8 my_RxTx_Data[16];
extern u8 RxTxValueInCCC[2];
#define SEND_BUFFER_SIZE	(ATT_MTU_SIZE-3) // = 20
extern uint8_t send_buf[SEND_BUFFER_SIZE];
extern uint8_t mac_public[6];
extern uint8_t mac_random_static[6];
extern uint8_t ble_name[12];

//#define ADV_CUSTOM_UUID16 0x181A // 16-bit UUID Service 0x181A Environmental Sensing

#define MY_ADV_INTERVAL_MIN	ADV_INTERVAL_1S
#define MY_ADV_INTERVAL_MAX	(ADV_INTERVAL_1S+ADV_INTERVAL_35MS)
#define MY_RF_POWER	RF_POWER_P0p04dBm

typedef struct
{
  /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMin;
  /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMax;
  /** Number of LL latency connection events (0x0000 - 0x03e8) */
  u16 latency;
  /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
  u16 timeout;
} gap_periConnectParams_t;
extern gap_periConnectParams_t my_periConnParameters;

///////////////////////////////////// ATT  HANDLER define ///////////////////////////////////////
typedef enum
{
	ATT_H_START = 0,

	//// Gap ////
	/**********************************************************************************************/
	GenericAccess_PS_H, 					//UUID: 2800, 	VALUE: uuid 1800
	GenericAccess_DeviceName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	GenericAccess_DeviceName_DP_H,			//UUID: 2A00,   VALUE: device name
	GenericAccess_Appearance_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	GenericAccess_Appearance_DP_H,			//UUID: 2A01,	VALUE: appearance
	CONN_PARAM_CD_H,						//UUID: 2803, 	VALUE:  			Prop: Read
	CONN_PARAM_DP_H,						//UUID: 2A04,   VALUE: connParameter

	//// Gatt ////
	/**********************************************************************************************/
	GenericAttribute_PS_H,					//UUID: 2800, 	VALUE: uuid 1801
	GenericAttribute_ServiceChanged_CD_H,	//UUID: 2803, 	VALUE:  			Prop: Indicate
	GenericAttribute_ServiceChanged_DP_H,   //UUID:	2A05,	VALUE: service change
	GenericAttribute_ServiceChanged_CCB_H,	//UUID: 2902,	VALUE: serviceChangeCCC

#if USE_DEVICE_INFO_CHR_UUID
	//// device information ////
	/**********************************************************************************************/
	DeviceInformation_PS_H,					//UUID: 2800, 	VALUE: uuid 180A
	DeviceInformation_ModName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_ModName_DP_H,			//UUID: 2A24,	VALUE: Model Number String
	DeviceInformation_SerialN_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_SerialN_DP_H,			//UUID: 2A25,	VALUE: Serial Number String
	DeviceInformation_FirmRev_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_FirmRev_DP_H,			//UUID: 2A26,	VALUE: Firmware Revision String
	DeviceInformation_HardRev_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_HardRev_DP_H,			//UUID: 2A27,	VALUE: Hardware Revision String
	DeviceInformation_SoftRev_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_SoftRev_DP_H,			//UUID: 2A28,	VALUE: Software Revision String
	DeviceInformation_ManName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_ManName_DP_H,			//UUID: 2A29,	VALUE: Manufacturer Name String
#endif
	//// Battery service ////
	/**********************************************************************************************/
	BATT_PS_H, 								//UUID: 2800, 	VALUE: uuid 180f
	BATT_LEVEL_INPUT_CD_H,					//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	BATT_LEVEL_INPUT_DP_H,					//UUID: 2A19 	VALUE: batVal
	BATT_LEVEL_INPUT_CCB_H,					//UUID: 2902, 	VALUE: batValCCC

	//// Telink OTA ////
	/**********************************************************************************************/
	OTA_PS_H, 								//UUID: 2800, 	VALUE: telink ota service uuid
	OTA_CMD_OUT_CD_H,						//UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	OTA_CMD_OUT_DP_H,						//UUID: telink ota uuid,  VALUE: otaData
	OTA_CMD_OUT_DESC_H,						//UUID: 2901, 	VALUE: otaName

	//// Custom RxTx ////
	/**********************************************************************************************/
	RxTx_PS_H, 								//UUID: 2800, 	VALUE: 1F10 RxTx service uuid
	RxTx_CMD_OUT_CD_H,						//UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	RxTx_CMD_OUT_DP_H,						//UUID: 1F1F,  VALUE: RxTxData
	RxTx_CMD_OUT_DESC_H,					//UUID: 2902, 	VALUE: RxTxValueInCCC

	ATT_END_H,

}ATT_HANDLE;

void my_att_init();
void init_ble();
void ble_set_name(void);
bool ble_get_connected();
void ble_send_cmf(void);

int otaWritePre(void * p);
int RxTxWrite(void * p);
void ev_adv_timeout(u8 e, u8 *p, int n);

inline void ble_send_battery(void) {
	bls_att_pushNotifyData(BATT_LEVEL_INPUT_DP_H, (u8 *) &battery_level, 1);
}
