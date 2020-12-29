#ifndef PLATFORM_GUARD 
#define PLATFORM_GUARD

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

uint32_t* PORT_NUM;
uint32_t* requests;
uint32_t* taddrs;
uint32_t* control;
uint32_t* grant;
uint32_t* discard;
uint32_t* available;
uint32_t mytrue;

#define GOT_PKG(pn) requests[pn] // get request flag for port pn
#define TADDR(pn) taddrs[pn]// get requested path for port pn
#define CONTROL_TX(pn,cw) control[pn]=cw // set control word to cw for port pn
#define GRANT(pn) grant[pn] // set granted field for port pn
#define DISCARD(pn) discard[pn] // set discard field for port pn
#define AVAILABLE(pn) available[pn] // get mux ready flag for port pn
#define MYTRUE mytrue

struct Table_bottom {
	uint32_t port_num;
};

struct Table_top {
	uint32_t target_port;
	uint32_t options_num;
	struct Table_bottom* options;
};

// sw constants at predefined addresses
uint32_t* TABLE_SIZE;
struct Table_top* table;

#endif
