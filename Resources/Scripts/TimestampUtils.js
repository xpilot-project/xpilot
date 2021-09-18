.pragma library

function currentTimestamp() {
    var dt = new Date();
    const h = dt.getUTCHours() < 10 ? "0" + dt.getUTCHours() : dt.getUTCHours();
    const m = dt.getUTCMinutes() < 10 ? "0" + dt.getUTCMinutes() : dt.getUTCMinutes();
    const s = dt.getUTCSeconds() < 10 ? "0" + dt.getUTCSeconds() : dt.getUTCSeconds();
    return h + ":" + m + ":" + s;
}
