#!/bin/bash
# path to clang++, llvm-dis, and opt
LLVM_BIN=/LLVM_ROOT/build/bin
# path to CSE231.so
LLVM_SO=/LLVM_ROOT/build/lib
# path to lib231.c
LIB_DIR=/lib231
# path to the test directory
TEST_DIR=.
#echo "mine:"
#$LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-liveness < $1 > /dev/null  2> l.out
#$LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-maypointto <  $1  2> m.out

#echo "solution:"
#/solution/opt -cse231-liveness < $1  > /dev/null 2> sl.out
#/solution/opt -cse231-maypointto <  $1  > /dev/null 2> sm.out

echo "mine:"
$LLVM_BIN/opt  -load $LLVM_SO/CSE231.so -cse231-maypointto < $1 
echo "solution:"
/solution/opt -cse231-maypointto <  $1 

