#!/bin/sh

PYTHON=C:/Users/user/AppData/Local/Programs/Python/Python310/python.exe

BOOTLOADER=../../umeter-bootloader/Debug/umeter-bootloader.bin
APPLICATION=./umeter.bin
BLSIZE=8192

$PYTHON ../binmerger.py -s $BLSIZE -b $BOOTLOADER -a $APPLICATION
