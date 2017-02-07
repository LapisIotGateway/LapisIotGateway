module.exports = function(RED) {

    var slr429Pool = require("./node-red-pool")(RED);

    function setup(node, serialConfig, slr429Config) {
        node.port = slr429Pool.get(serialConfig, slr429Config);

        node.port.on("ready", function() {
            node.status({ fill: "green", shape: "dot", text: "node-red:common.status.connected" });
        });

        node.port.on("error", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("closed", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("data", function(data, rssi) {
            var obj = { payload: data, rssi: rssi };
            node.send(obj);
        });
        return true;
    }

    function teardown(node, serialConfig, done) {
        slr429Pool.close(serialConfig, done);
    }

    function RxNode(n) {
        var node = this;

        RED.nodes.createNode(this, n);

        this.slr429 = n.slr429;
        this.slr429Config = RED.nodes.getNode(this.slr429);

        this.serial = n.serial;
        this.serialConfig = RED.nodes.getNode(this.serial);

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

    return RxNode;
};
