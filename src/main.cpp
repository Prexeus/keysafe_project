// Project Description...

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// included libraries:
#include <Arduino.h>
#include <Keypad.h>             // Library for Keypad
#include <LiquidCrystal_I2C.h>  // Library for I2C Display
#include <MFRC522.h>            // Library for RFID Reader
#include <SPI.h>                // Library for SPI Communication (for RFID Reader)
#include <Wire.h>               // opt
#include <stdio.h>              // opt
#include <stream.h>             // opt
#include <string.h>             // opt

//#include "State.cpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pin-declarations & related variables:

// LCD Display
const byte ledLine = 20;                                   // Anzahl der im LCD Display vorhandenen Zeichen pro Zeile (verwendet wird ein 20x4 Zeichen I2C Display)
LiquidCrystal_I2C lcd(0x27, 20, 4);                        // Einstellen der LCD Adresse auf 0x27, sowie für 20 Zeichen und 4 Zeilen
const int ledScrollSpeed[4] = {300, 300, 300, 300};        // Definieren der Scrollgeschwindigkeit für die 4 LED Zeilen. es sind individuelle Geschwinidkeit je Zeile möglich
const boolean ledScrollDir[4] = {true, true, true, true};  // Definieren der Scrollrichtung für ein 4 Zeilen Display (true bedeutet von rechts nach links))
char textRow[4][178];                                      // Definition der Textvariablen des LCD Displays - 2-dimensionales Array für 4 Zeilen mit je 178 Zeichen --> //Da das LCD Display 20 Zeilen hat müssen im String immer mindestens 20 Zeichen angegeben sein --> Ggf. mit Leerzeichen auf 20 Zeichen auffüllen! Sonst kann es zu Artefakten auf dem Display vom vorherigen Text kommen!
char lastTextRow[4][178];                                  //{{0},{0},{0},{0}};    letzten Zustand der Textzeile merken um Veränderungen zu erkennen!

// stuff
#define alarmSiren 2      // Pin-Definition für die Alarmsirene
#define doorLock 7        // Pin-Definition für das Türschloss
#define doorLockSensor 8  // Pin-Deginition für den Türschloss-Sensor

// RFID-Reader
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfidReader(SS_PIN, RST_PIN);

// Status-LED
#define statusLedRed 9     // Pin für den roten Kanal
#define statusLedGreen 10  // Pin für den grünen Kanal
#define statusLedBlue 11   // Pin für den blauen Kanal

typedef enum LedColor {
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA,
    WHITE,
    OFF
};
LedColor statusLedColor = OFF;

// Keypad
const byte rowAmount = 4;     // vier Reihen
const byte columnAmount = 3;  // drei Spalten

const char keys[rowAmount][columnAmount] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte pinRows[rowAmount] = {9, 8, 7, 6};    // verbinde die Reihen mit diesen Pins
byte pinColumn[columnAmount] = {5, 4, 3};  // verbinde die Spalten mit diesen Pins

Keypad keypad = Keypad(makeKeymap(keys), pinRows, pinColumn, rowAmount, columnAmount);

char firstKey = '\0';
unsigned long lastKeyPressTime = 0;
const unsigned long resetTimeout = 5000;  // Zeitlimit für die Eingabe in Millisekunden (hier 5 Sekunden)
int keyNumberVar = 0;                     // Globale Variable für die Zahlenkombination des Keypads

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global variables:
State state = STARTING;

