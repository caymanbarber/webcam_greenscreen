#!/bin/bash

printf "Running script\n"

cmake -S . -B build/
cd build/
make
