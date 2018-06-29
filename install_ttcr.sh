#!/bin/bash

cd ttcr
cmake ./
make
cd ../ttcrpy
python setup.py build_ext --inplace
cd ../
