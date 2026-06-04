#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
int suankiDirsek = 90;
int suankiKiskac = 30;
int hedefTaban = 90;
int hedefOmuz = 90;
int hedefDirsek = 90;
int hedefKiskac = 30;

unsigned long sonHareketZamani = 0;
unsigned long sonHareketZamaniTaban = 0;
unsigned long sonHareketZamaniOmuz = 0;
unsigned long sonHareketZamaniDirsek = 0;
unsigned long sonHareketZamaniKiskac = 0;
const int hareketGecikmesi = 15; // Her 15ms'de 1 derece hareket (Hız ayarı)

bool isMoving = false;
String lastStatus = "BOSTA";

// Servo kütüphanesi write() komutunda 200 gibi değerleri milisaniye zannedip
// 0'a çöker. Bunu aşmak ve 180 derece üstü özel açılara izin vermek için
// mikrosaniye haritalaması kullanıyoruz:
void safeServoWrite(Servo &motor, int angle, int maxAngle) {
  int pulse = map(angle, 0, maxAngle, 450, 2550);
  motor.writeMicroseconds(pulse);
}

void safeServoDetach(Servo &motor, int pin) {
  if (motor.attached()) {
    motor.detach();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Üst Başlık
  display.setCursor(0, 0);
  display.print("MANUEL KONTROL PANEL");

  // Pozisyon Bilgileri
  display.setCursor(0, 10);
  display.print("T:");
  display.print(suankiTaban);
  display.print(" O:");
  display.print(suankiOmuz);

  display.setCursor(64, 10);
  display.print("D:");
  display.print(suankiDirsek);
  display.print(" K:");
  display.print(suankiKiskac);

  // Durum
  display.setCursor(0, 22);
  display.print("DURUM: ");
  display.print(lastStatus);

  display.display();
}

void attachServos() {
  if (!motorTaban.attached())
    motorTaban.attach(pinTaban);
  if (!motorOmuz.attached())
    motorOmuz.attach(pinOmuz);
  if (!motorDirsek.attached())
    motorDirsek.attach(pinDirsek);
  if (!motorKiskac.attached())
    motorKiskac.attach(pinKiskac);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  pinMode(pinBuzzer, OUTPUT);

  // OLED Başlatma
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;)
      ;
  }
  display.clearDisplay();

  lastStatus = "BASLATILIYOR...";
  updateOLED();

  attachServos();

  // Başlangıç Pozisyonu
  safeServoWrite(motorTaban, suankiTaban, 180);
  safeServoWrite(motorOmuz, suankiOmuz, 180);
  safeServoWrite(motorDirsek, suankiDirsek, 180);
  safeServoWrite(motorKiskac, suankiKiskac, 180);

  // Açılış Sesi
  tone(pinBuzzer, 1500, 100);
  delay(150);
  tone(pinBuzzer, 2000, 200);

  delay(1500);

  // Titrememeleri için başlangıçta hemen kapat
  safeServoDetach(motorTaban, pinTaban);
  safeServoDetach(motorOmuz, pinOmuz);
  safeServoDetach(motorDirsek, pinDirsek);
  safeServoDetach(motorKiskac, pinKiskac);

  lastStatus = "HAZIR - BOSTA";
  updateOLED();
}

void parseMessage(String msg) {
  // Manuel Kontrol Komutu
  if (msg.startsWith("K,")) {
    int firstComma = msg.indexOf(',');
    int secondComma = msg.indexOf(',', firstComma + 1);
    int thirdComma = msg.indexOf(',', secondComma + 1);
    int fourthComma = msg.indexOf(',', thirdComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1 ||
        fourthComma == -1) {
      Serial.println("Error: Malformed command!");
      return;
    }

    int t = msg.substring(firstComma + 1, secondComma).toInt();
    int o = msg.substring(secondComma + 1, thirdComma).toInt();
    int d = msg.substring(thirdComma + 1, fourthComma).toInt();
    int k = msg.substring(fourthComma + 1).toInt();

    // Güvenlik sınırları kontrolü
    if (t >= 0 && t <= 180 && o >= 15 && o <= 180 && d >= 80 && d <= 180 &&
        k >= 0 && k <= 180) {
      hedefTaban = t;
      hedefOmuz = o;
      hedefDirsek = d;
      hedefKiskac = k;

      lastStatus = "HAREKETLI";
      isMoving = true;
      Serial.println("Status: Command accepted");
    } else {
      Serial.println("Error: Out of bounds!");
    }
  }
  // Buzzer Komutu
  else if (msg.startsWith("B,")) {
    int firstComma = msg.indexOf(',');
    int secondComma = msg.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
      int freq = msg.substring(firstComma + 1, secondComma).toInt();
      int dur = msg.substring(secondComma + 1).toInt();
      tone(pinBuzzer, freq, dur);
      Serial.println("Status: Buzzer tone played");
    } else {
      tone(pinBuzzer, 1000, 100);
      Serial.println("Status: Default beep played");
    }
  }
}

