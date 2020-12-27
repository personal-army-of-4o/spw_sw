#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include "../src/spacewire.h"

static char my_char = 'A';

int main()
{
    printf("*** start test ***\n");

    assert(my_char == read_byte(&my_char));

    null_byte(&my_char);
    assert('\0' == my_char);

    write_byte(&my_char, 'B');
    assert('B' == my_char);

    printf("*** all tests passed! ***\n");

    return 0;
}
