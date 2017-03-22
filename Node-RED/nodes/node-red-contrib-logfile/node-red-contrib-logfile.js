
var filePath=""

module.exports = function(RED) {
	function translate(value) {
	    return ("0" + value).slice(-2)
	}

	function getFileName(id,extention) {
		var util = require("util");
		var date = new Date(Date.now());
		if(filePath=="") return filePath;
		if(filePath.slice(-1)=="/") {
			filePath = filePath.slice(0,filePath.length-1);
		}
		if(id) id=String(id);
		else id=""
		var fileName = util.format("%s/%d%s%s%s%s.%s",
			filePath,
			date.getFullYear(),
			translate(date.getMonth() + 1),
			translate(date.getDate()),
			translate(date.getHours()),
			id,
			extention);
		return fileName;
	}

	function logFile(config) {
		RED.nodes.createNode(this,config);
		var node = this;
		filePath=config.filePath;

		this.on('input', function(msg) {
			var newMsg = {};
			var fileName = getFileName(msg.id,"csv");
			if(fileName != "") {
				newMsg.filename = fileName;
				newMsg.payload = msg.payload;
			}
			node.send(newMsg);
		});
	}
	RED.nodes.registerType("log file",logFile);
}
