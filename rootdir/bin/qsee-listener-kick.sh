i=0
while [ "$(getprop sys.listeners.registered)" != "true" ] && [ "$i" -lt 30 ]; do
    sleep 2
    i=$((i + 1))
    echo "QSEE-KICK: attempt $i - listeners not up yet, restarting qseecomd" > /dev/kmsg
    setprop ctl.restart vendor.qseecomd
done

if [ "$(getprop sys.listeners.registered)" = "true" ]; then
    echo "QSEE-KICK: listeners registered after $i restart(s)" > /dev/kmsg
else
    echo "QSEE-KICK: GAVE UP after $i restarts, listeners still down (NOT timing)" > /dev/kmsg
fi
