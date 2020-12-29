#include <pthread.h>
#include <stdio.h>

#include "platform.h"
#include "loop.h"

//#define DEBUG

const uint32_t MINPORTS = 3; // minimum number of router ports to test
const uint32_t MAXPORTS = 32; // maximum number of router ports to test
const uint32_t MINLOGICADDRS = 0; // minimum number of logic paths per port in routing table
const uint32_t MAXLOGICADDRS = 3; // maximum number of logic paths per port in routing table
const uint32_t MINOPTIONSNUM = 1; // minimum number of options for target ports for a path
const uint32_t MAXOPTIONSNUM = 3; // maximum number of options for target ports for a path

void setup_router_regs (uint32_t pn) {
    uint32_t i;
#ifdef DEBUG
    printf ("setup_router_regs (%d)\n", pn_arg);
#endif
    port_num = malloc(sizeof(uint32_t));
    *port_num = pn;
    requests = malloc(pn*sizeof(uint32_t));
    taddrs = malloc(pn*sizeof(uint32_t));
    control = malloc(pn*sizeof(uint32_t));
    grant = malloc(pn*sizeof(uint32_t));
    discard = malloc(pn*sizeof(uint32_t));
    available = malloc(pn*sizeof(uint32_t));
    for (i=0;i<pn;i++) {
        requests[i] = 0;
        taddrs[i] = 0;
        control[i] = 0;
        grant[i] = 0;
        discard[i] = 0;
        available[i] = 0;
    }
}

void cleanup_router_regs () {
#ifdef DEBUG
    puts ("cleanup_router_regs");
#endif
    free (port_num);
    free (requests);
    free (taddrs);
    free (control);
    free (grant);
    free (discard);
    free (available);
}

struct Table* compose_routing_table (uint32_t pn, uint32_t lan, uint32_t options) {
    uint32_t rec_num = pn*(lan+1);
    struct Table* ret = malloc(rec_num*sizeof(struct Table));
#ifdef DEBUG
    printf ("compose_routing_table (%d, %d)\n", pn_arg, lan_arg);
#endif
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint32_t index;
    uint32_t tp;
    uint32_t tport;
    for (i=0;i<=lan;i++) {
        for (j=0;j<pn;j++) {
#ifdef DEBUG
            printf ("i=%d, j=%d\n", i, j);
#endif
            if (i == 0) { // wormhole addresses
                index = j;
                tp = j;
            } else { // logic addresses
                index = pn*i+j;
                tp = 32+(i-1)*pn+j;

            }
#ifdef DEBUG
            printf ("tp=%d, index=%d\n", tp, index);
#endif
            ret[index].target_port = tp;
            ret[index].options_num = options;
            ret[index].options = malloc(options*sizeof(uint32_t));
            for (k=0;k<options;k++) {
                tport = j+k;
                if (tport >= pn) {
                    tport -= pn;
                }
                ret[index].options[k]= tport;
#ifdef DEBUG
                printf ("record (%d): %d -> %d\n", k, tp, tport);
#endif
            }
        }
    }
    return ret;
}

void setup_routing_table (uint32_t pn, uint32_t lan, uint32_t options) {
    table_size_addr = malloc(sizeof(uint32_t));
    table_addr = malloc(sizeof(uint32_t));
    table_size = malloc (sizeof(uint32_t));
    uint32_t rec_num = pn*(lan+1);
    *table_size = rec_num;
    table = compose_routing_table (pn, lan, options);
    *table_size_addr = 0;
    *table_addr = 0;
}
 
void reload_table (uint32_t pn, uint32_t lan, uint32_t options) {
    uint32_t rec_num = pn*(lan+1);
    if (rec_num < *table_size) {
        puts ("reloading to a smaller table may cause segfault");
        exit (1);
    }
    struct Table* ptr = table;
    table = compose_routing_table (pn, lan, options);
    free (ptr);
    *table_size = rec_num;
}

void cleanup_routing_table () {
    uint32_t i;
    free (table_size_addr);
    free (table_addr);
#ifdef DEBUG
    puts ("cleanup_routing_table ()");
#endif
    for (i=0;i<*table_size;i++) {
        free (table[i].options);
    }
    free (table_size);
    free (table);
}

void setup (uint32_t pn, uint32_t lan, uint32_t options, pthread_t* tid) {
#ifdef DEBUG
    printf ("setup (%d, %d, %d)\n", pn, lan, tid);
#endif
    setup_router_regs (pn);
    setup_routing_table (pn, lan, options);
    enable = 1;
    pthread_create (tid, NULL, (void*)&loop, NULL);
}

void cleanup (pthread_t tid) {
#ifdef DEBUG
    puts ("cleanup ()");
#endif
    enable = 0;
    pthread_join (tid, NULL);
    cleanup_router_regs ();
    cleanup_routing_table ();
}

