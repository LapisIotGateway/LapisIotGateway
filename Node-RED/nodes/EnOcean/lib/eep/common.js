module.exports.rangeScaleParse = function parse(eep, target, conf, data) {
    var range = conf.range;
    var scale = conf.scale;
    var offset = conf.offset;
    var size = conf.size;
    var unit = conf.unit;

    var temp;
    var value = 0;
    for (var i = 0; i < 4; i++) {
        if (offset >= 8) {
            offset -= 8;
            continue;
        } else if (offset > 0) {
            temp = data.readUInt8(i) << offset;
            offset -= 8;
        } else {
            temp = data.readUInt8(i);
        }

        if (size > 8) {
            value += (temp << 8);
        } else {
            value += (temp >> (8 - size));
            break;
        }
    }

    var positive = (range[0] < range[1]);
    var max = Math.max(range[0], range[1]);
    var min = Math.min(range[0], range[1]);

    if (value < min || max < value) {
        throw new Error(eep + " allow " + target + " " + min + "～" + max + ", but " + value);
    }

    var fix = (value - min) * (Math.abs(scale[0] - scale[1])) / (max - min);
    if (positive) {
        fix += scale[0];
    } else {
        fix = scale[1] - fix;
    }

    return {
        value: fix,
        unit: unit,
    };
};