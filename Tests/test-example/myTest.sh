#!/bin/bash

# path to clang++, llvm-dis, and opt
LLVM_BIN=/LLVM_ROOT/build/bin
# path to CSE231.so
LLVM_SO=/LLVM_ROOT/build/lib
# path to lib231.c
LIB_DIR=/lib231
# path to the test directory
TEST_DIR=.
# cat $LLVM_SO/CSE231.so
# cat /LLVM_ROOT/llvm/test/Transforms/MemCpyOpt/multiple-phis.ll
# $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-liveness < /LLVM_ROOT/llvm/test/Transforms/SimplifyCFG/multiple-phis.ll > /dev/null  2> l.out
# $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-maypointto < /LLVM_ROOT/llvm/test/Transforms/SimplifyCFG/multiple-phis.ll  > /dev/null 2> m.out
# /solution/opt -cse231-liveness < /LLVM_ROOT/llvm/test/Transforms/SimplifyCFG/multiple-phis.ll  > /dev/null 2> sl.out
# /solution/opt -cse231-maypointto < /LLVM_ROOT/llvm/test/Transforms/SimplifyCFG/multiple-phis.ll  > /dev/null 2> sm.out
# echo "====Liveness======"
# diff l.out sl.out
# echo "====MayPointTo====="
# diff m.out sm.out
count1=0
count2=0
for file in testDir/*
do
#                 echo "$file"
    $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-liveness < "$file" > /dev/null  2> l.out
    $LLVM_BIN/opt -load $LLVM_SO/CSE231.so -cse231-maypointto <  "$file" > /dev/null 2> m.out
    /solution/opt -cse231-liveness < "$file"  > /dev/null 2> sl.out
    /solution/opt -cse231-maypointto < "$file"  > /dev/null 2> sm.out
    #     echo "====Liveness======"
    diff l.out sl.out > d1.out
    #     echo "====MayPointTo====="
    diff m.out sm.out > d2.out
    if [ -s "d1.out" ]
    then
#                 echo "liveness"
touch "diffOut/${file}_live_diff.out"

       cat "d1.out" > "diffOut/${file}_live_diff.out"
        count1=${count1+1};
    fi
    if [ -s "d2.out" ]
    then
#                 echo "may"
touch "diffOut/${file}_may_diff.out"
       cat "d2.out" > "diffOut/${file}_may_diff.out"
        count2=${count2+1};
    fi
done
echo "live $count1"
echo "may $count2"
