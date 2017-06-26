#!/bin/bash

export CXX=clang++
cmake -DCMAKE_BUILD_TYPE=Release
make -j8
