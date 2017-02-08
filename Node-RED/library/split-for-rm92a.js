// name: Split for RM92A
// outputs: 1

/* globals msg, node, util */
var max = 93;
var split = max;

var payload = msg.payload;
var num;
var digit = 1;
for (;; digit++) {
    num = Math.ceil(payload.length / split);
    if (num >= Math.pow(10, digit)) {
        split = split - 2;
    } else {
        break;
    }
}

if (split < 0) {
    node.error("Can't split payload, because payload too long(" + payload.length + ").", msg);
    return null;
}

var zeroPadding = "";
var i;
for (i = 0; i < digit; i++) {
    zeroPadding += "0";
}

var seq;
var amount = (zeroPadding + num).slice(-digit);
var data;
var format;
var msgs = [];
for (i = 0; i < num; i++) {
    seq = (zeroPadding + i).slice(-digit);
    data = payload.slice(split * (i), Math.min(split * (i+1), payload.length));
    format = util.format("@F;%s;%s;%s", seq, amount, data);
    msgs.push({payload: format});
}

return [msgs];