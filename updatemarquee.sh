#!/bin/bash

scp marquee/src/marquee.c root@192.168.123.30:

ssh root@192.168.123.30 "gcc marquee.c -lwiringPi -lpthread -o marquee " 

