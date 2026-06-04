const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const { SerialPort } = require('serialport');

const portAdi = 'COM3'; // Kendi portuna göre ayarla

const arduinoPort = new SerialPort({
    path: portAdi,
    baudRate: 115200,
    autoOpen: false
});

// Arduino'dan gelen log ve hata mesajlarını sunucu terminaline yazdır
arduinoPort.on('data', (data) => {
    console.log(`🤖 ARDUINO -> ${data.toString().trim()}`);
});

// Port kapandığında tetiklenir (Kablo çekilmesi vb.)
arduinoPort.on('close', () => {
    sonBaglantiDurumu = false;
    baglantiDeneniyor = false;
    io.emit('arduinoDurum', false);
    console.log(`❌ Arduino (${portAdi}) BAĞLANTISI KOPTU!`);
});

// Hataları ekrana bas ama Node.js'in çökmesini engelle
arduinoPort.on('error', (err) => {
    if (err.message.includes('File not found') || err.message.includes('Access denied')) {
        sonBaglantiDurumu = false;
        io.emit('arduinoDurum', false);
    }
});

app.use(express.static(__dirname));
app.get('/', (req, res) => { res.sendFile(__dirname + '/index.html'); });

let sonBaglantiDurumu = false;
let baglantiDeneniyor = false;

setInterval(async () => {
    try {
        const ports = await SerialPort.list();
        const portSistemdeVarMi = ports.some(p => p.path === portAdi);
        let anlikDurum = arduinoPort.isOpen && portSistemdeVarMi;

        // Eğer port sistemde yok ama biz açık sanıyorsak, zorla kapat
        if (!portSistemdeVarMi && arduinoPort.isOpen) {
            arduinoPort.close();
            anlikDurum = false;
        }

        if (anlikDurum !== sonBaglantiDurumu) {
            sonBaglantiDurumu = anlikDurum;
            io.emit('arduinoDurum', anlikDurum);
            if (anlikDurum) {
                console.log(`🔌 Arduino (${portAdi}) BAĞLANDI.`);
            } else {
                console.log(`❌ Arduino (${portAdi}) KOPTU!`);
            }
        }

        // Yeniden bağlanma denemesi
        if (!anlikDurum && !baglantiDeneniyor && portSistemdeVarMi) {
            baglantiDeneniyor = true;
            arduinoPort.open(() => { baglantiDeneniyor = false; });
        }
    } catch (err) {
        console.error("Port kontrol hatası:", err);
    }
}, 1000); 

io.on('connection', (socket) => {
    socket.emit('arduinoDurum', sonBaglantiDurumu);

    // Manuel kontrol verisini Arduino'ya gönder (Klavye veya Slider)
    socket.on('manuelKontrol', (veri) => {
        if (sonBaglantiDurumu) {
            arduinoPort.write(`K,${veri.taban},${veri.omuz},${veri.dirsek},${veri.kiskac}\n`, (err) => {
                if (err) {
                    console.log("Manuel kontrol gönderim hatası:", err.message);
                }
            });
        }
    });

    // Geriye dönük uyumluluk için klavyeKontrol desteğini koru
    socket.on('klavyeKontrol', (veri) => {
        if (sonBaglantiDurumu) {
            arduinoPort.write(`K,${veri.taban},${veri.omuz},${veri.dirsek},${veri.kiskac}\n`, (err) => {
                if (err) {
                    console.log("Klavye gönderim hatası:", err.message);
                }
            });
        }
    });

    // Buzzer çalma emri gönder
    socket.on('buzzerCal', (veri) => {
        if (sonBaglantiDurumu) {
            const frekans = veri.frekans || 1000;
            const sure = veri.sure || 100;
            arduinoPort.write(`B,${frekans},${sure}\n`, (err) => {
                if (err) {
                    console.log("Buzzer gönderim hatası:", err.message);
                }
            });
        }
    });
});

http.listen(3000, () => {
    console.log('🚀 Ana Sunucu Çalışıyor: http://localhost:3000');
});
