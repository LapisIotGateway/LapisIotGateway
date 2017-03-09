// name: Translate for CSV
// outputs: 1

function translate(value) {
    return ("0" + value).slice(-2)
}

var obj = Object.keys(msg.payload).reduce(function(obj, key) {
    obj[key] = msg.payload[key];
    return obj;
}, {});

var date = new Date(obj.gwtimestamp)
obj.date = util.format(
  "%d-%s-%s %s:%s:%s",
  date.getFullYear(),
  translate(date.getMonth() + 1),
  translate(date.getDate()),
  translate(date.getHours()),
  translate(date.getMinutes()),
  translate(date.getSeconds())
);

if (obj.l01) {
    obj.la = obj.l01.latitude;
    obj.lo = obj.l01.longitude;
}

msg.payload = obj

return msg;
