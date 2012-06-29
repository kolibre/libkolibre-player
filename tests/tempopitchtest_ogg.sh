#!/bin/sh

toppkgdir=${srcdir:-.}/..

# test ogg tempo and pitch changes
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./tempopitchtest $ogg_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result

