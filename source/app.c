#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"
#include "cmd_parser.h"
#include "battery.h"
#include "ble.h"
#include "app.h"
//#include "i2c.h"

void app_enter_ota_mode(void);

RAM uint16_t battery_mv;    // 2200..3300 mV
RAM uint8_t  battery_level; // 0..100%
RAM volatile uint8_t send_measure; // measure complete
#define measurement_step_time (CLOCK_16M_SYS_TIMER_CLK_1S*10)
RAM uint32_t tim_measure; // timer measurements >= 10 sec


RAM uint32_t utc_time_sec;	// clock in sec (= 0 -> 1970-01-01 00:00:00)
RAM uint32_t utc_time_sec_tick;
#define utc_time_tick_step CLOCK_16M_SYS_TIMER_CLK_1S

//--- check battery
_attribute_ram_code_ void check_battery(uint16_t bmv) {
	battery_mv = get_battery_mv();
	if (battery_mv < bmv) {
		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER,
			clock_time() + 120 * CLOCK_16M_SYS_TIMER_CLK_1S); // go deep-sleep 2 minutes
	}
	battery_level = get_battery_level(battery_mv);
}
//------------------ user_init_normal -------------------
void user_init_normal(void) {
//this will get executed one time after power up
	check_battery(MIN_VBAT_MV); // 2.2V
	random_generator_init(); //must
	init_ble();
//	bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &suspend_exit_cb);
//	bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_ENTER, &suspend_enter_cb);
}

//------------------ user_init_deepRetn -------------------
_attribute_ram_code_ void user_init_deepRetn(void) {
//after sleep this will get executed
	blc_ll_initBasicMCU();
	rf_set_power_level_index(MY_RF_POWER);
	blc_ll_recoverDeepRetention();
	bls_ota_registerStartCmdCb(app_enter_ota_mode);
}

//----------------------- main_loop()
_attribute_ram_code_ void main_loop(void) {
	blt_sdk_main_loop();
	while(clock_time() -  utc_time_sec_tick > utc_time_tick_step) {
		utc_time_sec_tick += utc_time_tick_step;
		utc_time_sec++; // + 1 sec
	}
	if (ota_is_working) {
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN); // SUSPEND_DISABLE
		bls_pm_setManualLatency(0);
	} else {
		if ((blc_ll_getCurrentState() & BLS_LINK_STATE_CONN) && blc_ll_getTxFifoNumber() < 9) {
			if(send_measure) {
				if (batteryValueInCCC[0] | batteryValueInCCC[1])
					ble_send_battery();
				send_measure = 0;
			}
			if(rd_fmem_size) {
				rdfb();
			}

		}
		uint32_t new = clock_time();
		if (new - tim_measure >= measurement_step_time) {
			send_measure = 1;
			tim_measure = new;
			check_battery(MIN_VBAT_MV);
		}
		bls_pm_setSuspendMask(
				SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN
						| DEEPSLEEP_RETENTION_CONN);
	}
}
