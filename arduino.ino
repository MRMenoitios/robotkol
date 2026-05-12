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
int suankiTaban = 90, hedefTaban = 90;
int suankiOmuz = 90, hedefOmuz = 90;
int suankiDirsek = 90, hedefDirsek = 90;

unsigned long sonHareketZamani = 0;
const int hareketGecikmesi = 15; 

void setup() {
  Serial.begin(9600);
  pinMode(pinBuzzer, OUTPUT);

  motorTaban.attach(pinTaban);
  motorOmuz.attach(pinOmuz);
  motorDirsek.attach(pinDirsek);
  motorKiskac.attach(pinKiskac);

  motorTaban.write(suankiTaban);  
  motorOmuz.write(suankiOmuz);   
  motorDirsek.write(suankiDirsek); 
  motorKiskac.write(30); 

  tone(pinBuzzer, 1500, 100); delay(150);
  tone(pinBuzzer, 2000, 200);
}

void loop() {
  if (Serial.available() > 0) {
    int x = Serial.parseInt(); 
    int y = Serial.parseInt();
    int z = Serial.parseInt(); // Z-Ekseni (Uzanma)

    if (x > 0 && y > 0) {
      tone(pinBuzzer, 1200, 50);
      hedefTaban = map(x, 0, 640, 180, 0); 
      hedefOmuz = map(y, 0, 480, 130, 50); 
      hedefDirsek = map(z, 0, 200, 40, 140); // Z derinliği Dirsek motoruna
    }
  }

  if (millis() - sonHareketZamani > hareketGecikmesi) {
    bool hareketVar = false;

    if (suankiTaban < hedefTaban) { suankiTaban++; hareketVar = true; }
    else if (suankiTaban > hedefTaban) { suankiTaban--; hareketVar = true; }

    if (suankiOmuz < hedefOmuz) { suankiOmuz++; hareketVar = true; }
    else if (suankiOmuz > hedefOmuz) { suankiOmuz--; hareketVar = true; }

    if (suankiDirsek < hedefDirsek) { suankiDirsek++; hareketVar = true; }
    else if (suankiDirsek > hedefDirsek) { suankiDirsek--; hareketVar = true; }

    if (hareketVar) {
      motorTaban.write(suankiTaban);
      motorOmuz.write(suankiOmuz);
      motorDirsek.write(suankiDirsek);
      sonHareketZamani = millis();
    }
  }
}