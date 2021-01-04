#ifndef PLATFORM_GUARD 
#define PLATFORM_GUARD

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////
// test environment
#ifdef TEST

uint32_t enable; // fake constant for while loop
#define ENABLE enable // give it to the infinite loop as abstraction

// hw flags/regs on target platform
uint32_t* port_num;
uint32_t* requests;
uint32_t* taddrs;
uint32_t* control;
uint32_t* grant;
uint32_t* discard;
uint32_t* available;

#define GOT_PKG(pn)       requests[(pn)]    // get request flag for port pn
#define TADDR(pn)         taddrs[(pn)]      // get requested path for port pn
#define CONTROL_TX(pn,cw) control[(pn)]=cw  // set control word to cw for port pn
#define GRANT(pn)         grant[(pn)]       // set granted field for port pn
#define DISCARD(pn)       discard[(pn)]     // set discard field for port pn
#define AVAILABLE(pn)     available[(pn)]   // get mux ready flag for port pn

#endif

/////////////////////////////////////////////////////////////////////
// target environment
#ifndef TEST

#include "hw.h" // generated header

#define ENABLE 1

volatile uint32_t* port_num = REGS_BASEADDR+PORT_NUM_OFFSET;
volatile uint32_t* regs = REGS_BASE_ADDRESS+PORT_REGS_OFFSET;

#define GOT_PKG(pn)       (regs[*port_num*(pn)+REQUEST_OFFSET]>>REQUEST_INDEX)&0x1
#define TADDR(pn)         (regs[*port_num*(pn)+RTADDR_OFFSET]>>TADDR_INDEX)&TADDR_MASK
#define CONTROL_TX(pn,cw) regs[*port_num*(pn)+RCONTROL_TX_OFFSET]|=(cw&CONTROL_TX_MASK)<<CONTROL_TX_INDEX
#define GRANT(pn)         regs[*port_num*(pn)+RGRANT_OFFSET]|=0x1&GRANT_INDEX
#define DISCARD(pn)       regs[*port_num*(pn)+RDISCARD_OFFSET]|=0x1&DISCARD_INDEX
#define AVAILABLE(pn)     (regs[*port_num*(pn)+RAVAILABLE_OFFSET]>>AVAILABLE_INDEX)&0x1

#endif

//////////////////////////////////////////////////////////////////
// routing table
struct Table {
    uint32_t target_port;
    uint32_t options_num;
    uint32_t* options;
};

// sw constants at predefined addresses
#ifdef TEST

uint32_t* table_size;
struct Table* table;
uint32_t* table_size_addr;
uint32_t* table_addr;

#endif

#ifndef TEST

volatile uint32_t table_size;
volatile Table* table = <define value somehow>; // make linter place this variable in separate region after all others

// these constants are unused in this software, but will be used by remote update software 
volatile uint32_t* table_size_addr = 0;
volatile uint32_t* table_addr = 1;
volatile uint32_t* table_region_max_range = 2; // make linter populate this as OCRAM_SIZE-<size of all regions except table region>

#endif

#endif
