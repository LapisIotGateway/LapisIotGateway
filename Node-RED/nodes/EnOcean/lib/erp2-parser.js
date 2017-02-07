var CRC8 = require("./crc-util").CRC8;

var TelegramTypes = [0xF6, 0xD5, 0xA5, 0xD0, 0xD2, 0xD4, 0xD1, 0x30, 0x31, 0x35, 0xB3, null, null, null, null];
var ExtendTelegramTypes = [0xC5, 0xC6, 0xC7, 0x40, 0x32, 0xB0, 0xB1, 0xB2];

function parseShortContents(erp2, len) {
    var originatorID;
    var data = new Buffer();

    if (len === 1) {
        originatorID = erp2.toString('hex');
    } else if (len === 2) {
        originatorID = erp2.toString('hex', 0, 1);
        data.copy(erp2, 0, 1);
    } else if (len === 3) {
        originatorID = erp2.toString('hex', 0, 2);
        data.copy(erp2, 0, 2);
    } else if (len === 4) {
        originatorID = erp2.toString('hex', 0, 3);
        data.copy(erp2, 0, 3);
    } else if (len === 5 && len === 6) {
        originatorID = erp2.toString('hex', 0, 4);
        data.copy(erp2, 0, 4);
    } else {
        throw new Error("Don't reach");
    }

    return { originatorID: originatorID, data: data };
}

function parseLongContents(erp2, len) {
    var offset = 0;

    var crc = erp2.readUInt8(len-1);
    var act = CRC8(erp2, 0, len - 1);
    if (crc !== act) {
        throw new Error("Unexpected CRC : " + act + "exptected: " + crc);
    }

    var header = erp2.readUInt8(offset++);

    var addressControl = (header & 0xE0) >> 5;
    var extendHeaderAvailable = (header & 0x10) !== 0;
    var type = header & 0x0F;

    var optlen = 0;
    if (extendHeaderAvailable) {
        var extendHeader = erp2.readUInt8(offset++);
        optlen = (extendHeader & 0x0F);
    }

    var telegramType;
    if (type === 0x0F) {
        var extendTelegram = erp2.readUInt8(offset++);
        telegramType = ExtendTelegramTypes[extendTelegram];
    } else {
        telegramType = TelegramTypes[type];
    }

    var originatorID = null;
    var destinationID = null;
    if (addressControl === 0x0) {
        originatorID = erp2.toString('hex', offset, offset+3);
        offset += 3;
    } else if (addressControl === 0x1) {
        originatorID = erp2.toString('hex', offset, offset+4);
        offset += 4;
    } else if (addressControl === 0x2) {
        originatorID = erp2.toString('hex', offset, offset+4);
        offset += 4;
        destinationID = erp2.toString('hex', offset, offset+4);
        offset += 4;
    } else if (addressControl === 0x3) {
        originatorID = erp2.toString('hex', offset, offset+6);
        offset += 6;
    } else {
        // reserved
        throw new Error("Unexpected AddressControl : " + addressControl);
    }

    var datalen = len - offset - optlen - 1;
    var data = new Buffer(datalen);
    if (datalen > 0) {
        erp2.copy(data, 0, offset, offset + datalen);
        offset += datalen;
    }

    var opt = new Buffer(optlen);
    if (optlen > 0) {
        erp2.copy(opt, 0, offset, offset + optlen);
    }

    return {
        telegramType: telegramType,
        originatorID: originatorID,
        destinationID: destinationID,
        data: data,
        opt: opt };
}

function parse(erp2packet) {
    var erp2 = erp2packet.erp2;
    var len = erp2.length;

    if (len === 0) {
        return null;
    } else if (len <= 6) {
        return parseShortContents(erp2, len);
    } else {
        return parseLongContents(erp2, len);
    }
}

module.exports.parse = parse;
