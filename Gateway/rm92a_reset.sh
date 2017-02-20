#!/bin/sh

PATH=/sys/class/gpio
NUM=22
TARGET=gpio${NUM}

echo 0 > $PATH/$TARGET/value

/bin/sleep 1

echo 1 > $PATH/$TARGET/value

/bin/sleep 1

exit 0