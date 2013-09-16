#!/bin/sh

toppkgdir=${srcdir:-.}/..

# test ogg codec
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./codectest $ogg_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result
