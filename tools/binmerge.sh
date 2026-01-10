#!/bin/sh
# Run from folder "Debug"

PYTHON=C:/Users/user/AppData/Local/Programs/Python/Python39/python.exe

BOOTLOADER=../../umeter-bootloader/Debug/umeter-bootloader.bin
APPLICATION=./umeter.bin
BLSIZE=32768

$PYTHON ../tools/binmerger.py -s $BLSIZE -b $BOOTLOADER -a $APPLICATION
