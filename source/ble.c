#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "drivers/8258/gpio_8258.h"
#include "ble.h"
#include "vendor/common/blt_common.h"
#include "cmd_parser.h"
#include "app.h"

void bls_set_advertise_prepare(void *p); // add ll_adv.h

RAM uint8_t ble_connected; // bit 0 - connected, bit 1 - conn_param_update, bit 2 - paring success, bit 7 - reset of disconnect
uint8_t send_buf[SEND_BUFFER_SIZE];

RAM uint8_t blt_rxfifo_b[64 * 8] = { 0 };
RAM my_fifo_t blt_rxfifo = { 64, 8, 0, 0, blt_rxfifo_b, };
RAM uint8_t blt_txfifo_b[40 * 16] = { 0 };
RAM my_fifo_t blt_txfifo = { 40, 16, 0, 0, blt_txfifo_b, };
RAM uint8_t ble_name[12] = { 11, 0x09,
		'B', 'L', 'E', '_', '0', '0', '0', '0',	'0', '0' };
RAM uint8_t mac_public[6];
//RAM uint8_t mac_random_static[6];
RAM adv_buf_t adv_buf;
uint8_t ota_is_working = 0;

void app_enter_ota_mode(void) {
	// chk_ota_clear(); //
	bls_ota_clearNewFwDataArea();
	ota_is_working = 1;
	bls_ota_setTimeout(45 * 1000000); // set OTA timeout  45 seconds
}

void ble_disconnect_callback(uint8_t e, uint8_t *p, int n) {
	if(ble_connected & 0x80) // reset device on disconnect?
		start_reboot();

	bls_pm_setManualLatency(0); // ?

	ble_connected = 0;
	ota_is_working = 0;
}

void ble_connect_callback(uint8_t e, uint8_t *p, int n) {
	// bls_l2cap_setMinimalUpdateReqSendingTime_after_connCreate(1000);
	ble_connected = 1;
//	bls_l2cap_requestConnParamUpdate (8, 8, 19, 200);   // 200mS
	bls_l2cap_requestConnParamUpdate (8, 8, 99, 800);   // 1 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 149, 600);  // 1.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 199, 800);  // 2 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 249, 800);  // 2.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 299, 800);  // 3 S
}

int app_conn_param_update_response(u8 id, u16  result) {
	if(result == CONN_PARAM_UPDATE_ACCEPT)
		ble_connected |= 2;
	else if(result == CONN_PARAM_UPDATE_REJECT) {
		// bls_l2cap_requestConnParamUpdate(160, 160, 4, 300); // (200 ms, 200 ms, 1 s, 3 s)
	}
	return 0;
}

extern u32 blt_ota_start_tick;
int otaWritePre(void * p) {
	blt_ota_start_tick = clock_time() | 1;
	return otaWrite(p);
}

_attribute_ram_code_ int RxTxWrite(void * p) {
	cmd_parser(p);
	return 0;
}

/*
 * bls_app_registerEventCallback (BLT_EV_FLAG_ADV_DURATION_TIMEOUT, &ev_adv_timeout);
 * blt_event_callback_t(): */
_attribute_ram_code_ void ev_adv_timeout(u8 e, u8 *p, int n) {
	(void) e; (void) p; (void) n;
	bls_ll_setAdvParam(MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
			ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL,
			BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	bls_ll_setAdvEnable(1);
}

_attribute_ram_code_
__attribute__((optimize("-Os")))
void set_adv_data(void) {
	adv_buf.flag[0] = 0x02; // size
	adv_buf.flag[1] = GAP_ADTYPE_FLAGS; // type
		/*	Flags:
		 	bit0: LE Limited Discoverable Mode
			bit1: LE General Discoverable Mode
			bit2: BR/EDR Not Supported
			bit3: Simultaneous LE and BR/EDR to Same Device Capable (Controller)
			bit4: Simultaneous LE and BR/EDR to Same Device Capable (Host)
			bit5..7: Reserved
		 */
	adv_buf.flag[2] = 0x06; // Flags
	adv_buf.data[0] = 3;
	adv_buf.data[1] = 2;
	adv_buf.data[2] = SERVICE_UUID_BATTERY & 0xff;
	adv_buf.data[3] = (SERVICE_UUID_BATTERY>>8)& 0xff;

	bls_ll_setAdvData((u8 *)&adv_buf, adv_buf.data[0] + 4);
}

extern attribute_t my_Attributes[ATT_END_H];
const char* hex_ascii = { "0123456789ABCDEF" };

void ble_set_name(void) {
	//Set the BLE Name to the last three MACs the first ones are always the same
	ble_name[6] = hex_ascii[mac_public[2] >> 4];
	ble_name[7] = hex_ascii[mac_public[2] & 0x0f];
	ble_name[8] = hex_ascii[mac_public[1] >> 4];
	ble_name[9] = hex_ascii[mac_public[1] & 0x0f];
	ble_name[10] = hex_ascii[mac_public[0] >> 4];
	ble_name[11] = hex_ascii[mac_public[0] & 0x0f];
//	ble_name[0] = 11;
}

__attribute__((optimize("-Os"))) void init_ble(void) {
	////////////////// BLE stack initialization //////////////////////
#if 0
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);
#else
	generateRandomNum(5, mac_public);
	mac_public[5] = 0xC0; 			// for random static
#endif
	/// if bls_ll_setAdvParam( OWN_ADDRESS_RANDOM ) ->  blc_ll_setRandomAddr(mac_random_static);
	ble_set_name();
	////// Controller Initialization  //////////
	blc_ll_initBasicMCU(); //must
	blc_ll_initStandby_module(mac_public); //must
	blc_ll_initAdvertising_module(mac_public); // adv module: 		 must for BLE slave,
	blc_ll_initConnection_module(); // connection module  must for BLE slave/master
	blc_ll_initSlaveRole_module(); // slave module: 	 must for BLE slave,
	blc_ll_initPowerManagement_module(); //pm module:      	 optional

	////// Host Initialization  //////////
	blc_gap_peripheral_init();
	my_att_init(); //gatt initialization
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	blc_smp_setSecurityLevel(No_Security);

	///////////////////// USER application initialization ///////////////////
	bls_ll_setScanRspData((uint8_t *) ble_name, ble_name[0]+1);
	rf_set_power_level_index(MY_RF_POWER);
	// bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &ble_connect_callback);
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &ble_disconnect_callback);

	///////////////////// Power Management initialization///////////////////
	blc_ll_initPowerManagement_module();
	bls_pm_setSuspendMask(SUSPEND_DISABLE);
	blc_pm_setDeepsleepRetentionThreshold(50, 30);
	blc_pm_setDeepsleepRetentionEarlyWakeupTiming(240);
	blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K);
//	bls_ota_clearNewFwDataArea();
	bls_ota_registerStartCmdCb(app_enter_ota_mode);
	blc_l2cap_registerConnUpdateRspCb(app_conn_param_update_response);
	set_adv_data();
//	bls_set_advertise_prepare(app_advertise_prepare_handler);
	ev_adv_timeout(0,0,0);
}
