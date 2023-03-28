#!/bin/bash
rm -r /data/*
./a.out | tee /data/fps.log
#
#mpp related and avdec