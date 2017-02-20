#!/bin/sh

if [ $1 = "on" ]; then
    /sbin/ifconfig wlan0 up >/dev/null 2>&1
    # echo 1 > /proc/sys/net/ipv4/ip_forward
    service hostapd start
    /bin/sleep 10
    service isc-dhcp-server start
else
    service isc-dhcp-server stop
    service hostapd stop
    # echo 0 > /proc/sys/net/ipv4/ip_forward
    /sbin/ifconfig wlan0 down >/dev/null 2>&1
fi

exit 0