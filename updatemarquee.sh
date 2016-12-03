#!/bin/bash

IP="192.168.123.30"

scp marquee/src/marquee.c root@$IP:

ssh root@$IP "gcc marquee.c -lwiringPi -lpthread -o marquee " 

scp getweather.sh root@$IP:
scp setmarquee.sh root@$IP:

