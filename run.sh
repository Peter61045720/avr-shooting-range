#!/bin/bash

if [ ! -d simavr ]; then
    echo "Error: 'simavr' repository not found."
    exit 1
fi

if [ ! -d build ]; then
    echo "Error: 'build' directory not found."
    exit 1
fi

cp ./build/atmega128_shooting_range.axf ./simavr/examples/board_olimex_avr_mt128/

cd simavr/examples/board_olimex_avr_mt128

./obj-x86_64-linux-gnu/mt128.elf ./atmega128_shooting_range.axf
