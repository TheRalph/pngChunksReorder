#!/bin/bash

rm -rf build
mkdir -p build
cd build
cmake ..

ln -s ../pngToReorder.png pngToReorder.png

cd ..
