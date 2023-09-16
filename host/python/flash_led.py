import usb.core
import sys
import time

dev = usb.core.find(idVendor=0xdead, idProduct=0xbeef)
if(dev == None):
    print("Could not find device")
    sys.exit(255)

while(1):
    # Send a control transfer write to the device to turn on the LED
    dev.ctrl_transfer(  0x40,   # bmRequestType
                        0x01,   # bRequest
                        0x0000, # wValue
                        0x0000, # wIndex
                        0x0000) # Length of data or data

    # Wait for 1 second
    time.sleep(1)

    # Send a control transfer write to the device to turn off the LED
    dev.ctrl_transfer(  0x40,   # bmRequestType
                        0x01,   # bRequest
                        0x0001, # wValue
                        0x0000, # wIndex
                        0x0000) # Length of data or data

    # Wait for 1 second
    time.sleep(1)