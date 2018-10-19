#!/bin/bash

echo ----------------------------------------
echo Architectures:
ls mk/arch | sed 's/\.mk//g'
echo Targets:
ls mk/target | sed 's/\.mk//g'
echo ----------------------------------------

echo ARCH?
read ARCH
echo TARGET?
read TARGET

echo "ARCH = $ARCH" >  config.mk
echo "TARGET = $TARGET" >> config.mk
