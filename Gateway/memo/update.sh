#/bin/bash

PI=/home/pi
REPO=$PI/docomo/lpwagw

cd $REPO

git pull

bash $REPO/update.sh

