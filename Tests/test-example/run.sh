#!/bin/bash

# path to clang++, llvm-dis, and opt
LLVM_BIN=/LLVM_ROOT/build/bin
# path to CSE231.so
LLVM_SO=/LLVM_ROOT/build/lib
# path to lib231.c
LIB_DIR=/lib231
# path to the test directory
TEST_DIR=.



$LLVM_BIN/clang -c -O0 $TEST_DIR/test1.c -emit-llvm -S -o /tmp/test1-c.ll

#resgiter pass, my pass is in CSE231.so 
$LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2> /tmp/csi.result


#get test.bc
$LLVM_BIN/clang++ -c -O0 $TEST_DIR/test1.cpp -emit-llvm -S -o /tmp/test1.ll
#get lib.bc
$LLVM_BIN/clang++ -c $LIB_DIR/lib231.cpp -emit-llvm -S -o /tmp/lib231.ll
#get main.bc
$LLVM_BIN/clang++ -c $TEST_DIR/test1-main.cpp -emit-llvm -S -o /tmp/test1-main.ll


# $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-cdi < /tmp/test1.ll -o /tmp/test1-cdi.bc
# $LLVM_BIN/llvm-dis /tmp/test1-cdi.bc
# $LLVM_BIN/clang++ /tmp/lib231.ll /tmp/test1-cdi.ll /tmp/test1-main.ll -o /tmp/cdi_test1
# /tmp/cdi_test1 2> /tmp/cdi.result
#
#
# $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-bb < /tmp/test1.ll -o /tmp/test1-bb.bc
# $LLVM_BIN/llvm-dis /tmp/test1-bb.bc
# $LLVM_BIN/clang++ /tmp/lib231.ll /tmp/test1-bb.ll /tmp/test1-main.ll -o /tmp/bb_test1
# /tmp/bb_test1 2> /tmp/bb.result

#$LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-reaching < /tmp/test1.ll -o /tmp/test1-rda.bc
$LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-reaching < /LLVM_ROOT/llvm/test/Transforms/SimplifyCFG/multiple-phis.ll -o /tmp/test1-rda.bc
$LLVM_BIN/llvm-dis /tmp/test1-rda.bc
$LLVM_BIN/clang++ /tmp/lib231.ll /tmp/test1-rda.ll /tmp/test1-main.ll -o /tmp/rda_test1

#run
/tmp/rda_test1 2> /tmp/rda.result