void loop() {
  static String inputString = "";
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      inputString.trim();
      if (inputString.length() > 0) {
        parseMessage(inputString);
      }
      inputString = "";
    } else {
      inputString += inChar;
    }
  }

  // Yumuşak Hareket Mantığı
  if (millis() - sonHareketZamani > hareketGecikmesi) {
    sonHareketZamani = millis();
    bool hareketVar = false;
    bool oledGuncelle = false;

    // --- TABAN MOTORU ---
    if (suankiTaban != hedefTaban) {
      if (!motorTaban.attached())
        motorTaban.attach(pinTaban);
      int diff = hedefTaban - suankiTaban;
      int step = diff / 4;
      if (step == 0)
        step = (diff > 0) ? 1 : -1;
      suankiTaban += step;
      safeServoWrite(motorTaban, suankiTaban, 180);
      hareketVar = true;
      sonHareketZamaniTaban = millis();
      oledGuncelle = true;
    } else {
      if (millis() - sonHareketZamaniTaban > 300) {
        safeServoDetach(motorTaban, pinTaban);
      }
    }

    // --- OMUZ MOTORU ---
    if (suankiOmuz != hedefOmuz) {
      if (!motorOmuz.attached())
        motorOmuz.attach(pinOmuz);
      int diff = hedefOmuz - suankiOmuz;
      int step = diff / 4;
      if (step == 0)
        step = (diff > 0) ? 1 : -1;
      suankiOmuz += step;
      safeServoWrite(motorOmuz, suankiOmuz, 180);
      hareketVar = true;
      sonHareketZamaniOmuz = millis();
      oledGuncelle = true;
    } else {
      if (millis() - sonHareketZamaniOmuz > 300) {
        safeServoDetach(motorOmuz, pinOmuz);
      }
    }

    // --- DİRSEK MOTORU ---
    if (suankiDirsek != hedefDirsek) {
      if (!motorDirsek.attached())
        motorDirsek.attach(pinDirsek);
      int diff = hedefDirsek - suankiDirsek;
      int step = diff / 4;
      if (step == 0)
        step = (diff > 0) ? 1 : -1;
      suankiDirsek += step;
      safeServoWrite(motorDirsek, suankiDirsek, 180);
      hareketVar = true;
      sonHareketZamaniDirsek = millis();
      oledGuncelle = true;
    } else {
      if (millis() - sonHareketZamaniDirsek > 300) {
        safeServoDetach(motorDirsek, pinDirsek);
      }
    }

    // --- KISKAÇ MOTORU ---
    if (suankiKiskac != hedefKiskac) {
      if (!motorKiskac.attached())
        motorKiskac.attach(pinKiskac);
      int diff = hedefKiskac - suankiKiskac;
      int step = diff / 3;
      if (step == 0)
        step = (diff > 0) ? 1 : -1;
      suankiKiskac += step;
      safeServoWrite(motorKiskac, suankiKiskac, 180);
      hareketVar = true;
      sonHareketZamaniKiskac = millis();
      oledGuncelle = true;
    } else {
      if (millis() - sonHareketZamaniKiskac > 300) {
        safeServoDetach(motorKiskac, pinKiskac);
      }
    }

    if (hareketVar) {
      isMoving = true;
    } else {
      if (isMoving) {
        isMoving = false;
        lastStatus = "HAZIR - BOSTA";
        oledGuncelle = true;
      }
    }

    if (oledGuncelle) {
      updateOLED();
    }
  }
}
