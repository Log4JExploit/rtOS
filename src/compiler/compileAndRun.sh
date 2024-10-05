#!/bin/bash

clear
g++ -g --output ./build/compiler.o ./compiler.cpp
if [[ $? == 0 ]]; then
   ./build/compiler.o $@
   echo "Compiler finished with code: $?"
else
   echo "Compiled with errors: aborting execution!"
fi
