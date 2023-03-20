#!/bin/bash
gcc ctest.c `pkg-config --cflags --libs gstreamer-1.0`
