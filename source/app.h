/*
 * app.h
 *
 *  Created on: 19.12.2020
 *      Author: pvvx
 */

#ifndef MAIN_H_
#define MAIN_H_

extern uint32_t utc_time_sec;	// clock in sec (= 0 1970-01-01 00:00:00)

extern uint8_t battery_level; // 0..100%

extern volatile uint8_t send_measure;

void ev_adv_timeout(u8 e, u8 *p, int n);

#endif /* MAIN_H_ */
