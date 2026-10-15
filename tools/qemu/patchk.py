import struct,sys
d=bytearray(open('kernel8.img','rb').read())
def patch(addr,old,new):
    o=addr-0x80000
    assert struct.unpack_from('<I',d,o)[0]==old,(hex(addr),hex(struct.unpack_from('<I',d,o)[0]))
    struct.pack_into('<I',d,o,new)
# SPI0: TXD/RXD/DONE always set (QEMU has no SPI0 model)
patch(0x150348,0xb9400024,0x52a000e4)
patch(0x15037c,0xb9400024,0x52a000e4)
patch(0x150398,0xb9400022,0x52a000e2)
# I2C1 write: DONE set, no ERR/CLKT (QEMU has no BSC model)
patch(0x14faac,0xb9400064,0x52800044)
patch(0x14faec,0xb9400062,0x52800002)
# I2C1 read: DONE set, RXD never set (empty reads), no errors
patch(0x14f8d8,0xb9400042,0x52800042)
patch(0x14f8fc,0xb94000a5,0x52800005)
patch(0x14f91c,0xb9400042,0x52800042)
patch(0x14f94c,0xb9400042,0x52800442)
patch(0x14f958,0xb9400040,0x52800000)
open('kernel8.spipatch.img','wb').write(d)
