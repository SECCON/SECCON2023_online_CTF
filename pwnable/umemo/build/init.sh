#!/bin/sh

service xinetd start && exec sh -c "while :; do rm -f /tmp/mp_*; sleep 15m; done"