// TODO: replace while with something that won't hang up indefinitely
uint32_t positive_test (uint32_t sport, uint32_t addr, uint32_t tport) {
    uint32_t ret = 0;
#ifdef DEBUG
    printf ("positive_test (%d, %d, %d)\n", sport, addr, tport);
#endif
    grant[sport] = 0;
    control[tport] = 0;
    taddrs[sport] = addr;
    requests[sport] = 1;
    available[tport] = 1;
    while (grant[sport] == 0);
    requests[sport] = 0;
    available[tport] = 0;
    if (control[tport] != 1 << sport) {
        ret = 1;
        printf (
            "\nsport=%d, addr=%d, tport=%d, expected %d got %d",
            sport, addr, tport, 1<<sport, control[tport]
        );
        exit(1);
    }
    return ret;
}

// TODO: replace while with something that won't hang up indefinitely
uint32_t negative_test (uint32_t sport, uint32_t addr) {
    discard[sport] = 0;
    taddrs[sport] = addr;
    requests[sport] = 1;
    while (discard[sport] == 0);
    return 0;
}

uint32_t test (uint32_t pn, uint32_t lan, uint32_t on) {
#ifdef DEBUG
    printf ("test (%d, %d)\n", pn, lan);
#endif
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint32_t x;
    uint32_t sport;
    uint32_t addr;
    uint32_t tport;
    uint32_t ret = 0;
    for (i=0;i<=lan;i++) {
        for (j=0;j<pn;j++) {
            for (k=0;k<pn;k++) {
                for (x=0;x<on;x++) {
                    sport = k;
                    tport = j+x;
                    if (tport >= pn) {
                        tport = 0;
                    }
                    if (i == 0) {
                        addr = j;
                    } else {
                        addr = 32+(i-1)*pn+j;
                    }
#ifdef DEBUG
                    printf (
                         "params pn=%d, lan=%d, i=%d, j=%d, k=%d, sport=%d, addr=%d, tport=%d\n", 
                         pn, lan, i, j, k, sport, addr, tport
                    );
#endif
                    ret |= positive_test (sport, addr, tport);
                }
            }
        }
    }
    return ret;
}

uint32_t test_case (uint32_t pn, uint32_t lan, uint32_t on) {
#ifdef DEBUG
    printf ("test_case (pn=%d, lan=%d, on=%d) ", pn, lan, on);
#endif
    uint32_t ret;
    pthread_t tid;
    setup (pn, lan, on, &tid);
    ret = test (pn, lan, on);
#ifdef DEBUG
    if (ret == 0) {
        puts ("passed");
    } else {
        puts ("failed");
    }
#endif
    cleanup (tid);
    return ret;
}

uint32_t test_routing () {
#ifdef DEBUG
    puts ("test_routing");
#endif
    uint32_t pn;
    uint32_t lan;
    uint32_t on;
    uint32_t cnt = 1;
    uint32_t ret = 0;
    for (pn=MINPORTS;pn<=MAXPORTS;pn++) {
        for (lan=MINLOGICADDRS;lan<=MAXLOGICADDRS;lan++) {
            for (on=MINOPTIONSNUM;on<MAXOPTIONSNUM;on++) {
#ifndef DEBUG
                printf("\rtest case %d/%d", 
                    cnt, 
                    (MAXPORTS-MINPORTS)*(MAXPORTS-MINPORTS)*
                        (MAXLOGICADDRS-MINLOGICADDRS)*(MAXOPTIONSNUM-MINOPTIONSNUM)
                );
                fflush (stdout);
                cnt++;
#endif
                ret |= test_case (pn, lan, on);
            }
        }
    }
#ifndef DEBUG
    puts ("\n");
#endif
    return ret;
}

uint32_t test_table_reloading () {
    printf ("testing table reloading ");
    uint32_t ret;
    pthread_t tid;
    setup (3, 0, 1, &tid);
    negative_test (0, 32);
    reload_table (3, 1, 1);
    ret = positive_test (0, 32, 0);
    if (ret == 0) {
        puts ("passed");
    } else {
        puts ("failed");
    }
    cleanup (tid);
    return ret;
}

uint32_t test_vars_setup () {
    printf ("tesings vars setup ");
    pthread_t tid;
    setup (3, 0, 1, &tid);
    while (*table_size_addr == 0);
    while (*table_addr == 0);
    if (*table_size_addr != (uint32_t)&table_size) {
        puts ("failed (0)");
        printf ("%d %d\n", *table_size_addr, table_size);
        return 1;
    }
    if (*table_addr != (uint32_t)table) {
        puts ("failed (1)");
        return 1;
    }
    puts ("passed");
    cleanup (tid);
    return 0;
}

uint32_t main () {
    uint32_t ret = 0;
    puts ("started");
    ret |= test_routing ();
    ret |= test_table_reloading ();
    ret |= test_vars_setup ();
    puts ("done");
    return ret;
}

