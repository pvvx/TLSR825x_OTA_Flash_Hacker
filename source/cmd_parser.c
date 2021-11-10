#include <stdint.h>
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"
#include "ble.h"

#include "cmd_parser.h"

RAM uint32_t fmem_adr, rd_fmem_size;


#define _flash_read(faddr,len,pbuf) flash_read_page(FLASH_BASE_ADDR + (uint32_t)faddr, len, (uint8_t *)pbuf)

__attribute__((optimize("-Os")))
static
uint32_t chk_addr_cur_boot(void) {
	uint32_t hid = 0;
	uint32_t faddr = 0;
	while(faddr <= 0x40000) {
		flash_read_page(faddr + 8, 4, (uint8_t *)&hid);
		if(hid == 0x544c4e4b) {
			return faddr;
		}
		faddr += 0x10000;
	}
	return -1;
}

__attribute__((optimize("-Os")))
static
uint32_t chk_addr_old_ota(uint32_t facur) {
	uint32_t hid = 0;
	uint32_t faddr = 0;
	if(facur == -1)
		return -1;
	if(facur) {
		flash_read_page(faddr + 8, 4, (uint8_t *)&hid);
		if((hid == 0)||(hid == 0x544c4e00))
			return faddr;
	} else {
		faddr = 0x20000;
		while(faddr <= 0x40000) {
			flash_read_page(faddr + 8, 4, (uint8_t *)&hid);
			if((hid == 0)||(hid == 0x544c4e00)) {
				flash_read_page(faddr -4, 4, (uint8_t *)&hid);
				if(hid == -1)
					return faddr;
			}
			faddr += 0x10000;
		}
	}
	return -1;
}

__attribute__((optimize("-Os")))
static
void fsec_patch(uint32_t faddr, uint32_t off, uint32_t data) {
	uint32_t _data = data;
	uint32_t offset = 0;
	uint8_t sec_buf[4096];
#if 1
	flash_read_page(faddr, sizeof(sec_buf), sec_buf);
#else
	while(offset < sizeof(sec_buf)) {
		flash_read_page(faddr + offset, 256, sec_buf + offset);
		offset += 256;
	}
#endif
	memcpy(sec_buf + off, &_data, sizeof(_data));
	flash_erase_sector(faddr);
	offset = 0;
	while(offset < sizeof(sec_buf)) {
		flash_write_page(faddr + offset, 256, sec_buf + offset);
		offset += 256;
	}
}

__attribute__((optimize("-Os")))
static
int chng_boot(void) {
	uint32_t bfaddr = chk_addr_cur_boot();
	if(bfaddr != -1) {
		uint32_t ofaddr = chk_addr_old_ota(bfaddr);
		if(ofaddr != -1) {
			fsec_patch(ofaddr, 8, 0x544c4e4b);
			uint32_t hid = 0;
			flash_write_page(bfaddr + 8, sizeof(hid), (uint8_t *)&hid);
			return 0;
		}
	}
	return -1;
}

__attribute__((optimize("-Os")))
void cmd_parser(void * p) {
	rf_packet_att_data_t *req = (rf_packet_att_data_t*) p;
	uint32_t len = req->l2cap - 3;
	if(len) {
		uint8_t cmd = req->dat[0];
		send_buf[0] = cmd;
		send_buf[1] = 0; // no err?
		uint32_t olen = 0;
		if (cmd == CMD_ID_UTC_TIME) { // Get/set utc time
			if(--len > sizeof(utc_time_sec)) len = sizeof(utc_time_sec);
			if(len)
				memcpy(&utc_time_sec, &req->dat[1], len);
			memcpy(&send_buf[1], &utc_time_sec, sizeof(utc_time_sec));
			olen = sizeof(utc_time_sec) + 1;
		} else if (cmd == CMD_ID_MTU && len > 1) { // Request Mtu Size Exchange
			if(req->dat[1] > ATT_MTU_SIZE)
				send_buf[1] = blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, req->dat[1]);
			else
				send_buf[1] = 0xff;
			olen = 2;
		} else if (cmd == CMD_ID_REBOOT) { // Set Reboot on disconnect
			ble_connected |= 0x80; // reset device on disconnect
			olen = 2;

			// Debug commands (unsupported in different versions!):

		} else if (cmd == CMD_ID_RDFB && len > 3) { // Read Flash Block
			fmem_adr = req->dat[1] | (req->dat[2]<<8) | (req->dat[3]<<16);
			if(len > 6) {
				rd_fmem_size = req->dat[4] | (req->dat[5]<<8) | (req->dat[6]<<16);
				if(rd_fmem_size > 16)
					bls_pm_setManualLatency(0);
			} else
				rd_fmem_size = 16;
		} else if (cmd == CMD_ID_WRFB && len > 4) { // Write Flash
			fmem_adr = req->dat[1] | (req->dat[2]<<8) | (req->dat[3]<<16);
			flash_write_page(fmem_adr, len - 4, &req->dat[4]);
			flash_read_page(fmem_adr, len - 4, &send_buf[4]);
			memcpy(send_buf, req->dat, 4);
			olen = len;
		} else if (cmd == CMD_ID_ERFB && len > 3) { // Erase Flash Sector
			fmem_adr = req->dat[1] | (req->dat[2]<<8) | (req->dat[3]<<16);
			flash_erase_sector(fmem_adr);
			flash_read_page(fmem_adr, 16, &send_buf[4]);
			memcpy(send_buf, req->dat, 4);
			olen = 16+4;
		} else if (cmd == CMD_ID_CHGB) { // Change boot
			send_buf[1] = (uint8_t)chng_boot();
			if(send_buf[1] == 0)
				ble_connected |= 0x80; // reset device on disconnect
			olen = 2;
		} else if (cmd == CMD_ID_OTAC) { // OTA clear
			bls_ota_clearNewFwDataArea();
			olen = 2;
		}
		if(olen)
			bls_att_pushNotifyData(RxTx_CMD_OUT_DP_H, send_buf, olen);
	}
}

void rdfb(void) {
	uint32_t olen = rd_fmem_size;

	if (olen > 16) olen = 16;
	if (olen) {
		send_buf[0] = CMD_ID_RDFB;
		send_buf[1] = (uint8_t)fmem_adr;
		send_buf[2] = (uint8_t)(fmem_adr >> 8);
		send_buf[3] = (uint8_t)(fmem_adr >> 16);

		flash_read_page(fmem_adr, 16, &send_buf[4]);

		fmem_adr += olen;
		rd_fmem_size -= olen;

		bls_att_pushNotifyData(RxTx_CMD_OUT_DP_H, send_buf, 16 + 4);
	}
	if(rd_fmem_size == 0)
		bls_pm_setManualLatency(99);
}

void chk_ota_clear(void) {
	uint32_t faddr = chk_addr_old_ota(chk_addr_cur_boot());
	if(faddr != -1)
		bls_ota_clearNewFwDataArea();
}
