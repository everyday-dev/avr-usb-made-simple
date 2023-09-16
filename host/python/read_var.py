import usb.core
import sys
import time

dev = usb.core.find(idVendor=0xdead, idProduct=0xbeef)
if(dev == None):
    print("Could not find device")
    sys.exit(255)

while(1):
    # Send a control transfer read to the device to read the value of the variable
    value = dev.ctrl_transfer(  0xC0,   # bmRequestType
                                0x02,   # bRequest
                                0x0000, # wValue
                                0x0000, # wIndex
                                0x0001) # Length of data or data

    print("Value: " + str(value[0]))

    # Wait for 1 second
    time.sleep(1)