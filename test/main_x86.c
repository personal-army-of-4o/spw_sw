// artistic description of this testbench
// main
// |-setup
// | |-setup_router_regs - allocates and zeros out memory for 
// | |                     router memory-mapped status/control
// | |-setup_routing_table - allocates and initializes "const" variables
// |-run_tests - loops through test parameters
// | |-test case - sets up testbench and spawns uut
// |   |-test - loops through sub-tests for one set of parameters
// |     |-one_tests - stimulates uut and checks reply
// |-cleanup - cleans up after a test case
// | |-cleanup_router_regs - cleans up what setup_router_regs allocated
// | |-cleanup_routing_table - cleans up what setup_routing_table allocated

#include <pthread.h>
#include <stdio.h>

#include "platform.h"
#include "loop.h"

//#define DEBUG

const int MINPORTS = 3;
const int MAXPORTS = 32;
const int MINLOGICADDRS = 0;
const int MAXLOGICADDRS = 3;
const int MINOPTIONSNUM = 1;
const int MAXOPTIONSNUM = 3;

void setup_router_regs (int pn_arg) {
    int i;
#ifdef DEBUG
    printf ("setup_router_regs (%d)\n", pn_arg);
#endif
    uint32_t pn = (uint32_t)pn_arg;
    PORT_NUM = malloc(sizeof(uint32_t));
    *PORT_NUM = pn;
    requests = malloc(pn*sizeof(uint32_t));
    taddrs = malloc(pn*sizeof(uint32_t));
    control = malloc(pn*sizeof(uint32_t));
    grant = malloc(pn*sizeof(uint32_t));
    discard = malloc(pn*sizeof(uint32_t));
    available = malloc(pn*sizeof(uint32_t));
    for (i=0;i<pn_arg;i++) {
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
    free (PORT_NUM);
    free (requests);
    free (taddrs);
    free (control);
    free (grant);
    free (discard);
    free (available);
}

void setup_routing_table (int pn_arg, int lan_arg, int options) {
#ifdef DEBUG
    printf ("setup_routing_table (%d, %d)\n", pn_arg, lan_arg);
#endif
    uint32_t pn = (uint32_t) pn_arg;
    uint32_t lan = (uint32_t) lan_arg;
    uint32_t rec_num = pn*(lan+1);
    int i;
    int j;
    int k;
    int index;
    uint32_t tp;
    uint32_t tport;
    TABLE_SIZE = malloc (sizeof(uint32_t));
    *TABLE_SIZE = rec_num;
    table = malloc(rec_num*sizeof(struct Table_top));
    for (i=0;i<=lan;i++) {
        for (j=0;j<pn;j++) {
#ifdef DEBUG
            printf ("i=%d, j=%d\n", i, j);
#endif
            if (i == 0) { // wormhole addresses
                index = j;
                tp = (uint32_t)j;
            } else { // logic addresses
                index = pn*i+j;
                tp = 32+(i-1)*pn+j;

            }
#ifdef DEBUG
            printf ("tp=%d, index=%d\n", tp, index);
#endif
            table[index].target_port = tp;
            table[index].options_num = (uint32_t)options;
            table[index].options = malloc(options*sizeof(struct Table_bottom));
            for (k=0;k<options;k++) {
                tport = j+k;
                if (tport >= pn) {
                    tport -= pn;
                }
                table[index].options[k].port_num = tport;
#ifdef DEBUG
                printf ("record (%d): %d -> %d\n", k, tp, tport);
#endif
            }
        }
    }
}

void cleanup_routing_table () {
    int i;
#ifdef DEBUG
    puts ("cleanup_routing_table ()");
#endif
    for (i=0;i<*TABLE_SIZE;i++) {
        free (table[i].options);
    }
    free (TABLE_SIZE);
    free (table);
}

void setup (int pn, int lan, int options, pthread_t* tid) {
#ifdef DEBUG
    printf ("setup (%d, %d, %d)\n", pn, lan, tid);
#endif
    setup_router_regs (pn);
    setup_routing_table (pn, lan, options);
    mytrue = 1;
    pthread_create (tid, NULL, loop, NULL);
}

void cleanup (pthread_t tid) {
#ifdef DEBUG
    puts ("cleanup ()");
#endif
    mytrue = 0;
    pthread_join (tid, NULL);
    cleanup_router_regs ();
    cleanup_routing_table ();
}

int one_test (int sport, int addr, int tport) {
    int ret = 0;
#ifdef DEBUG
    printf ("one_test (%d, %d, %d)\n", sport, addr, tport);
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

int test (int pn, int lan, int on) {
#ifdef DEBUG
    printf ("test (%d, %d)\n", pn, lan);
#endif
    int i;
    int j;
    int k;
    int x;
    uint32_t sport;
    uint32_t addr;
    uint32_t tport;
    int ret = 0;
    for (i=0;i<=lan;i++) {
        for (j=0;j<pn;j++) {
            for (k=0;k<pn;k++) {
                for (x=0;x<on;x++) {
                    sport = k;
                    tport = j+(uint32_t)x;
                    if (tport >= (uint32_t)pn) {
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
                    ret |= one_test (sport, addr, tport);
                }
            }
        }
    }
    return ret;
}

int test_case (int pn, int lan, int on) {
    printf ("test_case (pn=%d, lan=%d, on=%d) ", pn, lan, on);
    int ret;
    pthread_t tid;
    setup (pn, lan, on, &tid);
    ret = test (pn, lan, on);
    if (ret == 0) {
        puts ("passed");
    } else {
        puts ("failed");
    }
    cleanup (tid);
    return ret;
}

int run_tests () {
#ifdef DEBUG
    puts ("run_tests");
#endif
    int pn;
    int lan;
    int on;
    int ret = 0;
    for (pn=MINPORTS;pn<=MAXPORTS;pn++) {
        for (lan=MINLOGICADDRS;lan<=MAXLOGICADDRS;lan++) {
            for (on=MINOPTIONSNUM;on<MAXOPTIONSNUM;on++) {
                ret |= test_case (pn, lan, on);
            }
        }
    }
    return ret;
}

int main () {
    int ret;
    // pre-test setup
    // tests
    ret = run_tests ();
    // post-test setup
    puts ("done");
    return ret;
}


























