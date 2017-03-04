#/bin/bash

PI=/home/pi
REPO=$PI/docomo/lpwgateway

cd $REPO

git pull

bash $REPO/update.sh

