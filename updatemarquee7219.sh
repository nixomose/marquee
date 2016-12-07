#!/bin/bash

IP="192.168.123.220"

scp marquee7219/src/marquee7219.c pi@$IP:

ssh pi@$IP "gcc marquee7219.c -lwiringPi -lpthread -o marquee7219 " 

scp getweather.sh pi@$IP:
scp setmarquee.sh pi@$IP:

