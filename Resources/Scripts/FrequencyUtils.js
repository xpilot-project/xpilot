.pragma library

function checkFrequencyValid(frequency) {
    let num = frequency % 100000;
    if (frequency < 108000000 || frequency > 136975000) {
        throw "Invalid frequency range.";
    }
    if (num !== 0 && num !== 25000 && num !== 50000 && num !== 75000) {
        throw "Invalid frequency format. 8.33kHz frequencies not currently supported.";
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
    return normalize25KhzFrequency(num);
}

function printFrequency(frequency) {
    if(frequency > 0) {
        return (frequency / 1000000.0).toFixed(3);
    }
    return "---.---";
}
