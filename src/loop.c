// this is a default router firmware supporting only wormhole routing and rmap on port 0

#include <stdint.h>

#include "platform.h"

//#define DEBUG

inline int32_t get_path (uint32_t taddr);

void loop () {
    int i;
    uint32_t taddr;
    int32_t found_address;

    // setup
    *table_size_addr = (uint32_t)&table_size;
    *table_addr = (uint32_t)table;

    while (enable) {
        for (i=0;i<*port_num;i++) {
            if (GOT_PKG(i)) {
                taddr = TADDR(i);
                found_address = get_path(taddr);
#ifdef DEBUG
                printf ("got pkg from port %d taddr=%d, tport=%d\n", i, taddr, found_address);
                GOT_PKG(i) = 0;
#endif
                if (found_address >= 0) {
                    CONTROL_TX(found_address, 1 << i);
                    GRANT(i) = 1;
#ifdef DEBUG
                    GOT_PKG(i) = 0;
                    printf ("granted %d, %d\n", found_address, 1<<i);
#endif
                } else if (found_address == -2) {
                    DISCARD(i) = 1;
#ifdef DEBUG
                    puts ("discard");
#endif
                }
            }
        }
    }
}

inline int32_t get_path (uint32_t taddr) {
    uint32_t i;
    uint32_t j;
    uint32_t on;
    uint32_t pn;
    uint32_t ret = -2;
    for (i=0;i<*table_size;i++) {
        if (table[i].target_port == taddr) {
            ret = -1;
            on = table[i].options_num;
            for (j=0;j<on;j++) {
                pn=table[i].options[j];
#ifdef DEBUG
                printf ("option %d = %d available %d\n", j, pn, AVAILABLE(pn));
#endif
                if (AVAILABLE(pn)) {
                    return pn;
                }
            }
        }
    }
    return ret;
}
