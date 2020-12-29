#!/bin/bash
echo "building"
gcc -DTEST -std=gnu90 -pthread -Itest -Isrc src/*.c test/main_x86.c
ret=$?
if [ "$ret" == "0" ]; then
    echo launching
	./a.out
    exit $?
fi
exit $ret
