#!/bin/bash
echo "building"
gcc -std=gnu90 -pthread -Itest -Isrc src/*.c test/main_x86.c
ret=$?
if [ "$?" == "0" ]; then
    echo launching
	./a.out
    exit $?
fi
exit $ret
