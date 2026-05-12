#include <Servo.h>

Servo motorTaban;  
Servo motorOmuz;   
Servo motorDirsek; 
Servo motorKiskac; 

const int pinTaban = 3;
const int pinOmuz = 5;
const int pinDirsek = 6;
const int pinKiskac = 9;
const int pinBuzzer = 10;

// Mevcut ve Hedef Pozisyonlar
int suankiTaban = 90;
int suankiOmuz = 90;
int hedefTaban = 90;
int hedefOmuz = 90;

unsigned long sonHareketZamani = 0;
const int hareketGecikmesi = 15; // Her 15ms'de 1 derece hareket (Hız ayarı)

void setup() {
  Serial.begin(9600);
  pinMode(pinBuzzer, OUTPUT);

  motorTaban.attach(pinTaban);
  motorOmuz.attach(pinOmuz);
  motorDirsek.attach(pinDirsek);
  motorKiskac.attach(pinKiskac);

  // Başlangıç Pozisyonu
  motorTaban.write(suankiTaban);  
  motorOmuz.write(suankiOmuz);   
  motorDirsek.write(90); 
  motorKiskac.write(30); 

  // Açılış Sesi
  tone(pinBuzzer, 1500, 100);
  delay(150);
  tone(pinBuzzer, 2000, 200);
}

void loop() {
  // Yeni hedef kontrolü
  if (Serial.available() > 0) {
    int x = Serial.parseInt(); 
    int y = Serial.parseInt();

    if (x > 0 && y > 0) {
      tone(pinBuzzer, 1200, 50); // Komut alındı sesi
      hedefTaban = map(x, 0, 640, 180, 0); 
      hedefOmuz = map(y, 0, 480, 130, 50); 
    }
  }

  // Yumuşak Hareket (Smoothing) Mantığı
  if (millis() - sonHareketZamani > hareketGecikmesi) {
    bool hareketVar = false;

    // Taban motoru ilerlemesi
    if (suankiTaban < hedefTaban) { suankiTaban++; hareketVar = true; }
    else if (suankiTaban > hedefTaban) { suankiTaban--; hareketVar = true; }

    // Omuz motoru ilerlemesi
    if (suankiOmuz < hedefOmuz) { suankiOmuz++; hareketVar = true; }
    else if (suankiOmuz > hedefOmuz) { suankiOmuz--; hareketVar = true; }

    if (hareketVar) {
      motorTaban.write(suankiTaban);
      motorOmuz.write(suankiOmuz);
      sonHareketZamani = millis();
    }
  }
}
