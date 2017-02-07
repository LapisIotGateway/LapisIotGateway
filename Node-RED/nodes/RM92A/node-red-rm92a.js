module.exports = function(RED) {
    RED.nodes.registerType("rm92a in", require("./lib/node-red-rx-node")(RED));
    RED.nodes.registerType("rm92a out", require("./lib/node-red-tx-node")(RED));

    function RM92AConfigNode(n) {
        RED.nodes.createNode(this, n);
        this.reset = n.reset;
        this.mode = parseInt(n.mode) || 1;
        this.ch = parseInt(n.ch) || 24;
        this.panid = parseInt(n.panid) || 0x1234;
        this.src = parseInt(n.src) || 0x0001;
        this.dst = parseInt(n.dst) || 0x00FF;
        this.pw = parseInt(n.pw) || 2;
        this.bw = parseInt(n.bw) || 0;
        this.sf = parseInt(n.sf) || 0;
        this.bitrate = parseInt(n.bitrate) || 50000;
        this.timeout = parseInt(n.timeout) || 1;
        this.ack == n.ack ? 1 : 0;
        this.retry = parseInt(n.retry) || 0;
        this.rtc = parseInt(n.rtc) || 0;
    }
    RED.nodes.registerType("rm92a-config", RM92AConfigNode);

    function SerialNode(n) {
        RED.nodes.createNode(this, n);
        this.serialport = n.serialport;
    }
    RED.nodes.registerType("rm92a-serial", SerialNode);
};
