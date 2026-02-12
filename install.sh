#!/bin/bash

# Este script va en el Raspberry Pi.
# Ejecutar como superuser, por favor.

# Instalar los requerimientos para que funcione el LoRaWAN:

apt update && apt -y upgrade
apt install -y python3-pip
pip3 install --upgrade setuptools
pip3 install --upgrade adafruit-python-shell
pip3 install adafruit-circuitpython-rfm9x


# Anadir la linea dtoverlay=gpio-shutdown en /boot/config.txt para
# habilitar el modo sleep. Tambien habilita la interfaz SPI:

CONF_DIR=/boot/config.txt
sed -i 's/disable_overscan=1/& \ndtoverlay=gpio-shutdown/' $CONF_DIR
sed -i 's/dtparam=spi=off/dtparam=spi=on/g' $CONF_DIR
sed -i 's/#dtparam=spi=on/dtparam=spi=on/g' $CONF_DIR


# Descargar el script de python y el servicio systemd e insertar los archivos necesarios en
# el directorio home:

LNK_LORA_SCRIPT=https://raw.githubusercontent.com/nisenare/comunitarias_IIPAO2022/main/lora.py
LNK_LORA_SERVICE=https://raw.githubusercontent.com/nisenare/comunitarias_IIPAO2022/main/lora.service
wget $LNK_LORA_SCRIPT
wget $LNK_LORA_SERVICE
OLD_IFS=$IFS
IFS='/'
read -a strarr <<< $(pwd)
THIS_USR=${strarr[2]}
IFS=$OLD_IFS
HOME_DIR="/home/$THIS_USR"
WRK_DIR="$HOME_DIR/WaterComsumptionPaipayales"
mkdir $WRK_DIR
chown $THIS_USR lora.py
chgrp $THIS_USR lora.py
touch $WRK_DIR/log.txt
chown $THIS_USR $WRK_DIR/log.txt
chgrp $THIS_USR $WRK_DIR/log.txt
mv lora.py $WRK_DIR


# Setear el servicio para que el script se ejecute automaticamente
# y se reinicie si hay errores de red. Rebootea el sistema luego de esto:

sed -i "s/pi/$THIS_USR/g" ./lora.service
mv lora.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable lora.service
systemctl start lora.service
reboot now
