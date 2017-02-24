module.exports = function(RED) {
    RED.nodes.registerType("slr429 in", require("./lib/node-red-rx-node")(RED));
    RED.nodes.registerType("slr429 out", require("./lib/node-red-tx-node")(RED));

    function SLR429ConfigNode(n) {
        RED.nodes.createNode(this, n);
        this.mode = n.mode ? parseInt(n.mode) : 0;
        this.gid = n.gid ? parseInt(n.gid) : 0x00;
        this.eid = n.eid ? parseInt(n.eid) : 0x01;
        this.did = n.did ? parseInt(n.did) : 0x01;
        this.ch = n.ch ? parseInt(n.ch) : 27;
        this.chip = n.chip ? parseInt(n.chip) : 0;
    }
    RED.nodes.registerType("slr429-config", SLR429ConfigNode);

    function SerialNode(n) {
        RED.nodes.createNode(this, n);
        this.serialport = n.serialport;
    }
    RED.nodes.registerType("slr429-serial", SerialNode);
};
