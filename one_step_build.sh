#!/bin/bash

mkdir -p m4
cd "`dirname $BASH_SOURCE`"
./autogen.sh
./configure
make

