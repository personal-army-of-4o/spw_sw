#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include "../src/spacewire.h"
#include "debug.h"

static char my_char = 'A';

int main()
{
    log_info("*** start test ***\n");

    assert(my_char == read_byte(&my_char));

    null_byte(&my_char);
    assert('\0' == my_char);

    write_byte(&my_char, 'B');
    assert('B' == my_char);

    log_info("*** all tests passed! ***\n");

    return 0;
}
