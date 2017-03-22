#/bin/sh

PI=/home/pi
REPOSITORY=$PI/docomo/lpwagw
SCRIPTDIR=$PI/gateway
NODEREDDIR=$PI/.node-red

askYesOrNo() {
	while true; do
		read -p  "$1 (y/n)?" anser
		case $anser in
			[[yY] | [yY]es | NO )
				return 0;;
			[nN] | [nN]o | NO )
				return 1;;
			* ) echo "Please answer yes or no.";;
		esac
	done
}

echo ""
echo ""
echo "==============================================="
echo "====      download program from github     ===="
echo "==============================================="
cd $REPOSITORY
git pull

echo ""
echo ""
echo ""
echo "==============================================="
echo "==== overwrite program to executing folder ===="
echo "==============================================="
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

echo "==============================================="
echo "====             end of update             ===="
echo "==============================================="
echo ""

askYesOrNo "Restart now?"
if [ $? -eq 0 ]; then
	echo " restart now program 3sec later"
	sleep 3
	sudo reboot
else
	echo "not restart now"
	echo "please restart later"
fi

# reboot

#sudo reboot
