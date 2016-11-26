#!/bin/bash

WEATHER=`cat /tmp/weather`
DATE=`date "+%A %Y-%m-%d %l:%M%p"`

echo $WEATHER " - " $DATE > /root/marquee.txt
