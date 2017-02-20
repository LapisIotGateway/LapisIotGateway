var connections = {}

module.exports = function(RED) {

    var exec = require('child_process').exec;
    var Promise = require("es6-promise").Promise;
    var rm92a = require("./rm92a");

    function error(message) {
        RED.log.error("RM92A Connection: " + message);
    }

    function open(port, serialConfig, rm92aConfig, callback) {
        Promise.resolve()
            .then(function() {
                return new Promise(function(resolve, reject) {
                    var reset = rm92aConfig.reset;
                    if (reset) {
                        exec(reset, function(err) {
                            !err ? resolve() : reject(err);
                        });
                    } else {
                        resolve();
                    }
                });
            })
            .then(function() {
                return new Promise(function(resolve, reject) {
                    port.open(serialConfig, function(err) {
                        !err ? resolve() : reject(err);
                    });
                });
            })
            .then(function() {
                var options = {
                    mode: rm92aConfig.mode,
                    ch: rm92aConfig.ch,
                    panid: rm92aConfig.panid,
                    src: rm92aConfig.src,
                    dst: rm92aConfig.dst,
                    parent: 1,
                    routing: 2,
                    power: rm92aConfig.pw,
                    bandwidth: rm92aConfig.bw,
                    factor: rm92aConfig.sf,
                    bitrate: rm92aConfig.bitrate,
                    ack: rm92aConfig.ack,
                    timeout: rm92aConfig.timeout,
                    retry: rm92aConfig.retry,
                    rtc: rm92aConfig.rtc,
                };
                port.config(options);
            })
            .then(callback)
            .catch(callback);
    }

    function setup(port, serialConfig) {
        port.on("error", function(err) {
            error("serialport(" + (serialConfig.device) + ") error: " + (err.toString()));
        });

        port.on("close", function() {
            if (!port._closing) {
                error("serialport(" + (serialConfig.device) + ") closed unexpectedly");
            }
        });
    }

    function config(serialConfig) {
        return {
            baudrate: 115200,
            databits: 8,
            parity: "none",
            stopbits: 1,
        };
    }

    return {
        get: function get(serialConfig, rm92aConfig) {
            var id = serialConfig.serialport;
            var params = config(serialConfig);
            if (!connections[id]) {
                connections[id] = (function() {
                    var conn = new rm92a(id, serialConfig);
                    setup(conn);
                    conn._closing = false;
                    open(conn, params, rm92aConfig, function(err) {
                        if (!err) {
                            conn.emit("ready");
                        } else {
                            error(("serialport(" + id + ") open error: " + (err.toString())));
                        }
                    });
                    return conn;
                }());
            }
            return connections[id];
        },

        close: function close(serialConfig, done) {
            var id = serialConfig.serialport;
            if (connections[id]) {
                connections[id]._closing = true;
                connections[id].close(done);
                delete connections[id];
            } else {
                done()
            }
        }
    };
};
