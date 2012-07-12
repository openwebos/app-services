#!/bin/sh

export GTEST_OUTPUT=xml:$PWD/Test-Reports/
export LD_LIBRARY_PATH=/opt/Palm/luna/desktop-binaries:../mojomail-common/debug-linux-x86:/opt/Palm/gtest/lib

PREFIX=""

if [ "$1" = "-gdb" ]; then
  PREFIX="gdb --args"
fi

if [ "$1" = "-gdbrun" ]; then
  PREFIX="gdb -ex "run" --args"
fi

if [ "$1" = "-memcheck" ]; then
  PREFIX="valgrind --tool=memcheck --free-fill=0xDA --track-origins=yes"
fi

if [ "$1" = "-leakcheck" ]; then
  PREFIX="valgrind --tool=memcheck --leak-check=full --log-file=leak.log"
fi

if [ "$1" = "-callgrind" ]; then
  PREFIX="valgrind --tool=callgrind --callgrind-out-file=callgrind.cxxtest"
fi

$PREFIX debug-linux-x86/run_tests --gtest_print_time=0 "$@"
