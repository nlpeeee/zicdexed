#!/bin/bash
# run.sh <sd.img> <seconds> <outprefix> [kernel]: boot, then dump registers and the framebuffer
img=$1; secs=$2; out=$3; k=${4:-kernel8.img}
rm -f mon.sock $out.log
qemu-system-aarch64 -M raspi3b -accel tcg,thread=single -icount shift=2 -kernel $k -drive file=$img,if=sd,format=raw -serial null -serial null -display none -monitor unix:mon.sock,server,nowait >/dev/null 2>&1 &
QP=$!
sleep $secs
python3 - $out <<'P'
import socket,time,sys
s=socket.socket(socket.AF_UNIX); s.connect("mon.sock"); time.sleep(.5); s.recv(65536)
s.send(b"info registers\n"); time.sleep(1); r=s.recv(65536).decode(errors='replace')
print([l for l in r.split('\n') if 'PC=' in l][:1])
s.send(("screendump %s.ppm\n"%sys.argv[1]).encode()); time.sleep(3)
P
kill $QP
python3 -c "
from PIL import Image; Image.open('$out.ppm').save('$out.png')"
true
