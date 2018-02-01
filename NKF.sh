#!/bin/sh

find . -name  "*.c" -o -name "*.cpp" -o -name "*.css" -o -name "*.h" -o -name "*.sh" | xargs -n 100 nkf -e -Lu -d -x --overwrite;
