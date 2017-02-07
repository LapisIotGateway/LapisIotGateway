module.exports = function(RED) {
    var ToamiSdata = require("./lib/toami-sdata");

    var ToamiOutNode = (function(RED) {
        var State = {
            None: 0,
            Send: 1,
            Retry: 2,
        };

        function success(node) {
            node.queue.shift();
            node.counter = 0;
            if (node.queue.length > 0) {
                excute(node);
            } else {
                node.state = State.None;
            }
        }

        function fail(node) {
            var retry = node.retry;
            if (!Number.isFinite(retry) || Number.isNaN(retry)) {
                // retry infinity
            } else if (node.counter < node.retry) {
                node.counter++;
            } else {
                node.queue.shift();
                node.counter = 0;
            }

            if (node.queue.length > 0) {
                node.state = State.Retry;
                node.timer = setTimeout(function() {
                    excute(node);
                    node.timer = null;
                }, node.interval);
            } else {
                node.state = State.None;
            }
        }

        function excute(node) {
            node.state = State.Send;
            node.status({ fill: "blue", shape: "dot", text: "node-red:httpin.status.requesting" });

            var data = node.queue[0];
            var req = node.toami.request(data);
            req.on("ok", function() {
                node.status({ fill: "red", shape: "ring", text: "node-red:common.status.ok" });
                success(node);
            });
            req.on("ng", function(code, status) {
                node.error("error : " + code + " " + status);
                node.status({ fill: "red", shape: "ring", text: code });
                fail(node);
            });
            req.on("timeout", function() {
                node.error(RED._("node-red:common.notification.errors.no-response"));
                node.status({ fill: "red", shape: "ring", text: "node-red:common.notification.errors.no-response" });
                fail(node);
            });
            req.on("error", function(err) {
                node.error(err);
                node.status({ fill: "red", shape: "ring", text: "node-red:common.status.error" });
                fail(node);
            });
        }

        function input(node, msg) {
            if (!msg.payload) {
                return;
            }

            var sdata = msg.payload;
            node.queue.push(sdata);

            if (node.state === State.None) {
                excute(node);
            }
        }

        function setup(node, config) {
            var host = config.host;
            var gateway = config.gateway;
            var key = config.key;

            node.state = State.None;
            node.queue = [];
            node.toami = new ToamiSdata(key, host, gateway, node.timeout);
            node.counter = 0;
            node.timer = null;

            node.on("input", function(msg) {
                input(node, msg);
            });

            node.on("close", function() {
                if (node.timer) clearInterval(node.timer);
            });
        }

        return function(n) {
            RED.nodes.createNode(this, n);

            var node = this;

            node.config = RED.nodes.getNode(n.config);
            node.timeout = parseInt(n.timeout) || parseInt(RED.settings.httpRequestTimeout) || 120000;
            node.retry = parseInt(n.retry) || Number.POSITIVE_INFINITY;
            node.interval = parseInt(n.interval) || 0;

            if (node.config) {
                setup(node, node.config);
            } else {
                node.error("missing config");
            }
            node.on("close", function() {
                node.status({});
            });
        };
    }(RED));
    RED.nodes.registerType("toami out", ToamiOutNode);

    function ToamiConfigNode(n) {
        RED.nodes.createNode(this, n);

        this.host = n.host || "";
        this.gateway = n.gateway || "";
        this.key = n.key || "";
    }
    RED.nodes.registerType("toami-config", ToamiConfigNode);
};
