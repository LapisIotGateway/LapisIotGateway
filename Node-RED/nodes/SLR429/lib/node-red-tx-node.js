module.exports = function(RED) {

    var Promise = require("es6-promise").Promise;
    var slr429Pool = require("./node-red-pool")(RED);
    var CarrierSenseError = require("./slr429").CarrierSenseError;

    var DT_RETRY_INTERVAL = 1000;
    var DT_RETRY_MAX = 5;

    function errorHandle(resolve, reject) {
        return function(err) {
            if (!err) {
                resolve();
            } else {
                reject(err);
            }
        };
    }

    function dt(port, retry, max, payload) {
        return Promise.resolve()
        .then(function() {
            return new Promise(function(resolve, reject) {
                port.dt(payload, errorHandle(resolve, reject));
            });
        })
        .catch(function(err) {
            if (err instanceof CarrierSenseError) {
                if (retry !== max) {
                    return new Promise(function(resolve) {
                        setTimeout(function() {
                            resolve();
                        }, DT_RETRY_INTERVAL);
                    })
                    .then(function() {
                        return dt(port, retry+1, max, payload);
                    })
                }
            }
            throw err;
        });
    }

    function write(node, gid, bgid, did, payload) {
        var port = node.port;
        Promise.resolve()
        .then(function() {
            return new Promise(function(resolve, reject) {
                port.di(did, errorHandle(resolve, reject));
            });
        })
        .then(function() {
            return new Promise(function(resolve, reject) {
                if (gid != bgid) {
                    port.gi(gid, errorHandle(resolve, reject));
                } else {
                    resolve();
                }
            });
        })
        .then(function() {
            return dt(port, 0, DT_RETRY_MAX, payload);
        })
        .then(function() {
            return new Promise(function(resolve, reject) {
                if (gid != bgid) {
                    port.gi(bgid, errorHandle(resolve, reject));
				} else {
					resolve();
				}
            });
        })
        .catch(function(err) {
            node.error(err.toString());
            if (err instanceof CarrierSenseError) {
                // DO NOTHING
            } else {
                slr429Pool.reconnect(node.serialConfig);
            }
        });
    }

    function setup(node, serialConfig, slr429Config) {
        node.port = slr429Pool.get(serialConfig, slr429Config);

        var bgid = slr429Config.gid;
        node.on("input", function(msg) {
            if (!msg.hasOwnProperty("payload")) {
                return;
            }

            var payload = msg.payload;
            if (node.port.binMode) {
                node.port.write(payload, function(err) {
                    var msg = { result: !err, error: err };
                    node.send(msg);
                });
            } else {
                var gid = msg.gid || node.gid;
                var did = msg.did || node.did;
                write(node, gid, bgid, did, payload);
            }
        });

        node.port.on("ready", function() {
            node.status({ fill: "green", shape: "dot", text: "node-red:common.status.connected" });
        });

        node.port.on("error", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("closed", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });
        return true;
    }

    function teardown(node, serialConfig, done) {
        slr429Pool.close(serialConfig, done);
    }

    function TxNode(n) {
        var node = this;

        RED.nodes.createNode(this, n);

        this.slr429 = n.slr429;
        this.slr429Config = RED.nodes.getNode(this.slr429);

        this.serial = n.serial;
        this.serialConfig = RED.nodes.getNode(this.serial);

        this.gid = n.gid || 0x00;
        this.did = n.did || 0x01;

        this.status({ fill: "grey", shape: "dot", text: "node-red:common.status.not-connected" });

        if (this.slr429Config && this.serialConfig) {
            setup(this, this.serialConfig, this.slr429Config);

            this.on("close", function(done) {
                teardown(node, node.serialConfig, done);
            });
        } else {
            node.error("missing config");

            this.on("close", function(done) {
                done();
            });
        }
    }

    return TxNode;
};
