var events = require("events");
var https = require("https");
var toami = require("./toami-const");

var common = toami.common;
var sdata = toami.properties.sdata;

function ToamiSdata(key, host, gateway, timeout) {
    this._host = common.domain(sdata);
    this._path = sdata.path(gateway);
    this._key = key;
    this._timeout = timeout;
}

ToamiSdata.prototype.request = function(data) {
    var host = this._host;
    var path = this._path;
    var key = this._key;
    var timeout = this._timeout;

    var options = {
        host: host,
        port: common.port,
        method: sdata.method,
        path: path,
        headers: sdata.headers(key),
    };

    var content = {};
    content.sdata = sdata.keys.reduce(function(obj, key) {
        if (data[key]) {
            obj[key] = data[key];
        }
        return obj;
    }, {});

    var emitter = new events.EventEmitter();
    var req = https.request(options, function(res) {
        var code = res.statusCode;
        if (code === 200) {
            emitter.emit("ok");
        } else {
            emitter.emit("ng", code, res.statusMessage);
        }
    });
    req.on("error", function(err) {
        emitter.emit("error", err);
    });
    req.setTimeout(timeout, function() {
        setImmediate(function() {
            emitter.emit("timeout");
        });
        req.abort();
    });
    req.write(JSON.stringify(content));
    req.end();

    return emitter;
};

module.exports = ToamiSdata;