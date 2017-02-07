var util = require("util");
var EventEmitter = require("events").EventEmitter;
var Promise = require("es6-promise").Promise;

var between = function(value, min, max) {
    return min <= value && value <= max;
};

var Debug = require("./util-debuglog")("slr429");

var PromiseWrapper = function() {
    this.promise = Promise.resolve();
};

PromiseWrapper.prototype.then = function(onFulfilled, onRejected) {
    this.promise = this.promise.then(onFulfilled, onRejected);
    return this;
};

PromiseWrapper.prototype.catch = function(onRejected) {
    this.promise = this.promise.catch(onRejected);
    return this;
};

function Readline(delimiter, encoding) {
    EventEmitter.call(this);
    this._stringBuffer = "";
    this._delimiter = delimiter;
    this._encoding = encoding;
}
util.inherits(Readline, EventEmitter);

Readline.prototype.append = function(buffer) {
    var self = this;

    this._stringBuffer += buffer.toString(this._encoding);
    var parts = this._stringBuffer.split(this._delimiter);
    this._stringBuffer = parts.pop();
    parts.forEach(function(part) { return self.emit("data", part); });
};

exports.between = between;
exports.Debug = Debug;
exports.PromiseWrapper = PromiseWrapper;
exports.Readline = Readline;
