#!/bin/bash

g++ -std=c++11 -o bulkprint src/main.cpp -I../libharu/include -L../libharu/build/src -lhpdf -lz
