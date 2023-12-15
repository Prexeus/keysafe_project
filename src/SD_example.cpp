#include <SD.h>


#define CS 10
#define CD 9

File dataFile;

void setup() {
    Serial.begin(9600);

    pinMode(CS, OUTPUT);
    pinMode(CD, INPUT);

    bool cardPresent = digitalRead(CD);

    if (!cardPresent) {
        Serial.print("No SD card found! Insert a card to proceed...");
        while (!cardPresent)
            cardPresent = digitalRead(CD);
        Serial.println("OK!");
    }

    if (!SD.begin(CS)) {
        Serial.println("Couldn't initialize SD card reader!");
        while (true)
            ;
    }

    /*
        Serial.print("Initializing SD card...");

        if (!SD.begin(4)) {
            Serial.println("initialization failed!");
            return;
        }
        Serial.println("initialization done.");

        dataFile = SD.open("keyDatabase.csv", FILE_WRITE);

        dataFile.read();

        SD.close("keyDatabase.csv");
    */
}

void loop() {
    writeLog();

    if (SD.exists("log.txt"))
        readLog();
    else
        Serial.println("Couldn't read log.txt! Make sure the file exists!");

    while (true)
        ;
}

void writeLog() {
    File logFile = SD.open("log.txt", FILE_WRITE);  // allows writing and reading
    if (logFile) {
        logFile.println("Device booted!");
        logFile.println("Hello World...");
        logFile.close();
    } else {
        Serial.println("Couldn't write log.txt!");
    }
}

void readLog() {
    File logFile = SD.open("log.txt");

    if (logFile) {
        Serial.println("Log contents:");

        while (logFile.available())
            Serial.write(logFile.read());

        logFile.close();

        Serial.println("-----------");
        Serial.println("");
    } else {
        Serial.println("Couldn't read log.txt!");
    }
}