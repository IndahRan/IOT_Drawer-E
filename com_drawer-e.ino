#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <Stepper.h>

#define WIFI_SSID "SINAR RIAU"
#define WIFI_PASSWORD "jolysitisu"

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

#define BOT_TOKEN "6781353313:AAEaoE_Vfl9CtyfRtntwX0nJUWgl1gaCU9E"
const char* mqtt_server = "mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient clienty (espClient);
const int stepsPerRevolution = 500; 
Stepper myStepper(stepsPerRevolution, 18, 19, 21, 22);

const unsigned long BOT_MTBS = 1000;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

int getFingerprintIDez();
int tanda;
int relay = 15;
const int ledHijau = 27;
const int ledMerah = 26;
const int pin_reed = 14;  // 0=terpisah 1=terhubung(HIGH)
const int pin_getar = 12;
const int pin_buzzer = 13;
const int buttonPressPin = 25;

bool buttonPressed;
int buttonPressCount = 0;  // Menghitung jumlah tekanan tombol

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

// Array untuk menyimpan ID sidik jari yang valid
int validFingerIDs[] = {1, 2, 3, 4};
int numValidFingerIDs = sizeof(validFingerIDs) / sizeof(validFingerIDs[0]);

void callback (String topic, byte* message, unsigned int length) {
  Serial.print("Message Arrived on topic : ");
  Serial.print(topic);
  Serial.print(". Message");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
}

void reconnect() {
  while (!clienty.connected()) {
    Serial.println("Attempting MQTT connection...");

    if (clienty.connect("espClient")) {
      Serial.println("Connected");
      clienty.subscribe("code");
    } else {
      Serial.println("failed");
    }
  }
}

void handleNewMessages(int numNewMessages)
{

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chat_id) {
      bot.sendMessage(chat_id, "Unauthorized User", "");
      continue;
    }
    int nilai_reed = digitalRead(pin_reed);
    long measurement = vibration();
    String text = bot.messages[i].text;
    // Serial.println(text);
    String from_name = bot.messages[i].from_name;

      if (clienty.subscribe("code")) {
        Serial.println(text);
        getFingerprintIDez();
        delay(500);
        if (tanda == 0)
        {
          // mengatur kondisi relay saat ID sidik jari dikenali
          bool isFingerValid = false;

          for (int i = 0; i < numValidFingerIDs; i++)
          {
            if (finger.fingerID == validFingerIDs[i] && finger.confidence >= 50)
            {
              isFingerValid = true;
              break;
            }
          }
          if (isFingerValid)
          {
            digitalWrite(relay, HIGH);
            digitalWrite(ledHijau, HIGH);
            digitalWrite(ledMerah, LOW);
            int waktu1 = millis();
            tanda = 0;
            // delay(50);
            finger.fingerID = 0;
            if (digitalRead(relay) == HIGH)
            {
              delay(2000);
              digitalWrite(relay, LOW);
            }
            myStepper.step(stepsPerRevolution);

            Serial.println("Kunci laci terbuka dengan sidik jari");
            bot.sendMessage(chat_id, "Sidik Jari Terdeteksi", "");
            bot.sendMessage(chat_id, "Laci Terbuka", "");
          } else {
            digitalWrite(relay, LOW);
            digitalWrite(ledHijau, LOW);
            digitalWrite(ledMerah, HIGH);
            bot.sendMessage(chat_id, "Sidik Jari Tidak Terdeteksi", "");
          }
        }
      }

    if (nilai_reed == HIGH && measurement > 500) {
      bot.sendMessage(chat_id, "Ada Maling!", "");
    }
    else {
      bot.sendMessage(chat_id, "Aman!!!", "");
    }
  }

  buttonPressed = digitalRead(buttonPressPin);

    if (buttonPressed == false) {
      delay(50);  // Tunggu sebentar untuk menghindari bouncing pada tombol

      // Hitung jumlah tekanan tombol
      buttonPressCount++;

      // Sesuaikan sesuai kebutuhan
      if (buttonPressCount == 1) {
        myStepper.step(stepsPerRevolution);
        delay(500);
      } else if (buttonPressCount > 1) {
        // Kembali ke awal jika melebihi dua tekanan
        buttonPressCount = 0;
      }
      buttonPressed = true;  // Reset status penekanan tombol
    }
  
}

long vibration() {
  long measurement = pulseIn(pin_getar, HIGH);
  return measurement;
}

void setup()
{
  myStepper.setSpeed(50);

  Serial.begin(9600);
  Serial.println("Tes sidik jari");
  pinMode(relay, OUTPUT);
  pinMode(ledHijau, OUTPUT);
  pinMode(ledMerah, OUTPUT);
  pinMode(pin_reed, INPUT_PULLUP);
  pinMode(pin_getar, INPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(buttonPressPin, INPUT_PULLUP);
  buttonPressed = true;

  digitalWrite(relay, LOW);
  digitalWrite(ledHijau, LOW);
  digitalWrite(ledMerah, HIGH);
  digitalWrite(pin_reed, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println("Terhubung ya");

  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  // mengatur kecepatan data untuk port serial sensor
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Sensor sidik jari terdeteksi!");
  }
  else
  {
    Serial.println("Sensor sidik jari tidak terdeteksi :(");
    while (1)
      ;
  }
  Serial.println("Menunggu sidik jari yang valid...");

   myStepper.setSpeed(50);

  clienty.setServer(mqtt_server, 1883);
  clienty.setCallback(callback);
}

void loop()
{
  if (!clienty.connected()) {
    reconnect();
  }
  clienty.loop();

  if (millis() > bot_lasttime + BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
    int nilai_reed = digitalRead(pin_reed);
    long measurement = vibration();
    Serial.print("getar:");
    Serial.println(measurement);
    if (nilai_reed == HIGH && measurement > 500) {
      digitalWrite(pin_buzzer, HIGH);
      Serial.println("Ada Maling!");
    }
    else {
      digitalWrite(pin_buzzer, LOW);
      Serial.println("Aman!");
    }
}

int getFingerprintIDez()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Mengambil gambar");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("Tidak ada sidik jari terdeteksi");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Kesalahan komunikasi");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Kesalahan pengambilan gambar");
      return p;
    default:
      Serial.println("Kesalahan tidak diketahui");
      return p;
  }

  // OK, berhasil

  p = finger.image2Tz();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Gambar diubah");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Gambar terlalu buram");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Kesalahan komunikasi");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Tidak dapat menemukan fitur sidik jari");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Tidak dapat menemukan fitur sidik jari");
      return p;
    default:
      Serial.println("Kesalahan tidak diketahui");
      return p;
  }

  // OK, tersimpan
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Ditemukan kesesuaian sidik jari!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Kesalahan komunikasi");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    Serial.println("Tidak menemukan kesesuaian");
    return p;
  }
  else
  {
    Serial.println("Kesalahan tidak diketahui");
    return p;
  }

  // ditemukan kesesuaian!
  Serial.print("Ditemukan ID #");
  Serial.print(finger.fingerID);
  Serial.print(" dengan tingkat kepercayaan ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}