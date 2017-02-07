var util = require("util");

var domainFormat = "%s.to4do.com";
var pathFormat = "/Thingworx/Things/%s";

var common = {
    domain: function(host) {
        return util.format(domainFormat, host);
    },
    port: 443,
};

var propertiesPathFormat = pathFormat + "/Properties";
var properties = {
    sdata: {
        method: "PUT",
        headers: function(key) {
            return {
                "Content-type" : "application/json;charset=UTF8",
                "Accept" : "application/json",
                "appKey" :key,
            };
        },
        path: function(gw) {
            return util.format(propertiesPathFormat + "/sdata", gw);
        },
        keys : [
        "n01", "n02", "n03", "n04", "n05", "n06", "n07", "n08", "n09", "n10",
        "n11", "n12", "n13", "n14", "n15", "n16", "n17", "n18", "n19", "n20",
        "n21", "n22", "n23", "n24", "n25", "n26", "n27", "n28", "n29", "n30",
        "s01", "s02", "s03", "s04", "s05", "s06", "s07", "s08", "s09", "s10",
        "l01",
        "gwtimestamp",
        ],
    },
};

module.exports.common = common;
module.exports.properties = properties;