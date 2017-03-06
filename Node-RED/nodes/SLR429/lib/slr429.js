var util = require("util");
var EventEmitter = require("events").EventEmitter;
var serialport = require("serialport");
var Promise = require("es6-promise").Promise;
var SLR429Util = require("./slr429-util");

var Debug = SLR429Util.Debug;
var PromiseWrapper = SLR429Util.PromiseWrapper;
var Readline = SLR429Util.Readline;

var WAIT_MO = [
    /^FSK BIN MODE$/,
    /^FSK CMD MODE$/,
    /^LORA BIN MODE$/,
    /^LORA CMD MODE$/,
    /^AIR MONITOR MODE$/,
];

var REGEX_ERROR = /^\*ER=([0-9A-F]{2})$/;
var REGEX_DATA = /^\*DR=([0-9A-F]{2})(.*)$/;

var MODE_TIMER = 5000;
var COMMAND_TIMER = 2000;
var WRITE_TIMER = 10000;
var COMMAND_RETRY_MAX = 3;

var MAX_BYTE = 0xFF;

function SLR429(dev) {
    var self = this;

    EventEmitter.call(this);
    this.binMode = false;

    this._dev = dev;
    this._promise = new PromiseWrapper();
    this._readline = new Readline("\r\n", "utf-8");
    this._readline.on("data", function(data) {
        return parsedLine(self, data);
    });
}
util.inherits(SLR429, EventEmitter);

SLR429.prototype.open = function(options, callback) {
    var self = this;

    Debug("Open : %s", JSON.stringify(options));

    this._promise
        .then(function() {
            if (self._serial) {
                throw new Error("Already opened"); }
        })
        .then(function() {
            var serial = create(self._dev, options);
            return new Promise(function(resolve, reject) {
                serial.open(function(err) {
                    err ? reject(err) : resolve(serial);
                });
            });
        })
        .then(function(serial) {
            self._serial = serial;
            setup(self, serial);
            callback(null);
        })
        .catch(callback);
};

SLR429.prototype.write = function(data, callback) {
    var self = this;

    var buffer = data instanceof Buffer ? data : new Buffer(data);
    Debug(("<= : bin %s", buffer.toString("hex")));
    this._promise
        .then(function() {
            return new Promise(function(resolve, reject) {
                self._serial.write(buffer, (function(err) {
                    err ? reject(err) : resolve();
                }));
            });
        })
        .then(callback, callback);
};

SLR429.prototype.close = function(callback) {
    var self = this;

    Debug("Close");

    this._promise
        .then(function() {
            if (!self._serial) {
                throw new Error("No Opened"); }
        })
        .then(function() {
            return new Promise(function(resolve, reject) {
                self._serial.close(function(err) {
                    err ? reject(err) : resolve();
                });
            });
        })
        .then(function() {
            self._serial = null;
            callback(null);
        })
        .catch(callback);
};

SLR429.prototype.mo = function(mode, callback) {
    var self = this;

     if (!SLR429Util.between(mode, 0, 4)) {
         setImmediate(function() {
             callback(new Error("Invalid @MO parameter : " + mode));
         });
     }

     function mo(self, mode, retry, max) {
         return Promise.resolve()
            .then(function() {
                var fix = ("00" + (mode.toString(16))).slice(-2);
                var command = "@MO" + fix;
                write(self, command);
            })
            .then(function() {
                return new Promise(function(resolve, reject) {
                    wait(self, "@MO", WAIT_MO[mode], MODE_TIMER, function(err) {
                        err ? reject(err) : resolve();
                    });
                });
            })
            .catch(function(err) {
                if (retry === max) {
                    throw err;
                } else {
                    Debug("Retry because " + err.toString());
                    mo(self, mode, retry+1, max);
                }
            });
     }

    this._promise
        .then(function() {
            return mo(self, mode, 0, COMMAND_RETRY_MAX);
        })
        .then(function() {
            // Binary mode check
            self.binMode = (mode === 0 || mode === 2);
            callback(null);
        })
        .catch(callback);
};

SLR429.prototype.ch = function(value, callback) {
    setting(this, "@CH", value, 0x07, 0x2E, /^\*CH=[0-9A-F]{2}$/, COMMAND_TIMER, callback);
};

SLR429.prototype.sf = function(value, callback) {
    setting(this, "@SF", value, 0, 5, /^\*SF=[0-9A-F]{2}$/, COMMAND_TIMER, callback);
};

SLR429.prototype.di = function(value, callback) {
    setting(this, "@DI", value, 0x00, 0xFF, /^\*DI=[0-9A-F]{2}$/, COMMAND_TIMER, callback);
};

SLR429.prototype.ei = function(value, callback) {
    setting(this, "@EI", value, 0x00, 0xFF, /^\*EI=[0-9A-F]{2}$/, COMMAND_TIMER, callback);
};

SLR429.prototype.gi = function(value, callback) {
    setting(this, "@GI", value, 0x00, 0xFF, /^\*GI=[0-9A-F]{2}$/, COMMAND_TIMER, callback);
};

SLR429.prototype.rs = function(callback) {
    var self = this;

    function rs(self, retry, max) {
        var com = "@RS";
        return Promise.resolve()
            .then(function() {
                return write(self, com);
            })
            .then(function() {
                return new Promise(function(resolve, reject) {
                    wait(self, com, /^RSSI=(-?\d{1,3})dBm$/, COMMAND_TIMER, function(err, m) {
                        err ? reject(err) : resolve(m);
                    });
                });
            })
            .catch(function(err) {
                if (retry === max) {
                    throw err;
                } else {
                    Debug("Retry because " + err.toString());
                    rs(self, retry+1, max);
                }
            });
    }

    this._promise
        .then(function() {
            return rs(self, 0, COMMAND_RETRY_MAX);
        })
        .then(function(m) { callback(null, Number(m[1])); })
        .catch(function(err) { callback(err, null); });
};

