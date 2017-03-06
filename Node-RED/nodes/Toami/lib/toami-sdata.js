var events = require("events");
var https = require("https");
var toami = require("./toami-const");

var common = toami.common;
var sdata = toami.properties.sdata;

var DEBUG = require("./util-debuglog")("toami");

function ToamiSdata(key, host, gateway, timeout) {
    this._host = common.domain(host);
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
    var timer = null;

    DEBUG("[Toami] Request Start", options, content);
    var req = https.request(options, function(res) {
        DEBUG("[Toami] Request Complete", res.statusCode);
        if (timer) {
            clearTimeout(timer);
            timer = null;
        }
        var code = res.statusCode;
        if (code === 200) {
            emitter.emit("ok");
        } else {
            emitter.emit("ng", code, res.statusMessage, options);
        }
    });
    req.on("error", function(err) {
        DEBUG("[Toami] Request Error", err);
        if (timer) {
            clearTimeout(timer);
            timer = null;
            emitter.emit("error", err, options);
        }
    });
    timer = setTimeout(function() {
        DEBUG("[Toami] Request Timeout");
        timer = null;
        setImmediate(function() {
            emitter.emit("timeout");
        });
        req.abort();
    }, timeout);
    req.write(JSON.stringify(content));
    req.end();

    return emitter;
};

module.exports = ToamiSdata;
