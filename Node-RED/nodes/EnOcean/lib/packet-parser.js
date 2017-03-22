var PacketType = {
    // reserved: 0x00,
    RADIO_ERP1: 0x01,
    RESPONSE: 0x02,
    RADIO_SUB_TES: 0x03,
    EVENT: 0x04,
    COMMON_COMMAND: 0x05,
    SMART_ACK_COMMAND: 0x06,
    REMOTE_MAN_COMMAND: 0x07,
    // reserved: 0x08,
    RADIO_MESSAGE: 0x09,
    RADIO_ERP2: 0x0A,
    // reserved: 0x0B - 0x0F,
    RADIO_802_15_4: 0x10,
    COMMAND_2_4: 0x11,
    // reserved: 0x12 - 0x7F,
    // available: 0x80 - 0xFF,
};

function handleRadioERP2(type, data, opt) {
    if (opt.length < 2) {
        throw new Error("ERP2 require 2 length at option");
    }

    var erp2 = new Buffer(data.length);
    data.copy(erp2);
    var telegram = opt.readUInt8(0);
    var rssi = -opt.readUInt8(1);

    return { type: type, erp2: erp2, telegram: telegram, rssi: rssi };
}

var PacketHandle = {};
PacketHandle[PacketType.RADIO_ERP1] = null;             // Unsupported
PacketHandle[PacketType.RESPONSE] = null;               // Unsupported
PacketHandle[PacketType.RADIO_SUB_TES] = null;          // Unsupported
PacketHandle[PacketType.EVENT] = null;                  // Unsupported
PacketHandle[PacketType.COMMON_COMMAND] = null;         // Unsupported
PacketHandle[PacketType.SMART_ACK_COMMAND] = null;      // Unsupported
PacketHandle[PacketType.REMOTE_MAN_COMMAND] = null;     // Unsupported
PacketHandle[PacketType.RADIO_MESSAGE] = null;          // Unsupported
PacketHandle[PacketType.RADIO_ERP2] = handleRadioERP2;
PacketHandle[PacketType.RADIO_802_15_4] = null;         // Unsupported
PacketHandle[PacketType.COMMAND_2_4] = null;            // Unsupported

function parse(esp3packet) {
    var type = esp3packet.type;
    var data = esp3packet.data;
    var opt = esp3packet.opt;

    var handle = PacketHandle[type];
    if (handle) {
        return handle(type, data, opt);
    } else {
        throw new Error("Unsupported packet type : " + type);
    }
}

module.exports.PacketType = PacketType;
module.exports.parse = parse;
