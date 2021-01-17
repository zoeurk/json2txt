#!/bin/sh
case $1
in
start)
	X=0
	Y=0
	i3-msg -t get_tree | ~/Projets/json2txt/json2txt -J | sed -nf .config/i3/script/start.sed > /tmp/move.src
	while read LINE; do
		X=$(($X+1))
		Y=$(($Y+3))
		sed -i /tmp/move.src -e "$X {s/X ppt Y ppt/$Y ppt $Y ppt/}"
	done < /tmp/move.src
	. /tmp/move.src >> /tmp/errors.txt
	rm /tmp/move.src
;;
restart)
	i3-msg -t get_tree | ~/Projets/json2txt/json2txt -J | sed -nf .config/i3/script/restart.sed > /tmp/move.src
	. /tmp/move.src >> /tmp/errors.txt
;;
esac
