#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ThingSpeak.h>
#include <UniversalTelegramBot.h>
#define LED D7
#define lock D6

#define WIFI_SSID "MULIA"
#define WIFI_PASSWORD "---"

//Konfigurasi Thingspeak
unsigned long myChannelNumber = 2262268;
const char * myReadAPIKey = "BNSOQBZ9CYNCVJIQ";

long count = 0;     //untuk status aktifasi keamanan 
String number;      //untuk nomor telepon darurat yang dikirimkan ke Telegram             
#define BOTtoken "6137052924:AAEpOSmAKijReOycHp5EdsFRLyhPz510" //Isi xxxx dengan token Bot Telegram
String chatid = "-------";                                     //Isi xxxx dengan ID telegram

X509List cert(TELEGRAM_CERTIFICATE_ROOT); 
WiFiClientSecure clientSec; //untuk Telegram
WiFiClient client;          //untuk Thingspeak
UniversalTelegramBot bot(BOTtoken, clientSec);

const int maxRetries = 10; // Maximum retry
int retryCount = 0;        // Global retry count

bool secureNotif = false;  //untuk mencegah pengiriman pesan berulang ulang

void setup() {
  Serial.begin(115200);
  delay(10);
  WifiStatus();       //Fungsi untuk menghubungkan koneksi dengan WiFi internet

  //membrikan waktu bagi setup untuk sinkronisasi antara waktu NTP dengan waktu device**
  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  //Pengecekab koneksi internet
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(chatid, "Connected with Telegram", "");
    Serial.println("Pesan Terkirim ke Telegram");
  } else {
    Serial.println("Pesan Belum Terkirim ke Telegram");
  }
  ThingSpeak.begin(client); // Initialize ThingSpeak

  pinMode (LED, OUTPUT);
  pinMode (lock, INPUT);
}

void loop() {
  int statusCode = 0;
  count = ThingSpeak.readLongField(myChannelNumber, 1, myReadAPIKey);
  statusCode = ThingSpeak.getLastReadStatus();

  if (statusCode == 200) {
    Serial.println("Counter: " + String(count));
    if (count == 0) {
      digitalWrite(LED, LOW);
    } else if (count == 1) {
      digitalWrite(LED, HIGH);
      if(digitalRead(lock) == 0) {
        if(!secureNotif) {
          String message = "Intruder enter the house!!\nCall "+ number + " for immediate assistance.";
          bot.sendMessage(chatid, message, "HTML");
          secureNotif = true;
        }
      } else {
        if(secureNotif){  
          bot.sendMessage(chatid, "The door has closed again.\nPlease check the safety and condition of the house.", "");
          secureNotif = false;
        }
      }
    }
  } else {
    Serial.println("Problem reading channel. HTTP error code " + String(statusCode));
  }


  number = String(ThingSpeak.readLongField(myChannelNumber, 2, myReadAPIKey)); 
  statusCode = ThingSpeak.getLastReadStatus();

  if (statusCode == 200) {
    Serial.println("Phone: " + number);
  } else {
    Serial.println("No update number...");
  }
  
  delay(500); 
}

void WifiStatus() {
  if (retryCount >= maxRetries) {
    Serial.println("Max retry attempts reached. WiFi connection failed.");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Connecting Wifi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  clientSec.setTrustAnchors(&cert);
 
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(1000);
    Serial.print(".");
    retries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Retrying...");
    retryCount++; 
    WifiStatus();
  }
}