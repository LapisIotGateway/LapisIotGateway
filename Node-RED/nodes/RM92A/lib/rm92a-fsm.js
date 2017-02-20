var fsm = require("javascript-state-machine");
var RM92AUtil = require("./rm92a-util");
var rm92aSettings = require("./rm92a-settings");

var SEND_BUFFER_MIN = 6;

var REGEX_ACK_OK = /^\[OK\] ACK SeqNo\[(\d{1,3})\] DstID\[(0x[0-9ABCDEF]{4})\]$/;
var REGEX_ACK_NG = /^\[NG\] ACK SeqNo\[(\d{1,3})\] DstID\[(0x[0-9ABCDEF]{4})\]$/;
var REGEX_CARRIER_SENSE_ERROR = /^Carrier Sense Error\.$/;
var REGEX_DATA = /^(\d{1,5}),\1,(-?\d{1,3}),(.*$)/;

var Debug = RM92AUtil.Debug;

// Event
var EVENT = {
    initial: "s_none",
    events: [
        { name: "open", from: "s_none", to: "s_open" },
        { name: "setting", from: "s_open", to: "s_setting" },
        { name: "ready", from: ["s_setting", "s_write", "s_ok", "s_ng"], to: "s_idle" },
        { name: "write", from: "s_idle", to: "s_write" },
        { name: "wait", from: "s_write", to: "s_wait", },
        { name: "ok", from: "s_wait", to: "s_ok" },
        { name: "ng", from: "s_wait", to: "s_ng" },
        { name: "carrier", from: "s_wait", to: "s_carrier" },
        { name: "close", from: "*", to: "s_none" },
        { name: "err", from: "*", to: "s_error" }
    ],
    callbacks: {
        onleavestate: function(event, from, to) {
            return Debug("|%s| [%s] => [%s]", event, from, to); },
    },
};

// State open
EVENT.callbacks.ons_open = function(event, from, to, self) {
    self._port.on("data", function(data) { Debug("Received : %s", data); });
};

// State setting
EVENT.callbacks.ons_setting = function(event, from, to, self) {
    var options = self._options;
    rm92aSettings(self._port, options).then(function() {
        self._port.on("data", function(data) {
            if (REGEX_ACK_OK.test(data)) {
                // Recv ACK OK
                fsmEvent(self._fsm, "ok", self);
            } else if (REGEX_ACK_NG.test(data)) {
                // Recv ACK NG
                fsmEvent(self._fsm, "ng", self);
            } else if (REGEX_CARRIER_SENSE_ERROR.test(data)) {
                // Recv Carrier sense error
                fsmEvent(self._fsm, "carrier", self);
            } else {
                // Recv data
                recv(self, data);
            }
        });
    })
    .then(function() { fsmEvent(self._fsm, "ready", self); });
};

// State idle
EVENT.callbacks.ons_idle = function(event, from, to, self) {
    if (self._writeQueue.length > 0) {
        fsmEvent(self._fsm, "write", self);
    }
};

// state write
EVENT.callbacks.ons_write = function(event, from, to, self) {
    var data = self._writeQueue[0];

    if (!data) {
        var err = new Error("Write queue empty");
        fsmEvent(self._fsm, "err", self, err);
        return;
    }

    send(self._port, data.dst, data.buffer, function() {
        if (!self._options.ack || self._options.ack === 0) {
            fsmEvent(self._fsm, "ready", self);
        } else {
            fsmEvent(self._fsm, "wait", self);
        }
    });
};

// state ACK OK
EVENT.callbacks.ons_ok = function(event, from, to, self) {
    var data = self._writeQueue.shift();
    self.emit("ok", data.dst, data.buffer.toString());
    fsmEvent(self._fsm, "ready", self);
};

// state ACK NG
EVENT.callbacks.ons_ng = function(event, from, to, self) {
    var data = self._writeQueue.shift();
    self.emit("ng", data.dst, data.buffer.toString());
    fsmEvent(self._fsm, "ready", self);
};

// State Carrier sense error
EVENT.callbacks.ons_carrier = function(event, from, to, self) {
    var data = self._writeQueue.shift();
    self.emit("carrier", data.dst, data.buffer.toString());
    fsmEvent(self._fsm, "ready", self);
};

// state error
EVENT.callbacks.ons_error = function(event, from, to, self, err) {
    self.emit("error", err);
};

function send(port, dst, data, callback) {
    Debug("Send : %s(%d)", data.toString(), dst);
    // create buffer
    var buffer = new Buffer(SEND_BUFFER_MIN + data.length);
    // start code
    buffer.write("@@", 0, 2, "ascii");
    // data size
    buffer.writeUInt8(data.length, 2);
    // dst address
    buffer.writeUInt16BE(dst, 3);
    // data
    data.copy(buffer, 5);
    // end code
    buffer.writeUInt8(0xAA, 5 + data.length);
    // write serialport
    port.write(buffer, callback);
}

function recv(self, data) {
    var match = REGEX_DATA.exec(data);
    if (!match) {
        return; }

    var dst = Number(match[1]);
    var rssi = Number(match[2]);
    var str = match[3];
    self.emit("data", dst, rssi, str);
}

function fsmEvent(fsm, event, self) {
    if (fsm.can(event)) { fsm[event](self); }
}

module.exports.create = function() {
    return fsm.create(EVENT);
};

module.exports.event = fsmEvent;
