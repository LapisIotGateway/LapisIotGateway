// name: Collect Toami Data
// outputs: 1

/*globals msg, context */
var startTrigger = "@start";
var sendTrigger = "@send";

var keys = [
    "n01", "n02", "n03", "n04", "n05", "n06", "n07", "n08", "n09", "n10",
    "n11", "n12", "n13", "n14", "n15", "n16", "n17", "n18", "n19", "n20",
    "n21", "n22", "n23", "n24", "n25", "n26", "n27", "n28", "n29", "n30",
    "s01", "s02", "s03", "s04", "s05", "s06", "s07", "s08", "s09", "s10",
    "l01",
];

var timestampKey = "gwtimestamp";
var toami;

var payload = msg.payload;
if (msg.payload === startTrigger) {
    context.temp = {};
    return null;
} else if (msg.payload === sendTrigger) {
    if (!context.temp) {
        return null;
    }

    toami = context.temp;
    context.temp = null;

    if (Object.keys(toami).length === 0) {
        return null;
    }

    toami[timestampKey] = Date.now();
    return { payload: toami };
} else {
    if (!context.temp) {
        return null;
    }

    keys.forEach(function(key) {
        if (payload.hasOwnProperty(key)) {
            context.temp[key] = payload[key];
        }
    });
    return null;
}