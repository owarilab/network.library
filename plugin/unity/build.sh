#!/bin/bash

make clean
make
make clean -f Makefile_Linux
make -f Makefile_Linux
