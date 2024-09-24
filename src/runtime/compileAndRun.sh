#!/bin/bash

clear
g++ --output ./build/runtime.o ./runtime.cpp
if [[ $? == 0 ]]; then
   ./build/runtime.o $@
   echo "Runtime finished with code: $?"
else
   echo "Compilation with errors: aborting execution!"
fi
