#pragma once 
enum {
	CMD_ID_UTC_TIME = 0x23, // Get/set utc time (if USE_CLOCK = 1)
	CMD_ID_MTU	= 0x71, // Request Mtu Size Exchange (23..255)
	CMD_ID_REBOOT	= 0x72, // Set Reboot on disconnect
	CMD_ID_OTAC    = 0xD1,  // OTA clear
	CMD_ID_WRFB    = 0xD3,  // Write Flash
	CMD_ID_RDFB    = 0xD4,  // Read Flash Block
	CMD_ID_ERFB    = 0xD5,  // Erase Flash Sector
	CMD_ID_CHGB    = 0xD7,  // Change boot
} CMD_ID_KEYS;

void cmd_parser(void * p);
extern uint32_t rd_fmem_size;
void rdfb(void);
void chk_ota_clear(void);

