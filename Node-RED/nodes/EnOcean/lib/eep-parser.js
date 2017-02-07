var RORGS = {
    0xA5: require("./eep/A5/entry"),
};

module.exports.get = function(profile) {
    var split = profile.split("-");
    if (split.length !== 3) {
        throw new Error("Invalid profile : " + profile);
    }

    var rorg = RORGS(parseInt(split[0], 16));
    if (!rorg) return null;

    var func = rorg(profile, parseInt(split[1], 16));
    if (!func) return null;

    var type = func(profile, parseInt(split[2], 16));
    if (!type) return null;

    return function(payload) {
        var telegramType = payload.telegramType;
        if (telegramType !== rorg) {
            throw new Error(profile + " allow telegram type " + rorg + ", but input telegram type " + telegramType);
        }

        return type(payload);
    };
};
