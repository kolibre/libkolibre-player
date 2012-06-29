#!/bin/sh

toppkgdir=${srcdir:-.}/..

# test mp3 tempo and pitch changes
mp3_file=$toppkgdir/tests/testdata/mp3/dtb_full.mp3
./tempopitchtest $mp3_file $gst_params

result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result
