module.exports = function(RED) {

    var enocean = require("./lib/enocean-parser");
    var ESP3Parser = enocean.ESP3Parser;
    var ERP2Parser = enocean.ERP2Parser;
    var EEPParser = enocean.EEPParser;
    var PacketParser = enocean.PacketParser;

    var ESP3InNode = (function() {
        function input(node, msg) {
            var payload = msg.payload;
            if (!payload) {
                node.warn("Deny input, because msg has none payload");
                return;
            } else if (Buffer.isBuffer(payload)) {
                node.warn("Deny input, because payload is not Buffer");
                return;
            }

            try {
                var esp3 = node.parser.parse(payload);
                if (!esp3) {
                    return;
                }

                var packet = PacketParser.parse(esp3);

                var erp2 = ERP2Parser.parse(packet);

                var profile = node.eeps[erp2.originatorID];
                if (!profile) {
                    node.warn("Deny input, because ID:" + erp2.originatorID + " is Unknown");
                    return;
                }

                var parser = EEPParser.get(profile);
                if (!parser) {
                    node.warn("Deny input, because EEP:" + profile + " is Unsupported");
                    return;
                }

                var eep = parser(erp2);

                msg = {
                    id: erp2.originatorID,
                    eep: profile,
                    payload: eep,
                };

                node.send(msg);
            } catch (e) {
                node.warn("Deny input, because packet parse failed(" + e + ")");
            }
        }

        function ESP3InNode(n) {
            RED.nodes.createNode(this, n);
            this.eeps = n.eeps || {};

            this.parser = new ESP3Parser();

            var node = this;
            this.on("input", function(msg) {
                input(node, msg);
            });
        }

        return ESP3InNode;
    }());
    RED.nodes.registerType("esp3 in", ESP3InNode);
};
