#!/bin/sh

toppkgdir=${srcdir:-.}/..

# test seek in mp3 file
mp3_file=$toppkgdir/tests/testdata/mp3/dtb_full.mp3
./seektest $mp3_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result
