# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

import machine
spi = machine.SPI(machine.SPI.SPI1, mosi=10, miso=9, sck=17)
cs_n = machine.Pin(15, mode=machine.Pin.OUT)

addr=0x82003000

def inttobytearray(addr):
    buf=bytearray(5)
    buf[0]=1    # Read from the Wishbone bus
    for i in range(4):  # 32 bits
        shift = (8*(3-i))   # how many bits to shift (position of byte), should be 24, 16, 8, 0
        buf[i+1] = (addr & (0xff << shift )) >> shift
    return(buf)

def read_ID():
    fpga_id = ""
    buf_r=bytearray(5)
    for i in range(255):
        addr_b=inttobytearray(addr+i*4)
        cs_n.value(0)
        spi.write(addr_b)
        spi.readinto(buf_r)
        cs_n.value(1)
        c=chr(buf_r[4])
        fpga_id += c
        if c == "\0":
            break
    print(fpga_id)
    
    
    

