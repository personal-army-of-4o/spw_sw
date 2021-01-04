#include <pthread.h>
#include <stdio.h>

#include "platform.h"
#include "loop.h"
#include "debug.h"

const uint32_t MINPORTS = 3; // minimum number of router ports to test
const uint32_t MAXPORTS = 32; // maximum number of router ports to test
const uint32_t MINLOGICADDRS = 0; // minimum number of logic paths per port in routing table
const uint32_t MAXLOGICADDRS = 3; // maximum number of logic paths per port in routing table
const uint32_t MINOPTIONSNUM = 1; // minimum number of options for target ports for a path
const uint32_t MAXOPTIONSNUM = 3; // maximum number of options for target ports for a path

void setup_router_regs(uint32_t pn) {
    port_num = malloc(sizeof(uint32_t));
    *port_num = pn;
    requests = calloc(pn, sizeof(uint32_t));
    taddrs = calloc(pn, sizeof(uint32_t));
    control = calloc(pn, sizeof(uint32_t));
    grant = calloc(pn, sizeof(uint32_t));
    discard = calloc(pn, sizeof(uint32_t));
    available = calloc(pn, sizeof(uint32_t));
}

void cleanup_router_regs() {
    log_info("cleanup_router_regs");
    free(port_num);
    free(requests);
    free(taddrs);
    free(control);
    free(grant);
    free(discard);
    free(available);
}

struct Table* compose_routing_table(uint32_t pn, uint32_t lan, uint32_t options) {
    uint32_t rec_num = pn * (lan + 1);
    struct Table* ret = calloc(rec_num, sizeof(struct Table));
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint32_t index;
    uint32_t tp;
    uint32_t tport;
    for (i=0;i<=lan;i++) {
        for (j=0;j<pn;j++) {
            log_debug("i=%d, j=%d\n", i, j);
            if (i == 0) { // wormhole addresses
                index = j;
                tp = j;
            } else { // logic addresses
                index = pn*i+j;
                tp = 32+(i-1)*pn+j;

            }
            log_debug("tp=%d, index=%d\n", tp, index);
            ret[index].target_port = tp;
            ret[index].options_num = options;
            ret[index].options = calloc(options, sizeof(uint32_t));
            for (k=0;k<options;k++) {
                tport = j+k;
                if (tport >= pn) {
                    tport -= pn;
                }
                ret[index].options[k]= tport;
                log_debug("record(%d): %d -> %d\n", k, tp, tport);
            }
        }
    }
    return ret;
}

void setup_routing_table(uint32_t pn, uint32_t lan, uint32_t options) {
    table_size_addr = malloc(sizeof(uint32_t));
    table_addr = malloc(sizeof(uint32_t));
    table_size = malloc(sizeof(uint32_t));
    uint32_t rec_num = pn*(lan+1);
    *table_size = rec_num;
    table = compose_routing_table(pn, lan, options);
    *table_size_addr = 0;
    *table_addr = 0;
}

void reload_table(uint32_t pn, uint32_t lan, uint32_t options) {
    uint32_t rec_num = pn*(lan+1);
    if (rec_num < *table_size) {
        log_info("reloading to a smaller table may cause segfault");
        exit(1);
    }
    struct Table* ptr = table;
    table = compose_routing_table(pn, lan, options);
    free(ptr);
    *table_size = rec_num;
}

void cleanup_routing_table() {
    uint32_t i;
    free(table_size_addr);
    free(table_addr);
    log_info("cleanup_routing_table()");
    for (i=0;i<*table_size;i++) {
        free(table[i].options);
    }
    free(table_size);
    free(table);
}

void setup(uint32_t pn, uint32_t lan, uint32_t options, pthread_t* tid) {
    log_info("setup(%d, %d, %d)\n", pn, lan, (int) tid);
    setup_router_regs(pn);
    setup_routing_table(pn, lan, options);
    enable = 1;
    // cast to stop compiler from complaining
    pthread_create(tid, NULL, (void *(*)(void *)) loop, NULL);
}

void cleanup(pthread_t tid) {
    log_info("cleanup()");
    enable = 0;
    pthread_join(tid, NULL);
    cleanup_router_regs();
    cleanup_routing_table();
}

