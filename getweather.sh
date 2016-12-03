#!/bin/bash

URL="http://www.accuweather.com/en/us/chappaqua-ny/10514/weather-forecast/3909_pc"
wget -q -O- "$URL" > /tmp/w1
T=`grep "large-temp" /tmp/w1 | head -1  | awk -F ">" '{ print $2 }' | awk -F "&" '{ print $1}'`
COND=`grep "cond" /tmp/w1 | head -1 | awk -F ">" '{ print $2 }' | awk -F "<" '{ print $1}'`

echo -n " - Temp - " $T " " $COND > /tmp/weather


