#!/bin/bash

# Install required packages
echo "Installing packages..."

pkgs=(git make gcc pkgconf gcc-avr avr-libc libelf-dev freeglut3-dev libncurses-dev gdb-avr)
sudo apt-get -y --ignore-missing install "${pkgs[@]}"

echo "Packages installed successfully."

# Clone and initialize the simulator
echo "Cloning simavr repository..."

if [ ! -d simavr ]; then
    git clone https://github.com/akosthekiss/simavr.git
else
    echo "simavr repository already exists."
fi

cd simavr
git checkout olimex-avr-mt128

# Pass linker options
export LDFLAGS="-Wl,--copy-dt-needed-entries"

make

echo "simavr repository initialized successfully."