SLR429.prototype.dt = function(str, callback) {
    var self = this;

    var buffer = new Buffer(str);
    var len = buffer.length;
    var offset = 0;

    (function(promise) {
        function dt(self, retry, max, buffer) {
            return Promise.resolve()
                .then(function() {
                    send(self, target);
                })
                .then(function() {
                    return new Promise(function(resolve, reject) {
                        wait(self, "@DT", /^\*IR=([0-9A-F]{2})$/, WRITE_TIMER, function(err, m) {
                            err ? reject(err) : resolve(m);
                        });
                    });
                })
                .catch(function(err) {
                    if (retry === max) {
                        throw err;
                    } else {
                        Debug("Retry because " + err.toString());
                        dt(self, retry+1, max, buffer);
                    }
                });
        }

        do {
            var wlen = Math.min(MAX_BYTE, len - offset);
            var target = buffer.slice(offset, offset + wlen);
            promise
            .then(function() {
                return dt(self, 0, COMMAND_RETRY_MAX, target);
            })
            .then(function(m) {
                var code = parseInt(m[1], 16);
                if (code !== 3) {
                   throw new CarrierSenseError("Error : IR(" + code + ")");
                }
            });
            offset += MAX_BYTE;
        } while (offset < len);
        return promise;
    }(self._promise))
    .then(callback)
    .catch(callback);
};

SLR429.Mode = {
    FSKBinary: 0,
    FSKCommand: 1,
    LoRaBinary: 2,
    LoRaCommand: 3,
    AirMonitor: 4,
};

SLR429.Chip = {
    C128: 0,
    C256: 1,
    C512: 2,
    C1024: 3,
    C2048: 4,
    C4096: 5,
};

function create(dev, options) {
    options.parser = serialport.parsers.raw;
    return new serialport.SerialPort(dev, options, false);
}

function setup(self, serial) {
    serial.on("data", function(data) {
        return received(self, data);
    });
    serial.on("error", function(err) {
        self._serial = null;
        self.emit("error", err);
    });
    serial.on("close", function() {
        self._serial = null;
        self.emit("close");
    });
}

function setting(self, com, value, min, max, regex, timeout, callback) {
    if (!SLR429Util.between(value, min, max)) {
        setImmediate(function() {
            callback(new Error("Invalid " + com + " parameter : " + value));
        })
    }

    var fix = ("00" + (value.toString(16))).slice(-2);
    var command = "" + com + fix;
    function _setting(self, retry, max, com, command, regex, timeout) {
        return Promise.resolve()
            .then(function() {
                write(self, command);
            })
            .then(function() {
                return new Promise(function(resolve, reject) {
                    wait(self, com, regex, timeout, function(err) {
                        err ? reject(err) : resolve();
                    });
                });
            })
            .catch(function(err) {
                if (retry === max) {
                    throw err;
                } else {
                    Debug("Retry because " + err.toString());
                    _setting(self, retry+1, max, com, command, regex, timeout);
                }
            });
    }

    self._promise
        .then(function() {
            return _setting(self, 0, COMMAND_RETRY_MAX, com, command, regex, timeout);
        })
        .then(callback)
        .catch(callback);
}

function send(self, data) {
    // write size
    var wlen = data.length;
    // create buffer
    var buffer = new Buffer(5 + wlen);
    // start code
    buffer.write("@DT", 0, 3, "ascii");
    // data size
    buffer.write(("00" + (wlen.toString(16))).slice(-2), 3, 2, "ascii");
    // data
    data.copy(buffer, 5, 0, wlen);
    // write serialport
    write(self, buffer.toString());
}

function write(self, data, callback) {
    Debug("<= : %s", data);

    self._serial.write((data + "\r\n"), callback);
}

function wait(self, com, regex, timeout, callback) {
    var timer = setTimeout(function() {
		self._wait = null;
        callback(new TimeoutError((com + " timeout")), null);
    }, timeout);

    self._wait = regex;
    self._callback = function(err, m) {
        clearTimeout(timer);
		setTimeout(function() {
			callback(err, m);
		}, 100);
    };
}

function received(self, buffer) {
    if (self.binMode) {
        Debug("=> : bin %s", buffer.toString("hex"));
        self.emit("bin", buffer);
    } else {
        self._readline.append(buffer);
    }
}

function parsedLine(self, data) {
    Debug("=> : %s", data);
    if (self._wait && self._wait.test(data)) {
        var m = self._wait.exec(data);
        self._callback(null, m);
    } else if (self._wait && REGEX_ERROR.test(data)) {
        var m$1 = REGEX_ERROR.exec(data);
        var eno = m$1[1];
        self._callback(new CommandError("Error : " + eno));
    } else if (REGEX_DATA.test(data)) {
        var m$2 = REGEX_DATA.exec(data);
        var str = m$2[2];
        self.rs(function(err, rssi) {
            self.emit("data", str, rssi);
        });
    }
}

module.exports = SLR429;

function TimeoutError(message) {
    Error.call(this, message)
}
util.inherits(TimeoutError, Error);

function CommandError(message) {
    Error.call(this, message)
}
util.inherits(CommandError, Error);

function CarrierSenseError(message) {
    Error.call(this, message)
}
util.inherits(CarrierSenseError, Error);

SLR429.TimeoutError = TimeoutError;
SLR429.CommandError = CommandError;
SLR429.CarrierSenseError = CarrierSenseError;