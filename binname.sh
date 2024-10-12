#!/bin/sh

PARAMS_FILE="../Core/Inc/params.h"

DEVICE_NAME=$(cat $PARAMS_FILE | grep PARAMS_DEVICE_NAME | awk '{print $3}')
DEVICE_VER1=$(cat $PARAMS_FILE | grep PARAMS_FW_B1 | head -n1 | awk '{print $3}')
DEVICE_VER2=$(cat $PARAMS_FILE | grep PARAMS_FW_B2 | head -n1 | awk '{print $3}')
DEVICE_VER3=$(cat $PARAMS_FILE | grep PARAMS_FW_B3 | head -n1 | awk '{print $3}')
DEVICE_VER4=$(cat $PARAMS_FILE | grep PARAMS_FW_B4 | head -n1 | awk '{print $3}')

BIN="${DEVICE_NAME//\"}-${DEVICE_VER1}.${DEVICE_VER2}.${DEVICE_VER3}.${DEVICE_VER4}.bin"

cp "${1}.bin" $BIN
