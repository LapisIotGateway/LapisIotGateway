var CRC8 = require("./crc-util").CRC8;

var OFFSET_SYNC = 0;
var OFFSET_DATA_LEN = OFFSET_SYNC + 1;
var OFFSET_OPT_LEN = OFFSET_DATA_LEN + 2;
var OFFSET_TYPE = OFFSET_OPT_LEN + 1;
var OFFSET_HEADER_CRC = OFFSET_TYPE + 1;
var OFFSET_DATA = OFFSET_HEADER_CRC + 1;

var SYNC_VALUE = 0x55;

function ESP3Parser() {
    this._buffer = new Buffer(0);
}

ESP3Parser.prototype.parse = function(buffer) {
    this._buffer = Buffer.concat([this._buffer, buffer], this._buffer.length + buffer.length);

    var index = Array.prototype.indexOf.call(this._buffer, SYNC_VALUE);
    if (index === -1) {
        this._buffer = new Buffer(0);
        return null;
    } else if (index !== 0) {
        this._buffer = this._buffer.slice(index);
    }

    var target = this._buffer;

    // Required length for Header CRC
    if (target.length < OFFSET_HEADER_CRC + 1) {
        return null;
    }

    var datalen = target.readUInt16BE(OFFSET_DATA_LEN);
    var optlen = target.readUInt8(OFFSET_OPT_LEN);
    var type = target.readUInt8(OFFSET_TYPE);
    var length = OFFSET_HEADER_CRC + 1 + datalen + optlen + 1;

    // Length check
    if (target.length < length) {
        return null;
    }

    // Header CRC check
    var headerCRC = target.readUInt8(OFFSET_HEADER_CRC);
    var headerAct = CRC8(target, OFFSET_DATA_LEN, 4);
    if (headerCRC !== headerAct) {
        this._buffer = this._buffer.slice(1);
        throw new Error("Unexpected header CRC : " + headerAct + ", exptected: " + headerCRC);
    }

    // Data CRC check
    var dataCRC = target.readUInt8(OFFSET_DATA + datalen + optlen);
    var dataAct = CRC8(target, OFFSET_DATA, datalen + optlen);
    if (dataCRC !== dataAct) {
        this._buffer = this._buffer.slice(1);
        throw new Error("Unexpected header CRC : " + headerAct + ", exptected: " + headerCRC);
    }

    var data = new Buffer(datalen);
    target.copy(data, 0, OFFSET_DATA, OFFSET_DATA + datalen);
    var opt = new Buffer(optlen);
    if (optlen > 0) {
        target.copy(opt, 0, OFFSET_DATA + datalen, OFFSET_DATA + datalen + optlen);
    }

    this._buffer = this._buffer.slice(length);

    return { type: type, data: data, opt: opt };
};

module.exports = ESP3Parser;
