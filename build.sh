#!/bin/bash

mkdir -p build

if [ ! -d simavr ]; then
    echo "Error: 'simavr' repository not found."
    exit 1
fi

cp ./src/atmega128_shooting_range.c ./simavr/examples/board_olimex_avr_mt128/

cd simavr/examples/board_olimex_avr_mt128

# Pass linker options
export LDFLAGS="-Wl,--copy-dt-needed-entries"

make

cp atmega128_shooting_range.axf ../../../build/
