#!/bin/sh

toppkgdir=${srcdir:-.}/..

#--gst-debug=neonhttpsrc:5
#--gst-debug-no-color
#gst_params="--gst-debug-level=3 --gst-debug=typefind:5"

# test mp3 tempo and pitch changes
mp3_file=$toppkgdir/tests/testdata/mp3/dtb_full.mp3
./tempopitchtest $mp3_file $gst_params

# test ogg tempo and pitch changes
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./tempopitchtest $ogg_file $gst_params

# test wav tempo and pitch changes
wav_file=$toppkgdir/tests/testdata/wav/dtb_full.wav
./tempopitchtest $wav_file $gst_params
