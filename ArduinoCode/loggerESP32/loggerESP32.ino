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
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define DS1307_I2C_ADDRESS 0x68

String daysOfWeek[8] = {"","Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"};
int Wartezeit = 10000;

Adafruit_BME280 bme;


byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
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
void displayTime(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS1307time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  //Ausgeben des Zeitstempels auf dem seriellen Ausgang.
  Serial.print(daysOfWeek[dayOfWeek]);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print(".");
  Serial.print(month, DEC);
  Serial.print(".");
  Serial.print(year, DEC);
  Serial.print(" ");
  Serial.print(hour<10?"0":"");
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute<10?"0":"");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second<10?"0":"");
  Serial.print(second, DEC);
  Serial.println(" ");
}

void setup(){
    Wire.begin();
    Serial.begin(9600);

    setDS1307(00,58,12,3,30,5,23);

    	if (!bme.begin(0x76)) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
		while (1);
	  }else Serial.println("BME280 ist angeschlossen");
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }else Serial.println("SD-Karte ist angeschlossen");
    
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

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
  char valu[20];
  
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS1307time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  sprintf(valu, "20%2i%02i%02i",year,month,dayOfMonth);
  appendFile(SD, "/log1.txt", valu);
  sprintf(valu,"%02i%02i%02i",hour,minute,second);
  appendFile(SD, "/log1.txt", valu);
  appendFile(SD, "/log1.txt", "\t");

  sprintf(valu, "%f", bme.readTemperature());
  appendFile(SD, "/log1.txt", valu);
  appendFile(SD, "/log1.txt", "°C ");
  sprintf(valu, "%f",bme.readPressure() / 100.0F);
  appendFile(SD, "/log1.txt", valu);
  appendFile(SD, "/log1.txt", "hPa ");
  sprintf(valu, "%f", bme.readAltitude(SEALEVELPRESSURE_HPA));
  appendFile(SD, "/log1.txt", valu);
  appendFile(SD, "/log1.txt", "m ");
  sprintf(valu, "%f", bme.readHumidity());
  appendFile(SD, "/log1.txt", valu);
  appendFile(SD, "/log1.txt", "%\n");
  
  delay (Wartezeit);
}