#include "spacewire.h"
#include "stddef.h"

char read_byte(const char *const src) {
    if (src == NULL)
        while (1)
            ;

    return *src;
}

char null_byte(char *const src) {
    if (src == NULL)
        while (1)
            ;

    *src = '\0';

    return '\0';
}

char write_byte(char *const src, char byte) {
    if (src == NULL)
        while (1)
            ;

    *src = byte;

    return '\0';
}
