.pragma library

function checkFrequencyValid(frequency) {
    if (frequency < 118000000 || frequency > 136975000) {
        throw "Invalid frequency range.";
    }
    return frequency;
}

function normalize25KhzFrequency(freq) {
    if(!isNaN(freq)) {
        if(freq > 1000000) {
            freq /= 1000.0
        }
        if (freq % 100 == 20 || freq % 100 == 70) {
            freq += 5
        }
        return freq
    }
    else {
        if (String(freq).indexOf(".") < 3) {
            return freq;
        }
        let freq2 = parseInt(String(freq).replace(".", "")) * Math.pow(10.0, 9 - (String(freq).length - 1));
        let text = normalize25KhzFrequency(freq2).toString();
        return text.substring(0, 3) + "." + text.substring(3);
    }
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
        return (normalize25KhzFrequency(frequency) / 1000.0).toFixed(3);
    }
    return "---.---";
}

function fromNetworkFormat(freq) {
    return (normalize25KhzFrequency(freq + 100000.0) / 1000.0).toFixed(3)
}
