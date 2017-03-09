#/bin/sh

PI=/home/pi
REPOSITORY=$PI/docomo/lpwagw
SCRIPTDIR=$PI/gateway
NODEREDDIR=$PI/.node-red

# update vnc
cp $REPOSITORY/Gateway/memo/lpwa_gw_start $PI
cp $REPOSITORY/Gateway/memo/lpwagw.desktop $PI/.config/autostart
cp $REPOSITORY/Gateway/memo/x11vnc.desktop $PI/.config/autostart

# update script
sudo rm -rf $SCRIPTDIR
sudo mkdir $SCRIPTDIR
sudo cp $REPOSITORY/Gateway/gw_peri $SCRIPTDIR
sudo cp $REPOSITORY/Gateway/wifi_sw_ctrl $SCRIPTDIR
sudo cp $REPOSITORY/Gateway/wifi_toggle.sh $SCRIPTDIR
sudo cp $REPOSITORY/Gateway/rm92a_reset.sh $SCRIPTDIR

# update Node-RED
cp -r $REPOSITORY/Node-RED/nodes/* $NODEREDDIR/node_modules

# update conf
sudo cp $REPOSITORY/Gateway/memo/99-local.rules /etc/udev/rules.d
sudo cp $REPOSITORY/Gateway/memo/rc.local /etc

# reboot
sudo reboot
