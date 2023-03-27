#!/bin/bash
gcc -g ctest.c `pkg-config --cflags --libs gstreamer-1.0` -o debug 
#
#mpp related and avdec

# 
