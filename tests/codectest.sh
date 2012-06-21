#!/bin/sh

toppkgdir=${srcdir:-.}/..

#--gst-debug=neonhttpsrc:5
#--gst-debug-no-color
#gst_params="--gst-debug-level=3 --gst-debug=typefind:5"

# test mp3 codec
mp3_file=$toppkgdir/tests/testdata/mp3/dtb_full.mp3
./codectest $mp3_file $gst_params

# test ogg codec
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./codectest $ogg_file $gst_params

# test wav codec
wav_file=$toppkgdir/tests/testdata/wav/dtb_full.wav
./codectest $wav_file $gst_params
