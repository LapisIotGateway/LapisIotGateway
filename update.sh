#/bin/sh

PI=/home/pi
REPOSITORY=$PI/docomo/lpwagw
SCRIPTDIR=$PI/gateway
NODEREDDIR=$PI/.node-red

# update vnc
cp $REPOSITORY/Gateway/memo/lpwa_gw_start $PI
cp $REPOSITORY/lpwagw.desktop $PI/.config/autostart
cp $REPOSITORY/x11vnc.desktop $PI/.config/autostart

# update script
cp $REPOSITORY/Gateway/gw_peri $SCRIPTDIR
cp $REPOSITORY/Gateway/wifi_sw_ctrl $SCRIPTDIR
cp $REPOSITORY/Gateway/wifi_toggle.sh $SCRIPTDIR
cp $REPOSITORY/Gateway/rm92a_reset.sh $SCRIPTDIR
chmod +x $SCRIPTDIR/*

# update Node-RED
cp -r $REPOSITORY/Node-RED/nodes/* $NODEREDDIR/node_modules

# update conf
cp $REPOSITORY/Gateway/memo/99-local.rules /etc/udev/rules.d
cp $REPOSITORY/Gateway/memo/rc.local /etc

# reboot
reboot
