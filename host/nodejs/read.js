var usb = require('usb');

var VID = 0xDEAD
var PID = 0xBEEF
var led_state = 0;

// Find our device
var dev = usb.findByIds(VID, PID);

// Open it
dev.open();

// Create a function that sends a control transfer to the device with the given value
function sendControlTransfer() {
    dev.controlTransfer(0xC0, 0x02, 0x0000, 0x0000, 0x0001, (err, buff) => {
        if(err) {
            console.log(err);
        }

        if(buff) {
            data = Array.prototype.slice.call(buff, 0)
            console.log(`Value: ${data[0]}`);
        }
    });
}

// Create an interval that will read the variable from the device every second
setInterval(() => {
    sendControlTransfer();
}, 1000);