.pragma library

function checkFrequencyValid(frequency) {
    let num = frequency % 100000;
    if (frequency < 118000000 || frequency > 136975000) {
        throw "Invalid frequency range.";
    }
    return frequency;
}

function normalize25KhzFrequency(freq) {
    if (!isNaN(freq)) {
        if (freq < 100000000) {
            return freq;
        }
        if (freq % 100000 == 20000 || freq % 100000 == 70000) {
            return freq + 5000;
        }
    } else {
        if (String(freq).indexOf(".") < 3) {
            return freq;
        }
        let freq2 =
            parseInt(String(freq).replace(".", "")) *
            Math.pow(10.0, 9 - (String(freq).length - 1));
        let text = normalize25KhzFrequency(freq2).toString();
        freq = text.substring(0, 3) + "." + text.substring(3);
    }
    return checkFrequencyValid(freq);
}

function frequencyToInt(freq) {
    let num = Math.round(parseFloat(freq.trim()) * 1000000.0);
    num = checkFrequencyValid(num);
    return toXplaneFormat(normalize25KhzFrequency(num));
}

function toXplaneFormat(freq) {
    if(freq < 1000000) {
        return freq;
    }
    return freq / 1000;
}

function printFrequency(frequency) {
    if(frequency > 0) {
        return (frequency / 1000.0).toFixed(3);
    }
    return "---.---";
}

function fromNetworkFormat(freq) {
    return ((freq + 100000.0) / 1000.0).toFixed(3);
}
