var connections = {};

module.exports = function(RED) {

    var Promise = require("es6-promise").Promise;
    var slr429 = require("./slr429");

    function error(message) {
        RED.log.error(("SLR429 Connection: " + message));
    }

    function open(port, serialConfig, slr429Config, callback) {
        Promise.resolve()
        .then(setting(port, "open", serialConfig))
        .then(setting(port, "mo", slr429Config.mode))
        .then(setting(port, "ch", slr429Config.ch))
        .then(setting(port, "gi", slr429Config.gid))
        .then(setting(port, "di", slr429Config.did))
        .then(setting(port, "ei", slr429Config.eid))
        .then(setting(port, "sf", slr429Config.chip))
        .then(function() {
            setup(port, serialConfig);
            callback(null);
        })
        .catch(callback);
    }

    function reset(port, serialConfig, callback) {
        Promise.resolve()
        .then(function() {
            port._closing = true;
        })
        .then(function() {
            return new Promise(function(resolve) {
                port.close(resolve);
            });
        })
        .then(setting(port, "open", serialConfig))
        .then(function() {
            port._closing = false;
            callback(null);
        })
        .catch(callback);
    }

    function setup(port, serialConfig) {
        port.on("error", function(err) {
            error(("serialport(" + (serialConfig.device) + ") error: " + (err.toString())));
        });

        port.on("close", function() {
            if (!port._closing) {
                error(("serialport(" + (serialConfig.device) + ") closed unexpectedly"));
            }
        });
    }

    function setting(port, command, value) {
        return function() {
            return new Promise(function(resolve, reject) {
                port[command](value, function(err) {
                    if (!err) {
                        resolve();
                    } else {
                        reject(err);
                    }
                });
            });
        };
    }

    function config(serialConfig) {
        return {
            baudrate: 19200,
            databits: 8,
            parity: "none",
            stopbits: 1,
        };
    }

    return {
        get: function get(serialConfig, slr429Config) {
            var id = serialConfig.serialport;
            var params = config(serialConfig);
            if (!connections[id]) {
                connections[id] = (function() {
                    var conn = new slr429(id);
                    conn._closing = false;
                    open(conn, params, slr429Config, function(err) {
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
                connections[id].close(function(err) {
                    if (err) { error(("serialport(" + id + ") close error: " + (err.toString()))); }
                    done();
                });
                delete connections[id];
            } else {
                done();
            }
        },

        reconnect: function reconnect(serialConfig) {
            var id = serialConfig.serialport;
            if (connections[id]) {
                var conn = connections[id];
                var params = config(serialConfig);
                reset(conn, params, function(err) {
                    if (err) { error(("serialport(" + id + ") reset error: " + (err.toString()))); }
                });
            }
        }
    };
};
