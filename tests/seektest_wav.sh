#!/bin/sh

toppkgdir=${srcdir:-.}/..

#--gst-debug=neonhttpsrc:5
#--gst-debug-no-color
#gst_params="--gst-debug-level=3 --gst-debug=typefind:5"

#
## test seek in wav file
wav_file=$toppkgdir/tests/testdata/wav/dtb_full.wav
./seektest $wav_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result
