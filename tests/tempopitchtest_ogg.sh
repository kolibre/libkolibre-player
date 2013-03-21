#!/bin/sh

toppkgdir=${srcdir:-.}/..

# test ogg tempo and pitch changes
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
## NOTE:
# This test fails due to segmentation fault. However, running
# it through gdb works. For wav and mp3 it also works fine.
#./tempopitchtest $ogg_file $gst_params
#gdb --args ./.libs/tempopitchtest $ogg_file $gst_params
result=$?

if [ $result -ne 0 ]
then
    echo TEST FAILED! Returned $result
fi

exit $result

