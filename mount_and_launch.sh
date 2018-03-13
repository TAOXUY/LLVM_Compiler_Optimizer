#!/bin/bash

SOURCE_PATH=`pwd`/Passes
TESTS_PATH=`pwd`/Tests
OUTPUT_PATH=`pwd`/Output
BASH_PATH=`pwd`/.bash_aliases
LIB231_PATH=`pwd`/lib231.cpp

# MOUNT POINTS IN CONTAINER:
# --------------------------
#
# SOURCE CODE: /LLVM_ROOT/llvm/lib/Transforms/CSE231_Project
# TESTS: /tests
# OUTPUT: /output

docker run --rm -it -v ${LIB231_PATH}:/lib231/lib231.cpp -v ${BASH_PATH}:/root/.bash_aliases -v ${SOURCE_PATH}:/LLVM_ROOT/llvm/lib/Transforms/CSE231_Project -v ${TESTS_PATH}:/tests -v ${OUTPUT_PATH}:/output prodromou87/llvm:5.0.1
