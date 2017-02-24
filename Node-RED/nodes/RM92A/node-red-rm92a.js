module.exports = function(RED) {
    RED.nodes.registerType("rm92a in", require("./lib/node-red-rx-node")(RED));
    RED.nodes.registerType("rm92a out", require("./lib/node-red-tx-node")(RED));

    function RM92AConfigNode(n) {
        RED.nodes.createNode(this, n);
        this.reset = n.reset;
        this.mode = n.mode ? parseInt(n.mode) : 1;
        this.ch = n.ch ? parseInt(n.ch) : 24;
        this.panid = n.panid ? parseInt(n.panid) : 0x1234;
        this.src = n.src ? parseInt(n.src) : 0x0001;
        this.dst = n.dst ? parseInt(n.dst) : 0x00FF;
        this.pw = n.pw ? parseInt(n.pw) : 0;
        this.bw = n.bw ? parseInt(n.bw) : 0;
        this.sf = n.sf ? parseInt(n.sf) : 0;
        this.bitrate = n.bitrate ? parseInt(n.bitrate) : 50000;
        this.timeout = n.timeout ? parseInt(n.timeout) : 1;
        this.ack == n.ack ? 1 : 0;
        this.retry = n.retry ? parseInt(n.retry) : 0;
        this.rtc = n.rtc ? parseInt(n.rtc) : 0;
    }
    RED.nodes.registerType("rm92a-config", RM92AConfigNode);

    function SerialNode(n) {
        RED.nodes.createNode(this, n);
        this.serialport = n.serialport;
    }
    RED.nodes.registerType("rm92a-serial", SerialNode);
};
