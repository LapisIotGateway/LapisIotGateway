var FUNCS = {
    0x02: require("./temperature-sensors"),
    0x04: require("./temperature-humidity-sonsors"),
};

module.exports = function(eep, func) {
    return function(payload) {
        var data = payload.data;
        if (data.length !== 4) {
            throw new Error(eep + " allow 4 byte data, but input data length " + data.length);
        }

        return FUNCS[func];
    };
};