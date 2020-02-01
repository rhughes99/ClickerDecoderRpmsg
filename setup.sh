#!/bin/bash
export PRUN=0
export TARGET=ClickDecPru
echo PRUN=$PRUN
echo TARGET=$TARGET


# Configure PRU pins
config-pin P9_31 pruin
config-pin P9_28 pruout
config-pin P9_29 pruout

