#/bin/sh

PI=/home/pi
REPOSITORY=$PI/docomo/lpwgateway
SCRIPTDIR=$PI/gateway
NODEREDDIR=$PI/.node-red/node_modules

# update script
cp $REPOSITORY/Gateway/gw_peri $SCRIPTDIR
cp $REPOSITORY/Gateway/wifi_sw_ctrl $SCRIPTDIR
cp $REPOSITORY/Gateway/wifi_toggle.sh $SCRIPTDIR
cp $REPOSITORY/Gateway/rm92a_reset.sh $SCRIPTDIR
chmod +x $SCRIPTDIR/*

# update Node-RED
cp -r $REPOSITORY/Node-RED/nodes/* $NODEREDDIR

# update conf
cp $REPOSITORY/Gateway/memo/99-local.rules /etc/udev/rules.d
cp $REPOSITORY/Gateway/memo/rc.local /etc

# reboot
reboot
