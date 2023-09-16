var usb = require('usb');

var VID = 0xDEAD
var PID = 0xBEEF
var led_state = 0;

// Find our device
var dev = usb.findByIds(VID, PID);

// Open it
dev.open();

// Create a function that sends a control transfer to the device with the given value
function sendControlTransfer(val) {
    dev.controlTransfer(0x40, 0x01, val, 0x0000, Buffer.alloc(0), (err, data) => {
        if(err) {
            console.log(err);
        }
    });
}

// Create an interval that will update the LED state via a control transfer every second
setInterval(() => {
    led_state = led_state ? 0 : 1;
    sendControlTransfer(led_state);
}, 1000);