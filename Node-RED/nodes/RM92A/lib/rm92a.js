var util = require("util");
var EventEmitter = require("events").EventEmitter;
var serialport = require("serialport");
var fsm = require("./rm92a-fsm");

var MAX_BYTE = 100;

function RM92A(dev) {
    EventEmitter.call(this);
    this._dev = dev;
    this._fsm = fsm.create();
    this._writeQueue = [];
}
util.inherits(RM92A, EventEmitter);

RM92A.prototype.open = function(options, callback) {
    if (this.isOpen()) {
        setImmediate(function() {
            callback(new Error("Already opend"));
        });
        return;
    }

    var self = this;

    self._port = new serialport.SerialPort(self._dev, {
        baudrate: options.baudrate,
        databits: options.databits,
        parity: options.parity,
        stopbits: options.stopbits,
        parser: serialport.parsers.readline("\r\n"),
    }, false);

    self._port.open(function(err) {
        if (!err) {
            fsm.event(self._fsm, "open", self);
            callback(null);
        } else {
            callback(err);
        }
    });
    self._port.on("close", function() {
        self._port = null;
        fsm.event(self._fsm, "close", self);
        self.emit("close");
    });
    self._port.on("error", function(err) {
        self._port = null;
        fsm.event(self._fsm, "close", self);
        self.emit("error", err);
    });
};

RM92A.prototype.config = function(options) {
    if (!this.isOpen()) {
        return;
    }

    this._options = options;
    fsm.event(this._fsm, "setting", this);
};

RM92A.prototype.close = function(callback) {
    if (!this.isOpen()) {
        setImmediate(function() {
            callback(new Error("Already closed"));
        });
        return;
    }

    this._port.close(callback);
    this._writeQueue = [];
    this._port = null;
};

RM92A.prototype.write = function(addr, data) {
    if (!this.isOpen()) {
        return;
    }

    var buffer = new Buffer(data);
    var len = buffer.length;
    var offset = 0;
    do {
        var wlen = Math.min(MAX_BYTE, len - offset);
        this._writeQueue.push({ dst: addr, buffer: buffer.slice(offset, offset + wlen) });
        offset += MAX_BYTE;
    } while (offset < len);
    fsm.event(this._fsm, "write", this);
};

RM92A.prototype.isOpen = function() {
    return this._port ? true : false;
};

RM92A.Mode = {
    LoRa: 1,
    FSK: 2,
};

RM92A.Parent = 0;
RM92A.Child = 1;

RM92A.Routing = {
    Fixaction: 0,
    AutoRouting: 1,
    NonRouting: 2,
};

RM92A.TXPower = {
    P20mW: 0,
    P4mW: 1,
    P1mW: 2,
};

RM92A.BandWidth = {
    BW125: 0,
    BW250: 1,
    BW500: 2,
};

RM92A.Factor = {
    SF6: 0,
    SF7: 1,
    SF8: 2,
    SF9: 3,
    SF10: 4,
    SF11: 5,
    SF12: 6,
};

module.exports = RM92A;
