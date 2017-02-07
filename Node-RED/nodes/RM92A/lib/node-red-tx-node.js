module.exports = function(RED) {

    var rm92aPool = require("./node-red-pool")(RED);

    function setup(node, serialConfig, rm92aConfig) {
        node.port = rm92aPool.get(serialConfig, rm92aConfig);

        node.on("input", function(msg) {
            if (!msg.hasOwnProperty("payload")) {
                return;
            }

            var payload = msg.payload;
            var dst = msg.dst || node.dst;
            node.port.write(dst, payload);
        });

        node.port.on("ready", function() {
            node.status({ fill: "green", shape: "dot", text: "node-red:common.status.connected" });
        });

        node.port.on("error", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("close", function() {
            node.status({ fill: "red", shape: "ring", text: "node-red:common.status.not-connected" });
        });

        node.port.on("carrier", function() {
            node.error("carrier sense error");
        });

        return true;
    }

    function teardown(node, serialConfig, done) {
        rm92aPool.close(serialConfig, done);
    }

    function TxNode(n) {
        var node = this;

        RED.nodes.createNode(node, n);

        node.rm92a = n.rm92a;
        node.rm92aConfig = RED.nodes.getNode(node.rm92a);

        node.serial = n.serial;
        node.serialConfig = RED.nodes.getNode(node.serial);

        node.dst = parseInt(n.dst) || 0x00FF;

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

    return TxNode;
};
