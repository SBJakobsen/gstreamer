#!/bin/bash
gcc ctest.c `pkg-config --cflags --libs gstreamer-1.0 glib-2.0 gobject-2.0 libcrypto  libcurl  libssl  log4cplus  openssl`
#
#mpp related and avdec

# 
