#!/bin/bash
# mksd.sh <img> <ini> : MBR + FAT32 card with ini, bank 000001 and a performance folder
set -e
img=$1; ini=$2
rm -f $img; truncate -s 256M $img
python3 - $img <<'P'
import struct,sys
f=open(sys.argv[1],'r+b'); n=256*1024*1024//512-2048
f.seek(446); f.write(struct.pack('<B3sB3sII',0x80,b'\x00\x21\x00',0x0c,b'\xfe\xff\xff',2048,n)); f.seek(510); f.write(b'\x55\xaa')
P
mkfs.vfat -F 32 --offset 2048 $img >/dev/null
export MTOOLS_SKIP_CHECK=1
M="-i $img@@1048576"
mmd $M ::sysex ::sysex/voice ::performance
mcopy $M $ini ::minidexed.ini
mcopy $M /mnt/project-files/minidexed/zicdexed-build3/sysex/voice/000001_nasty.syx ::sysex/voice/
