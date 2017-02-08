var common = require("../common");
var rangeScaleParse = common.rangeScaleParse;

var TYPES = {
    0x01: {
        type: "Range 0℃ to +40℃ and 0% to 100%",
        humidity: {
            range: [0, 250],
            scale: [0, 100],
            offset: 8,
            size: 8,
            unit: "percentage"
        },
        temperature: {
            range: [0, 250],
            scale: [0, 40],
            offset: 16,
            size: 8,
            unit: "celsius"
        },
    },
};

module.exports = function(eep, type) {
    var conf = TYPES[type];
    if (!conf) return null;

    return function(payload) {
        var data = payload.data;

        var humidity = rangeScaleParse(eep, "humidity", conf.humidity, data);
        var temperature = rangeScaleParse(eep, "temperature", conf.temperature, data);
        return {
            func: "Temperature and Humidity Sensor",
            humidity: humidity,
            temperature: temperature,
        };
    };
};