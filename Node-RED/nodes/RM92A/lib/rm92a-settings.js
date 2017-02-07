var Promise = require("es6-promise").Promise;

var MODE_LORA = 1;
var NL = "\r";

var SETTING_SEND_RATE = 100;

var DEFAULT_CONFIG = {
    mode: MODE_LORA,
    ch: 24,
    panid: 0x1234,
    src: 0x0001,
    dst: 0x00FF,
    parent: 1,
    routing: 2,
    power: 2,
    bandwidth: 0,
    factor: 0,
    bitrate: 50000,
    ack: 0,
    timeout: 1,
    retry: 0,
    carrier: 3,
};

module.exports = function(port, params) {
    var config = {};
    Object.keys(DEFAULT_CONFIG).forEach(function(key) {
        return config[key] = params[key] || DEFAULT_CONFIG[key]; });

    return setting(config).reduce(function(p, value) {
            return String(value).split("").reduce(function(p2, char) {
                return p2.then(settingWrite(port, char));
            }, p);
        }, Promise.resolve())
        .then(setTimeoutPromiseFunction(function(resolve) {
            resolve();
        }, SETTING_SEND_RATE));
};

function setTimeoutPromiseFunction(f, timeout) {
    return function() {
        return setTimeoutPromise(f, timeout); };
}

function setTimeoutPromise(f, timeout) {
    return new Promise(function(resolve, reject) {
        setTimeout(function() {
            f(resolve, reject);
        }, timeout);
    });
}

function settingWrite(port, data) {
    return setTimeoutPromiseFunction(function(resolve) {
        port.write(data);
        resolve();
    }, SETTING_SEND_RATE);
}

function setting(config) {
    var params = [NL];
    // mode(LoRa/FSK)
    if (config.mode === MODE_LORA) {
        params.push(1, NL);
    } else {
        params.push(2, NL);
    }
    // ch
    params.push("a", config.ch, NL);
    // panid
    params.push("b", 1, NL, config.panid, NL);
    // src address
    params.push("c", config.src, NL);
    // dst address
    params.push("d", config.dst, NL);
    // parent or child (Parent)
    params.push("e", config.parent, NL);
    // routing (Non Routing)
    params.push("f", config.routing, NL);
    // RF
    params.push("g", 1, NL, config.power, NL);
    if (config.mode === MODE_LORA) {
        params.push("g", 2, NL, config.bandwidth, NL);
        params.push("g", 3, NL, config.factor, NL);
    } else {
        params.push("g", 2, NL, config.bitrate, NL);
    }
    // ACK
    if (config.ack === 0) {
        params.push("h", 0, NL);
    } else {
        params.push("h", 1, NL, config.timeout, NL, config.retry, NL);
    }
    // send setting (Frame mode)
    params.push("i", 1, NL);
    // sleep (Disable)
    params.push("j", 0, NL);
    // recv setting (Enable RSSI / src addr)
    params.push("l", 1, NL, 1, NL);
    params.push("l", 2, NL, 1, NL);
    // carrier sense (Enable)
    params.push("m", 1, NL, config.carrier, NL);
    // AES (Disable)
    params.push("n", 0, NL);
    // system start
    params.push("s");
    return params;
}
