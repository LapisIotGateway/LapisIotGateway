var common = require("../common");
var rangeScaleParse = common.rangeScaleParse;

var TYPES = {
    0x05: {
        type: "Temperature Sensor Range 0℃ to +40℃",
        temperature: {
            range: [255, 0],
            scale: [0, 40],
            offset: 16,
            size: 8,
            unit: "celsius"
        },
    }
};

module.exports = function(eep, type) {
    var conf = TYPES[type];
    if (!conf) return null;

    return function(payload) {
        var data = payload.data;

        var temperature = rangeScaleParse(eep, "temperature", conf.temperature, data);
        return {
            func: "Temperature Sensors",
            type: conf.type,
            temperature: temperature
        };
    };
};