// TODO: replace while with something that won't hang up indefinitely
uint32_t positive_test(uint32_t sport, uint32_t addr, uint32_t tport) {
    uint32_t ret = 0;
    log_debug("positive_test(%d, %d, %d)\n", sport, addr, tport);
    grant[sport] = 0;
    control[tport] = 0;
    taddrs[sport] = addr;
    requests[sport] = 1;
    available[tport] = 1;
    while (grant[sport] == 0);
    requests[sport] = 0;
    available[tport] = 0;
    if (control[tport] != (1 << sport)) {
        ret = 1;
        log_err(
            "\nsport=%d, addr=%d, tport=%d, expected %d got %d",
            sport, addr, tport, 1<<sport, control[tport]
        );
        exit(1);
    }
    return ret;
}

// TODO: replace while with something that won't hang up indefinitely
uint32_t negative_test(uint32_t sport, uint32_t addr) {
    discard[sport] = 0;
    taddrs[sport] = addr;
    requests[sport] = 1;
    while (discard[sport] == 0);
    return 0;
}

uint32_t test(uint32_t pn, uint32_t lan, uint32_t on) {
    log_info("test(%d, %d)\n", pn, lan);
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
                    log_debug(
                         "params pn=%d, lan=%d, i=%d, j=%d, k=%d, sport=%d, addr=%d, tport=%d\n",
                         pn, lan, i, j, k, sport, addr, tport
                    );
                    ret |= positive_test(sport, addr, tport);
                }
            }
        }
    }
    return ret;
}

uint32_t test_case(uint32_t pn, uint32_t lan, uint32_t on) {
    log_info("test_case(pn=%d, lan=%d, on=%d) ", pn, lan, on);
    uint32_t ret;
    pthread_t tid;
    setup(pn, lan, on, &tid);
    ret = test(pn, lan, on);
    if (ret == 0) {
        log_info("passed");
    } else {
        log_err("failed");
    }
    cleanup(tid);
    return ret;
}

uint32_t test_routing() {
    log_info("test_routing");
    uint32_t pn;
    uint32_t lan;
    uint32_t on;
    uint32_t cnt = 1;
    uint32_t ret = 0;
    for (pn=MINPORTS;pn<=MAXPORTS;pn++) {
        for (lan=MINLOGICADDRS;lan<=MAXLOGICADDRS;lan++) {
            for (on=MINOPTIONSNUM;on<MAXOPTIONSNUM;on++) {
                log_info("\rtest case %d/%d",
                    cnt,
                    (MAXPORTS-MINPORTS)*(MAXPORTS-MINPORTS)*
                        (MAXLOGICADDRS-MINLOGICADDRS)*(MAXOPTIONSNUM-MINOPTIONSNUM)
                );
                fflush(stdout);
                cnt++;
                ret |= test_case(pn, lan, on);
            }
        }
    }
    log_info("\n");
    return ret;
}

uint32_t test_table_reloading() {
    log_info("testing table reloading ");
    uint32_t ret;
    pthread_t tid;
    setup(3, 0, 1, &tid);
    negative_test(0, 32);
    reload_table(3, 1, 1);
    ret = positive_test(0, 32, 0);
    if (ret == 0) {
        log_info("passed");
    } else {
        log_err("failed");
    }
    cleanup(tid);
    return ret;
}

uint32_t test_vars_setup() {
    log_info("tesings vars setup ");
    pthread_t tid;
    setup(3, 0, 1, &tid);
    while (*table_size_addr == 0);
    while (*table_addr == 0);
    if (*table_size_addr != (uint32_t)&table_size) {
        log_err("failed(0)");
        log_err("%d %d\n", *table_size_addr, (int) table_size);
        return 1;
    }
    if (*table_addr != (uint32_t)table) {
        log_err("failed(1)");
        return 1;
    }
    log_info("passed");
    cleanup(tid);
    return 0;
}

int main() {
    uint32_t ret = 0;
    log_info("started");
    ret |= test_routing();
    ret |= test_table_reloading();
    ret |= test_vars_setup();
    log_info("done");
    return ret;
}

