#!/bin/sh

toppkgdir=${srcdir:-.}/..

## test seek in ogg file
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./seektest $ogg_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi


exit $result
