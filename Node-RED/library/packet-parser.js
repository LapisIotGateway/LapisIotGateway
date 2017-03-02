// name: Packet Parser
// outputs: 1

// $1 : Src address
// $2 : GNRMC
// $3 : Temperature
// $4 : Humidity
// $5 : Atmospheric pressure
var regex = /^([0-9a-fA-F]+),(?:\$GNRMC,(.*),)?BME280,([0-9.]*),([0-9.]*),([0-9.]*)$/;

// $1  : UTC Time
// $2  : A:Valid, V:Invalid
// $3  : Latitude
// $4  : N:North, S:South
// $5  : Longitude
// $6  : E:East, W:West
// $7  : Speed over ground
// $8  : Course over ground
// $9  : UTC Date
// $10 : Magnetic variation
// $11 : Magnetic variation direction (E:East, W:West)
// $12 : Positioning system mode indicator
// $13 : Checksum
var rmc = /^([0-9.]*),(A|V),([0-9.]*),(N|S),([0-9.]*),(E|W),([0-9.]*),([0-9.]*),(\d*),([0-9.]*),(E?|W?),(A|D|E|M|S|N)\*([0-9A-F]{2})$/;

//  $1 : Degree
//  $2 : Minutes
//  $3 : Seconds
var dms = /(\d*)(\d{2})\.(\d*)/;

/* globals msg, node */
var rssi = msg.rssi;
var payload = msg.payload;

// payload parse
var m = regex.exec(payload);
if (!m) {
    node.error("Invalid msg : " + payload, msg);
    return;
}

var src = parseInt(m[1], 16);
var gnrmc = m[2];
var temperature = parseFloat(m[3]);
var humidity = parseFloat(m[4]);
var pressure = parseFloat(m[5]);

var result = {
    src: src,
    rssi: rssi,
    payload: {
        temperature: temperature,
        humidity: humidity,
        pressure: pressure,
    },
};

var location = (function(gnrmc) {

    function translate(value) {
        var degrees = parseInt(value[1], 10);
        var minutes = parseInt(value[2], 10);
        var seconds = parseFloat(value[3].replace(/(\d{2})(\d*)/, "$1.$2"));
        return degrees + (minutes / 60) + (seconds / 3600);
    }

    if (!gnrmc) {
        return;
    }

    var m = rmc.exec(gnrmc);
    if (!m) {
        node.error("Invalid gnrmc : " + gnrmc, msg);
        return;
    }

    var latitudeValue = m[3];
    var latitudeDirection = m[4];

    var longitudeValue = m[5];
    var longitudeDirection = m[6];

    m = dms.exec(latitudeValue);
    if (!m) {
        node.error("Invalid latitude : " + latitudeValue , msg);
        return;
    }
    var latitude = translate(m);

    m = dms.exec(longitudeValue);
    if (!m) {
        node.error("Invalid longitude : " + longitudeValue , msg);
        return;
    }
    var longitude = translate(m);

    if (latitudeDirection === "S") {
        latitude *= -1;
    }

    if (longitudeDirection === "W") {
        longitude *= -1;
    }

    return { latitude: latitude, longitude: longitude };
}(gnrmc));

if (location) {
    result.payload.location = location;
}

return result;