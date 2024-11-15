
/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
//#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const int chipSelect = 15; //D8

const char* SSID = "neuralink_chip_32";
const char* PSK = "jtmHd)4>/7e6?ZxLC_+<KU";
const char* mqtt_server = "192.168.4.1";
const int mqtt_port = 1883;

const char* mqtt_user = "weather";
const char* mqtt_password = "12345";

#define SEALEVELPRESSURE_HPA (1013.25)
#define DS1307_I2C_ADDRESS 0x68

String daysOfWeek[8] = {"","Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"};
int wartezeit = 60000; // ms

Adafruit_BME280 bme;

WiFiClient espClient;
PubSubClient client(espClient);

char  csvtxt[60];
char  mqttmsg[50];


byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}



// void readFile(fs::FS &fs, const char * path){
//     Serial.printf("Reading file: %s\n", path);

//     File file = fs.open(path);
//     if(!file){
//         Serial.println("Failed to open file for reading");
//         return;
//     }

//     Serial.print("Read from file: ");
//     while(file.available()){
//         Serial.write(file.read());
//     }
//     file.close();
// }

// void writeFile(fs::FS &fs, const char * path, const char * message){
//     Serial.printf("Writing file: %s\n", path);

//     File file = fs.open(path, FILE_WRITE);
//     if(!file){
//         Serial.println("Failed to open file for writing");
//         return;
//     }
//     if(file.print(message)){
//         Serial.println("File written");
//     } else {
//         Serial.println("Write failed");
//     }
//     file.close();
// }


void appendFile(const char * path, const char * message){
    // Serial.printf("Appending to file: %s\n", path);

    File dataFile = SD.open(path, FILE_WRITE); //FILE_APPEND
    if(!dataFile){
        //Serial.println("Failed to open file for appending");
        ledOn();
        return;
    }
    if(dataFile.print(message)){
        //Serial.println("Message appended");
        ledOn();delay(200);ledOff();
    } else {
        //Serial.println("Append failed");
        ledOn();
    }
    dataFile.close();
}

void setup_wifi() {
    ledOn();
    WiFi.begin(SSID, PSK);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        ledOn();
        delay(2000);
        ledOff();
        Serial.print("Wifi not found\n");
    }
    ledOn();
    delay(1000);
    ledOff();
    delay(1000);
    ledOn();
    delay(1000);
    ledOff();
    delay(1000);
    ledOn();
    delay(1000);
    ledOff();
    delay(1000);
    ledOn();
    delay(1000);
    ledOff();
    delay(1000);
    //Serial.println(WiFi.localIP());
}


void setDS1307(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year){
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second)); // setzt die Sekunden
  Wire.write(decToBcd(minute)); // setzt die Minuten
  Wire.write(decToBcd(hour)); // setzt die Stunden
  Wire.write(decToBcd(dayOfWeek)); // setzt den Wert für den Tag der Woche
  Wire.write(decToBcd(dayOfMonth)); // setzt den Wert für den Tag im Monat
  Wire.write(decToBcd(month)); // setzt den Monat
  Wire.write(decToBcd(year)); // setzt das Jahr
  Wire.endTransmission();
}
void readDS1307time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year){
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  //Anfordern der ersten 7 Datenbyte  vom DS1307
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}



void ledOn() {
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
}

void ledOff() {
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void ledToggle() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Change the state of the LED
}


void setup(){
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
    ledOn();
        
    
    // WIFI
    setup_wifi();
    //MQTT
    client.setServer(mqtt_server, mqtt_port);

    //TIME
    /*/ Aktuelle Uhrzeit über den Zeitserver ermitteln
    configTime(2 * 3600, 0, "0.de.pool.ntp.org", "ptbtime1.ptb.de");   
    delay(1000); 
    time_t now = time(nullptr);
    String time = String(ctime(&now));
    time.trim();
    time.substring(0,19).toCharArray(time_value, 20); 
    */
    
    //start I2C
    Wire.begin();

    // SPI
    // just write date time once. Will be remembered by RTC
    //** setDS1307(00,33,15,5,02,06,23);

    if (!bme.begin(0x76)) {		  
		  while (1){
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        ledOff();delay(500);ledOn();
        delay(10000);
      }
	  } else ledOff();delay(200);ledOn();delay(200);ledOff();Serial.println("BME280 ist angeschlossen");
   
    if(!SD.begin(chipSelect)){
        Serial.println("Card Mount Failed");
        ledOff();delay(500);ledOn();
        delay(10000);
        return;
    } else ledOff(); Serial.println("SD-Karte ist angeschlossen");
    
    

    //uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    //Serial.printf("SD Card Size: %lluMB\n", cardSize);

    /*listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");
    appendFile(SD, "/test.txt", "I");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));*/
}

void loop(){
    
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS1307time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  float bme280_temp = bme.readTemperature();
  float bme280_press = bme.readPressure() / 100.0F;
  float bme280_humi = bme.readHumidity();
  sprintf(csvtxt, "20%2i%02i%02i %02i%02i%02i;\t %.2f °C;\t %.2f hPa;\t  %.2f %%;\n", year,month,dayOfMonth,hour,minute,second, bme280_temp, bme280_press, bme280_humi);
  appendFile("/log_bme280.csv", csvtxt);
  // sprintf(valu, "20%2i%02i%02i",year,month,dayOfMonth);
  // appendFile("/log1.txt", valu);
  // sprintf(valu,"%02i%02i%02i",hour,minute,second);
  // appendFile("/log1.txt", valu);
  // appendFile("/log1.txt", ";\t");

  // sprintf(valu, "%f °C;\t %f hPa  %f m;\t %f", bme.readTemperature());
  // appendFile("/log1.txt", valu);
  // appendFile("/log1.txt", " °C");
  // sprintf(valu, "%f hPa ",bme.readPressure() / 100.0F);
  // appendFile(SD, "/log1.txt", valu);
  // appendFile(SD, "/log1.txt", "hPa ");
  // sprintf(valu, "%f", bme.readAltitude(SEALEVELPRESSURE_HPA));
  // appendFile(SD, "/log1.txt", valu);
  // appendFile(SD, "/log1.txt", "m ");
  // sprintf(valu, "%f", bme.readHumidity());
  // appendFile(SD, "/log1.txt", valu);
  // appendFile(SD, "/log1.txt", "%\n");

  // send mqtt
  
  sprintf(mqttmsg, "{\"clock\": \"20%2i-%02i-%02iT%02u:%02u:%02uZ\", \"temp\": %.2f, \"press\": %.2f, \"humi\":  %.2f}",year,month,dayOfMonth,hour,minute,second, bme280_temp, bme280_press, bme280_humi);
  
  if (!client.connected()) {
        while (!client.connected()){
            Serial.print("Couldn't connect to MQTT Broker!\n");
            client.connect("ESP8266Client",mqtt_user, mqtt_password);
            delay(100);
            ledOn();
            delay(500);
            ledOff();
            delay(500);
        }
  }
  client.loop();
  client.publish("daten/topBME280",mqttmsg);
  
  delay (wartezeit);
  
}
