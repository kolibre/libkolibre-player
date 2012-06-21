#!/bin/sh

if [ $# -eq 0 ]; then
	echo "Finds the millisecond stuff for a smil/mp3 combo in a daisy book"
	echo "   Usage maketestdata.sh path-to-file"
	exit
fi

if [ ! -f $1.smil ]; then
	echo $1.smil does not exist
fi

if [ ! -f $1.mp3 ]; then
	echo $1.mp3 does not exist
fi



for a in `cat $1.smil |grep clip-begin | cut -c45-82 | sed -e 's/clip-end=\"npt=//g' | sed -e 's/[\.\"s]//g' | sed -e 's/id//g' | sed -e 's/[=i]//g' | sed -e 's/ /,/g' `; do 
	echo "urls.push_back(playitem(\"$1.mp3\",$a  ));" | sed -e 's/,  //g'
done
