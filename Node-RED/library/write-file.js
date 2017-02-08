// name: Write File
// outputs: 1

/*globals msg, context, util */
var payload = msg.payload;
var dir = "/home/pi/toami";

if (Number.isFinite(payload)) {
    // timestamp
    var date = new Date(payload);
    context.filename = util.format("%s/toami_%d%s%s%s%s%s.csv",
        dir,
        date.getFullYear(),
        ("0" + (date.getMonth() + 1)).slice(-2),
        ("0" + date.getDate()).slice(-2),
        ("0" + date.getHours()).slice(-2),
        ("0" + date.getMinutes()).slice(-2),
        ("0" + date.getSeconds()).slice(-2));
    return null;
} else {
    // csv
    return {
        filename: context.filename,
        payload: msg.payload,
    };
}