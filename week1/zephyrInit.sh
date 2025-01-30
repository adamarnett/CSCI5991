#!/bin/bash

# make sure packages are up to date
sudo apt update
sudo apt upgrade

# necessary dependencies for ubuntu < 22.04
wget https://apt.kitware.com/kitware-archive.sh
sudo bash kitware-archive.sh

# necessary dependencies for all
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

# check if stuff installed correctly
cmake --version
if [ $? -ne 0 ] then
    echo "ERROR: cmake not installed. Exiting."
    exit
fi

python3 --version
if [ $? -ne 0 ] then
    echo "ERROR: python3 not installed. Exiting."
    exit
fi

dtc --version
if [ $? -ne 0 ] then
    echo "ERROR: dtc not installed. Exiting."
    exit
fi

# get python virtual environment
sudo apt install python3-venv
# check if installed
if [ $? -ne 0 ] then
    echo "ERROR: python3-venv not installed. Exiting."
    exit
fi

# make a new python virtual environment for zephyr
python3 -m venv ~/zephyrproject/.venv
# check if created
$DIREC="~/zephyrproject.venv"
if [ ! -d "$DIREC" ] then
    echo "ERROR: Unable to create virtual environment. Exiting."
    exit
fi

# start the new virtual environment
source ~/zephyrproj.venv/bin/activate
# check if started
if [ $? -ne 0 ] then
    echo "ERROR: python virtual environment not activated. Exiting."
    exit
fi

# install west for the current virtual environment
pip install west
# check if installed
west --version
if [ $? -ne 0 ] then
    echo "ERROR: west not installed. Exiting."
    exit
fi














