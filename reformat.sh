#!/bin/bash 
find . -regex '.*\.\(cpp\|hpp\)' -exec clang-format -style=file -i {} \;

