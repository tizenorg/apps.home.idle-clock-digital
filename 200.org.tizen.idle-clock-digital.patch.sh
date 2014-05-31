#!/bin/sh

if [ ! -e /opt/var/kdb/db/idle-clock/digital/showdate ]; then
/usr/bin/vconftool set -tf int db/idle-clock/digital/showdate 1 -u 5000 -s org.tizen.idle-clock-digital
fi

if [ ! -e /opt/var/kdb/db/idle-clock/digital/clock_font ]; then
/usr/bin/vconftool set -tf int db/idle-clock/digital/clock_font 1 -g 5000 -s org.tizen.idle-clock-digital
fi

if [ ! -e /opt/var/kdb/db/idle-clock/digital/clock_font_color ]; then
/usr/bin/vconftool set -tf int db/idle-clock/digital/clock_font_color 8 -g 5000 -s org.tizen.idle-clock-digital
fi
