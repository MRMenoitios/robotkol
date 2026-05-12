const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const { SerialPort } = require('serialport');

const portAdi = 'COM3'; // Kendi portuna göre ayarla

const arduinoPort = new SerialPort({
    path: portAdi,
    baudRate: 9600,
    autoOpen: false
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

    socket.on('hedefKilitlendi', (veri) => {
        if (sonBaglantiDurumu) {
            console.log(`🎯 EMİR -> ${veri.isim} | X:${veri.x}, Y:${veri.y}, Z:${veri.z}`);
            arduinoPort.write(`${veri.x},${veri.y},${veri.z}\n`, (err) => {
                if (err) {
                    console.log("Gönderim hatası:", err.message);
                    sonBaglantiDurumu = false;
                    io.emit('arduinoDurum', false);
                }
            });
        }
    });
});

http.listen(3000, () => {
    console.log('🚀 Ana Sunucu Çalışıyor: http://localhost:3000');
});