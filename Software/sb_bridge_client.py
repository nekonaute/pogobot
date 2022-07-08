#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License.
# Please refer to file LICENCE for details.

from litex import RemoteClient # Code is in litex/litex/tools/litex_client.py
from litescope import LiteScopeAnalyzerDriver

import time
import sys
import argparse
from os import path

#args.port=1234
#args.csr_csv="csr.csv"
parser = argparse.ArgumentParser()
parser.add_argument("--port", help="Port to connect to the server", type=int)
parser.add_argument("--regs", help="List the available registers", action="store_true")
parser.add_argument("--readIR", help="Read the IR uart registers",action="store_true")
parser.add_argument("--writeIR", help="write the IR uart registers", type=str)
parser.add_argument("--read", help="Read memory at address", type=lambda x: int(x,0))
parser.add_argument("--length", help="How many bytes to read", type=int)
parser.add_argument("--write", help="write one byte to the address", type=lambda x: int(x,0))
parser.add_argument("--data", help="data to write", type=lambda x: int(x,0))
parser.add_argument("--power", help="IR Power on all outputs", type=lambda x: int(x,0))
parser.add_argument("--loop", help="write the registers in loop", type=str)
parser.add_argument("--tap", help="record the signals",action="store_true")
parser.add_argument("--chan", help="IR channel to write to", type=int)
parser.add_argument("--csr_csv", help="Specify the csr.csv file, default to ./csr.csv", type=str)

args = parser.parse_args()
# debug :print(args.csr_csv,args.port)
if args.port and args.csr_csv:
    wb = RemoteClient( port=args.port, csr_csv=args.csr_csv)
elif args.port:
    wb = RemoteClient( port=args.port)
elif args.csr_csv:
    wb = RemoteClient( csr_csv=args.csr_csv )
else:
    if path.exists("csr.csv"):
        wb = RemoteClient()
    else:
        print("Error: file csr.csv does not exist in current path")
        print("  Specify it's path with option \"--csr_csv path/to/csr.csv\"")
        exit(-1)

wb.open()

# # #

def print_regs():
    # Read and print all the registers available
    print("Registers found : ")
    for i in wb.regs.d.items():
        print(i[0], ":", i[1].read() )
    print("Memory regions found : ")
    print(wb.mems) #for i in wb.mems:
    #    print(i[0], ":", i[1].read() )

def msg_loop(message=args.loop, chan=None):
    # Loop write a message to the uart        
    while(True):
        msg_writeIR(message, chan)

        
def msg_writeIR(message=42, chan=None):
    # Check power before sending nothing...
    for i in IR:
        if wb.regs.d["IR"+str(i)+"_power"].read() == 0:
            print("Error: power is at 0 !")
            exit(1)

    # Write a message to the uart
    if chan is not None:
        c = [chan]
    else:
        c = range(4)
        
    for i in c:
        if wb.regs.d["IR"+str(i)+"_uart_txfull"] != 1 :
            for j in message:
                print("Sending on IR[",i,"] : ", j)
                wb.regs.d["IR"+str(i)+"_uart_rxtx"].write(ord(j))
        
    time.sleep(0.5)
        

def tap():
    analyzer = LiteScopeAnalyzerDriver(wb.regs, "analyzer")
    analyzer.configure_subsampler(1)   
    analyzer.configure_group(0)
    #analyzer.configure_trigger(cond={"ts_n_phy.rx.rx_data_clock" : 1})  # only include this if you want a trigger condition
    # analyzer.configure_trigger(cond={"foo": 0xa5, "bar":0x5a}) # you can add my conditions by building a "dictionary"
    analyzer.add_rising_edge_trigger("tx")
    analyzer.add_trigger(cond={"tx" : 1})

    analyzer.run(offset=32, length=128)  # controls the "pre-trigger" offset plus length to download for this run, up to the length of the total analyzer config in hardware
    analyzer.wait_done()
    analyzer.upload()
    analyzer.save("dump.vcd")

def msg_read():
    # Read the message available on the uart
    msg=[]
    for i in range (4):
        msg.append("")
    while True:    
        for i in range(4):
            while wb.regs.d["IR"+str(i)+"_uart_rxempty"].read() != 1:
                if wb.regs.d["IR"+str(i)+"_configured"].read() == 1:
                    val=wb.regs.d["IR"+str(i)+"_uart_rxtx"].read()
                    if val < 127:   # printable character
                        msg[i]+=chr(val)
                    wb.regs.d["IR"+str(i)+"_uart_ev_pending"].write(2)
                else:
                    print("IR"+i+" not configured")
                
            if(len(msg[i]) != 0):
                print("Received on IR",i,":",msg[i])
                """for z in msg[i]:
                    print("Char ",i, "0x"+format(ord(z), '02x')) 
                """        
                print("End")
                msg[i]=""
    time.sleep(0.01)

def mem_read(address, length):
    for i in range(length):
        data = wb.read(address+i*4)
        print(format(address+i*4, '02x'),":", "0x"+format(data, '02x'), "ASCII:", data )
        
def set_power(i, power):
    wb.regs.d["IR"+str(i)+"_power"].write(power)

# Get FPGA ID if it exists and print it
fpga_id = ""
try:
    wb.bases.identifier_mem
except:
    print("No identifier found")
else:
    for i in range(256):
        c = chr(wb.read(wb.bases.identifier_mem + 4*i) & 0xff)
        fpga_id += c
        if c == "\0":
            break
    print("FPGA ID : ",fpga_id)
    
# Init InfraRed instances 
IR=[]
for i in range(4):
    try:
        wb.regs.d["IR"+str(i)+"_uart_rxtx"]
    except:
        print("Error : IR UART "+str(i)+" not found")
    else:
        IR.append(i)



if args.tap:
    tap()
    
if args.regs:
    print_regs()

if args.read:
    if args.length:
        mem_read(args.read, args.length)
    else:
        mem_read(args.read, 1)
        
if args.readIR:
    msg_read()

if args.chan is not None:
    chan=args.chan
else:
    chan=None

if args.writeIR:
    msg_writeIR(args.writeIR, chan)

if args.write:
    address = ''.join('0x{:02x}'.format(args.write)) # Convert int to string hex
    data = ''.join('0x{:02x}'.format(args.data)) # Convert int to string hex
    print("Writing to address : ", address, " value : ", data)
    #wb.regs.gpio_out.write( args.data)
    wb.write(args.write, args.data)
    
if args.loop:
    msg_loop(args.loop, chan)

if args.power is not None:
    for i in range(4):
        set_power(i, args.power)

wb.close()

