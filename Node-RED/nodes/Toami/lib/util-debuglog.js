var util = require("util");

var debugs = {};
var debugEnviron = process.env.NODE_DEBUG || "";
var debuglog = util.debuglog || function(set) {
    set = set.toUpperCase();
    if (!debugs[set]) {
        if (new RegExp("\\b" + set + "\\b", "i").test(debugEnviron)) {
            debugs[set] = function() {
                var msg = util.format.apply(exports, arguments);
                console.error("%s : %s", set, msg);
            };
        } else {
            debugs[set] = function() {};
        }
    }
    return debugs[set];
};

module.exports = debuglog;