unsigned long currentStateEnteredTime;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state deklarations:
typedef enum State {
    STARTING,
    INACTIVE,

    READY,

    LOGGED_IN,

    LOGGED_IN_KEY_RETURN,

    GUEST_KEY_RETURN,
    GUEST_WAITING,

    WRONG_KEY_EXCHANGE,
    WAITING_FOR_WRONG_KEY,

    LOGGED_IN_KEY_SEARCH,
    GUEST_KEY_SEARCH

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup:
void setup() {
    Serial.begin(9600);

    // LCD Display initialisation
    lcd.init();
    lcd.backlight();
    // Willkommenstext anzeigen
    strcpy(textRow[0], "   IFU Stuttgart    ");
    strcpy(textRow[1], "  Schlüsselausgabe  ");
    strcpy(textRow[2], "  2024 Version 1.0  ");
    strcpy(textRow[3], " ->Tür schließen!<- ");
    // Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays
    taskText(textRow[0], 0);
    taskText(textRow[1], 1);
    taskText(textRow[2], 2);
    taskText(textRow[3], 3);

    // RFID Reader initialisation
    SPI.begin();
    rfidReader.PCD_Init();

    // OUTPUT-Pins
    pinMode(statusLedRed, OUTPUT);
    pinMode(statusLedGreen, OUTPUT);
    pinMode(statusLedBlue, OUTPUT);
    pinMode(doorLock, OUTPUT);
    pinMode(alarmSiren, OUTPUT);

    // INPUT-Pins
    pinMode(doorLockSensor, INPUT);

    changeStateTo(INACTIVE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state-machine-loop:
void loop() {
    switch (state) {
        case INACTIVE:
            inactive();
            break;
        case READY:
            ready();
            break;
        case LOGGED_IN:
            loggedIn();
            break;
        case LOGGED_IN_KEY_RETURN:
            loggedInKeyReturn();
            break;
        case GUEST_KEY_RETURN:
            guestKeyReturn();
            break;
        case GUEST_WAITING:
            guestWaiting();
            break;
        case WRONG_KEY_EXCHANGE:
            wrongKeyExchange();
            break;
        case WAITING_FOR_WRONG_KEY:
            waitingForWrongKey();
            break;
        case LOGGED_IN_KEY_SEARCH:
            loggedInKeySearch();
            break;
        case GUEST_KEY_SEARCH:
            guestKeySearch();
            break;
        default:
            break;
    }

    // Überprüfe ob die Schranktür geschlossen ist
    checkDoorLockSensor();

    // Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays am Ende der Void Loop Funktion!
    taskText(textRow[0], 0);
    taskText(textRow[1], 1);
    taskText(textRow[2], 2);
    taskText(textRow[3], 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state-change-function:
void changeStateTo(State newState) {
    currentStateEnteredTime = millis();
    switch (newState) {
        case INACTIVE:
            initiateInactive();
            break;
        case READY:
            initiateReady();
            break;
        case LOGGED_IN:
            initiateLoggedIn();
            break;
        case LOGGED_IN_KEY_RETURN:
            initiateLoggedInKeyReturn();
            break;
        case GUEST_KEY_RETURN:
            initiateGuestKeyReturn();
            break;
        case GUEST_WAITING:
            initiateGuestWaiting();
            break;
        case WRONG_KEY_EXCHANGE:
            initiateWrongKeyExchange();
            break;
        case WAITING_FOR_WRONG_KEY:
            initiateWaitingForWrongKey();
            break;
        case LOGGED_IN_KEY_SEARCH:
            initiateLoggedInKeySearch();
            break;
        case GUEST_KEY_SEARCH:
            initiateGuestKeySearch();
            break;
        default:
            break;  
    }
    state = newState;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state-functions:
void initiateInactive() {
}
void inactive() {
    // state repetition:
    blinkStatusLed(RED);

    // state changeconditions:
    if (isDoorLocked()) {
        state = READY;                  //Soll das So?
        changeStateTo(READY);
    }
}

void initiateReady() {
    setStatusLed(RED);
    closeDoorLock();  // Türschloss schließen

    strcpy(textRow[0], "Scannen Sie Ihren   ");
    strcpy(textRow[1], "RFID-Chip oder      ");
    strcpy(textRow[2], "einen               ");
    strcpy(textRow[3], "Schlüsselanhänger.  ");
}
void ready() {
    // state repetition:

    // state changeconditions:
    keyNumberVar = keypadReadout();  // Auslesen des Keypads

    if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            //
        } else if (isRfidEmployee(rfidId)) {
            //
        } else {
            // print a warning for a wrong RFID device
        }
    } else if (keyNumberVar != 0) {  // Wenn eine Zahlenkombination eingegeben wurde wechsle in guestKeySearch
        changeStateTo(GUEST_KEY_SEARCH);
    }
}

void initiateLoggedIn() {
    setStatusLed(GREEN);
    openDoorLock();

    strcpy(textRow[0], "Schlüssel entnehmen ");
    strcpy(textRow[1], "oder zur Rückgabe   ");
    strcpy(textRow[2], "Schlüsselband       ");
    strcpy(textRow[3], "einscannen.         ");
}
void loggedIn() {
    // state repetition:

    // state changeconditions:
}

void initiateLoggedInKeyReturn() {
    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schlüssel zurück.   ");
    strcpy(textRow[3], "                    ");
}
void loggedInKeyReturn() {
    // state repetition:
    blinkStatusLed(GREEN);

    // state changeconditions:
}

void initiateGuestKeyReturn() {
    openDoorLock();

    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schlüssel zurück.   ");
    strcpy(textRow[3], "                    ");
}
void guestKeyReturn() {
    // state repetition:
    blinkStatusLed(BLUE);

    // state changeconditions:
    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schlüssel zurück.   ");
    strcpy(textRow[3], "                    ");
}

void initiateGuestWaiting() {
    setStatusLed(BLUE);

    strcpy(textRow[0], "Vorgang             ");
    strcpy(textRow[1], "abgeschlossen. Neuen");
    strcpy(textRow[2], "Schlüssel einscannen");
    strcpy(textRow[3], "oder Türe schließen.");
}
void guestWaiting() {
    // state repetition:

    // state changeconditions:
    if (isDoorLocked()) {
        changeStateTo(READY);
    }
}

void initiateWrongKeyExchange() {
    setStatusLed(RED);
    openDoorLock();

    strcpy(textRow[0], "Tauschen Sie den    ");
    strcpy(textRow[1], "Schlüssel der roten");
    strcpy(textRow[2], "LED mir dem einge-  ");
    strcpy(textRow[3], "scannten Schlüssel.");
}
void wrongKeyExchange() {
    // state repetition:
    // state changeconditions:
}

void initiateWaitingForWrongKey() {
    setStatusLed(YELLOW);
    openDoorLock();

    strcpy(textRow[0], "Scanne den eben     ");  // TODO: Text anpassen
    strcpy(textRow[1], "entnommenen Schlüs-");
    strcpy(textRow[2], "sel und stecke ihn  ");
    strcpy(textRow[3], "zurück.            ");
}
void waitingForWrongKey() {
    // state repetition:
    blinkStatusLed(YELLOW);
    // state changeconditions:
}

void initiateLoggedInKeySearch() {
    setStatusLed(GREEN);
}
void loggedInKeySearch() {
    // state repetition:
    // state changeconditions:
}

void initiateGuestKeySearch() {
    setStatusLed(RED);
}
void guestKeySearch() {
    // state repetition:
    // state changeconditions:
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions:

boolean isDoorLocked() {
    return digitalRead(doorLockSensor) == HIGH;
}

void setStatusLed(LedColor color) {
    switch (color) {
        case RED:
            setStatusLed(255, 0, 0);
            break;
        case GREEN:
            setStatusLed(0, 255, 0);
            break;
        case BLUE:
            setStatusLed(0, 0, 255);
            break;
        case YELLOW:
            setStatusLed(255, 255, 0);
            break;
        case CYAN:
            setStatusLed(0, 255, 255);
            break;
        case MAGENTA:
            setStatusLed(255, 0, 255);
            break;
        case WHITE:
            setStatusLed(255, 255, 255);
            break;
        case OFF:
            setStatusLed(0, 0, 0);
            break;
        default:
            setStatusLed(0, 0, 0);
            break;
    }
}

void setStatusLed(int red, int green, int blue) {
    digitalWrite(statusLedRed, red);
    digitalWrite(statusLedGreen, green);
    digitalWrite(statusLedBlue, blue);
}

void blinkStatusLed(LedColor color) {
    int blinkDuration = 500;  // in ms
    if (millis() % (blinkDuration * 2) < blinkDuration) {
        setStatusLed(color);
    } else {
        setStatusLed(OFF);
    }
}

// Funktion zum öffnen des Türschlosses
void openDoorLock() {
    digitalWrite(doorLock, HIGH);  // Türschloss auf
}

// Funktion zum schließen des Türschlosses
void closeDoorLock() {
    digitalWrite(doorLock, LOW);  // Türschloss zu
}

// Funktion zum überprüfen ob Tür geschlossen ist
void checkDoorLockSensor() {
    if (digitalRead(doorLockSensor) == HIGH && state != WRONG_KEY_EXCHANGE) {  // Wenn der Türschloss-Sensor HIGH ist und der Zustand nicht "wrongKeyExchange" ist, da dieser Vorgang auf jeden Fall abgeschlossen werden muss bevor in "ready" gewechselt werden kann
        delay(3000);                                                           // Warte 3 Sekunden bis das Türschloss sich schließt
        closeDoorLock();                                                       // Türschloss schließen
        state = READY;
    }
}

// Funktion für die Scrollfunktion, um den Text in der Variable zu schieben!
char charAt(char *text, int pos)
// scrolling-logic coded here
{
    if (pos < ledLine)
        return ' ';  // scroll in
    else if (pos >= ledLine && pos < ledLine + strlen(text))
        return text[pos - ledLine];  // scroll text
    else
        return ' ';  // scroll out
}

// Zeitlicher Ablauf wann gescrollt werden muss und ob die Textlänge ein Scrollen erfordert prüfen und ggf. Scrollen durchführen
void taskText(char *text, byte line)
// scroll the LED lines
{
    char currentText[ledLine + 1];
    static unsigned long nextScroll[4];
    static int positionCounter[4];
    int i;

    // Textinhalte des 2dimensionalen Arrays vergleichen. hats sich inhaltlich ewats verändert --> Position dieser Zeile auf Null setzen
    if (strcmp(text, lastTextRow[line]) != 0) {
        positionCounter[line] = 0;
        // Hier muss der aktuelle Text in die LastTextzeile[][] Variable kopiert werden!
        strcpy(lastTextRow[line], text);
    }
    // Beim Wechsel der Textlänge kann der positionCounter des Textes beim Scrollen zu hoch sein und sich nicht mehr zurückstellen --> Dann würde der Text nicht angezeigt werden! --> Hier muss die Position  zurückgesetzt werden!
    if (ledScrollDir[line] == true && positionCounter[line] > (strlen(text) + ledLine)) {  // für nach links scrollen
        positionCounter[line] = 0;
    } else if (positionCounter[line] < 0) {  // für nach rechts scrollen
        positionCounter[line] = strlen(text) + ledLine - 1;
    }

    // Prüfen ob Zeitpunkt zum Scrollen jetzt vorhanden und ob die Textlänge scrollen nötig macht!
    if (millis() > nextScroll[line] && strlen(text) > ledLine) {
        nextScroll[line] = millis() + ledScrollSpeed[line];

        for (i = 0; i < ledLine; i++)
            currentText[i] = charAt(text, positionCounter[line] + i);
        currentText[ledLine] = 0;

        lcd.setCursor(0, line);
        lcd.print(currentText);

        if (ledScrollDir[line]) {
            positionCounter[line]++;
            if (positionCounter[line] == strlen(text) + ledLine) positionCounter[line] = 0;
        } else {
            positionCounter[line]--;
            if (positionCounter[line] < 0) positionCounter[line] = strlen(text) + ledLine - 1;
        }
    } else if (millis() > nextScroll[line] && strlen(text) <= ledLine)  // geändert von if -> else if
    {
        nextScroll[line] = millis() + ledScrollSpeed[line];
        lcd.setCursor(0, line);
        lcd.print(text);
    }
}

// Funktion zum auslesen des Keypads
char keypadReadout() {
    int keyNumber = 0;
    char key = keypad.getKey();
    unsigned long currentTime = millis();

    if (key) {  // Überprüfe, ob eine Taste gedrückt wurde

        if (isDigit(key)) {  // Überprüfe, ob die gedrückte Taste eine Zahl ist

            if (firstKey == '\0') {  // Überprüfe, ob die erste Ziffer bereits gedrückt wurde
                // Speichere die erste gedrückte Zahl
                firstKey = key;
                lastKeyPressTime = currentTime;  // Setze die Zeit für die erste Ziffer

                // Text für LCD Display
                strcpy(textRow[0], "Suche Schluessel:");
                sprintf(textRow[1], "%c", firstKey);
                strcpy(textRow[2], "");
                strcpy(textRow[3], "");

            } else {  // Zwei Zahlen nacheinander wurden gedrückt

                keyNumber = (firstKey - '0') * 10 + (key - '0');

                // Text für LCD Display
                strcpy(textRow[0], "Suche Schluessel:");
                sprintf(textRow[1], "%d", keyNumber);
                strcpy(textRow[2], "");
                strcpy(textRow[3], "");

                firstKey = '\0';  // Setze firstKey zurück, um auf die nächste Zahlenkombination zu warten
            }
        }
    }

    // Überprüfe, ob die Zeitgrenze überschritten wurde. Wenn ja, setze die erste Ziffer zurück
    if (firstKey != '\0' && currentTime - lastKeyPressTime >= resetTimeout) {
        firstKey = '\0';  // Setze die erste Ziffer zurück
        changeStateTo(state); // Initialisiere den aktuellen State neu um das Display auf den Ausgangszustand zurückzusetzen
    }

    return keyNumber;  // Gib die Zahlenkombination zurück
}

// RFID Funktionen
boolean isRfidPresented() {
    return (rfidReader.PICC_IsNewCardPresent() && rfidReader.PICC_ReadCardSerial());
}

boolean isRfidKey(long rfidId) {
    return rfidId == 123456789;
}

boolean isRfidEmployee(long rfidId) {
    return rfidId == 987654321;
}

long getRfidId() {
    long code = 0;
    for (byte i = 0; i < rfidReader.uid.size; i++) {
        code = ((code + rfidReader.uid.uidByte[i]) * 10);
    }
    return code;
}
