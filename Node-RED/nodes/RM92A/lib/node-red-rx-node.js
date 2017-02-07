module.exports = function(RED) {

    var rm92aPool = require("./node-red-pool")(RED);

    function setup(node, serialConfig, rm92aConfig) {
        node.port = rm92aPool.get(serialConfig, rm92aConfig);

        node.port.on("ready", function() {
            node.status({ fill: "green", shape: "dot", text: "node-red:common.status.connected" });
        });

        node.port.on("error", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("closed", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });
        node.port.on("data", function(dst, rssi, data) {
            var obj = { dst: dst, rssi: rssi, payload: data };
            node.send(obj);
        });
        return true;
    }

    function teardown(node, serialConfig, done) {
        rm92aPool.close(serialConfig, done);
    }

    function RxNode(n) {
        var node = this;

        RED.nodes.createNode(node, n);

        node.rm92a = n.rm92a;
        node.rm92aConfig = RED.nodes.getNode(node.rm92a);

        node.serial = n.serial;
        node.serialConfig = RED.nodes.getNode(node.serial);

        node.status({ fill: "grey", shape: "dot", text: "node-red:common.status.not-connected" });

        if (node.rm92aConfig && node.serialConfig) {
            setup(node, node.serialConfig, node.rm92aConfig);

            node.on("close", function(done) {
                teardown(node, node.serialConfig, done);
            });
        } else {
            node.error("missing config");

            node.on("close", function(done) {
                done();
            });
        }
    }

    return RxNode;
};
