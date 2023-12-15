// Project Description...

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// included libraries:
#include <Arduino.h>
#include <Keypad.h>             // Library for Keypad
#include <LiquidCrystal_I2C.h>  // Library for I2C Display
#include <MFRC522.h>            // Library for RFID Reader
#include <SD.h>                 // Library for SD Card Reader
#include <SPI.h>                // Library for SPI Communication (for RFID Reader)

// #include "State.cpp"
#include "Database.h"

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

// SD-Card Reader
Database database;
#define sdOut 4
#define sdIn 5

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
    strcpy(textRow[0], "IFU Stuttgart");
    strcpy(textRow[1], "Schlüsselausgabe");
    strcpy(textRow[2], "2024 Version 1.0");
    strcpy(textRow[3], "Tuer schliessen!");
    // Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays
    taskText(textRow[0], 0);
    taskText(textRow[1], 1);
    taskText(textRow[2], 2);
    taskText(textRow[3], 3);

    // RFID Reader initialisation
    SPI.begin();
    rfidReader.PCD_Init();

    // SD-Card Reader initialisation
    pinMode(sdOut, OUTPUT);
    pinMode(sdIn, INPUT);
    if (!digitalRead(sdIn)) {
        Serial.print("No SD card found! Insert a card to proceed...");
        while (!digitalRead(sdIn)) {
        }
        Serial.println("OK!");
    }
    if (!SD.begin(sdOut)) {
        Serial.println("Couldn't initialize SD card reader!");
        while (true){
        }
    }
    database = Database(SD);

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
    // TODO: Alle SchlüsselLEDs ausschalten
    openDoorLock();  // Türschloss zu Beginn öffnen falls bei Start Tür nicht zu.

    strcpy(textRow[0], "Bitte Türe");
    strcpy(textRow[1], "schliessen!");
    strcpy(textRow[2], "");
    strcpy(textRow[3], "");
}
void inactive() {
    // state repetition:
    blinkStatusLed(RED);

    // state changeconditions:
    if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateReady() {
    setStatusLed(RED);
    closeDoorLock();
    // TODO: Alle SchlüsselLEDs ausschalten
    // TODO: Alle Schlüsselbolzen schließen

    strcpy(textRow[0], "Scannen Sie Ihren");
    strcpy(textRow[1], "RFID-Chip oder");
    strcpy(textRow[2], "einen");
    strcpy(textRow[3], "Schlüsselanhänger.");
}
void ready() {
    // state repetition:

    // state changeconditions:
    keyNumberVar = keypadReadout();  // Auslesen des Keypads

    if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            changeStateTo(GUEST_KEY_RETURN);  // Wenn ein Schlüssel gescannt wird, wechsle in den Zustand "guestKeyReturn"
        } else if (isRfidEmployee(rfidId)) {
            changeStateTo(LOGGED_IN);  // Wenn ein RFID-Chip gescant wird, wechsle in den Zustand "loggedIn"
        } else {
            // print a warning for a wrong RFID device
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(GUEST_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in guestKeySearch
    }
}

void initiateLoggedIn() {
    setStatusLed(GREEN);
    openDoorLock();

    strcpy(textRow[0], "Schluessel entnehmen");
    strcpy(textRow[1], "oder zur Rueckgabe");
    strcpy(textRow[2], "Schluesselband");
    strcpy(textRow[3], "einscannen.");
}
void loggedIn() {
    // state repetition:

    // TODO: SchlüsselLEDs der Schlüssel aktualisieren nach Berechtigung oder fehlen des Schlüssels
    // TODO: Schlüsselbolzen der Schlüssel aktualisieren nach Berechtigung oder fehlen des Schlüssels

    // state changeconditions:
    keyNumberVar = keypadReadout();  // Auslesen des Keypads

    if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            changeStateTo(LOGGED_IN_KEY_RETURN);  // Wenn ein Schlüssel gescannt wird, wechsle in den Zustand "loggedInKeyReturn"
        } else {
            // TODO?: print a warning for a wrong RFID device
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(LOGGED_IN_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in "loggedInKeySearch"
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateLoggedInKeyReturn() {
    // TODO: SchlüsselLED des Schlüssels der zurückgegeben werden soll einschalten alle anderen ausschalten
    // TODO: Schlüsselbolzen des Schlüssels der zurückgegeben werden soll öffne alle anderen schließen
    openDoorLock();

    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schlüssel zurück.   ");
    strcpy(textRow[3], "                    ");
}
void loggedInKeyReturn() {
    // state repetition:
    blinkStatusLed(GREEN);

    // state changeconditions:

    // TODO: If abfrage ob der sensor des Schlüssels der zurückgegeben wurde HIGH ist, wenn ja changeStateTo(loggedIn) und SD aktualisieren
    if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestKeyReturn() {
    openDoorLock();

    // TODO: SchlüsselLED des Schlüssels der zurückgegeben werden soll einschalten alle anderen ausschalten
    // TODO: Schlüsselbolzen des Schlüssels der zurückgegeben werden soll öffne alle anderen schließen

    strcpy(textRow[0], "Stecken Sie");
    strcpy(textRow[1], "den gescannten");
    strcpy(textRow[2], "Schlüssel zurück.");
    strcpy(textRow[3], "");
}
void guestKeyReturn() {
    // state repetition:
    blinkStatusLed(BLUE);

    // state changeconditions:

    // TODO: If abfrage ob der sensor des Schlüssels der zurückgegeben wurde HIGH ist, wenn ja changeStateTo(guestWaiting) und SD aktualisieren
    if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestWaiting() {
    setStatusLed(BLUE);

    // TODO: Alle SchlüsselLEDs ausschalten
    // TODO: Alle Schlüsselbolzen schließen

    strcpy(textRow[0], "Vorgang");
    strcpy(textRow[1], "abgeschlossen. Neuen");
    strcpy(textRow[2], "Schluessel scannen");
    strcpy(textRow[3], "o. Tuere schliessen.");
}
void guestWaiting() {
    // state repetition:

    // state changeconditions:
    if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            changeStateTo(GUEST_KEY_RETURN);  // Wenn ein Schlüssel gescannt wird, wechsle in den Zustand "guestKeyReturn"
        } else {
            // TODO?: print a warning for a wrong RFID device
        }
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateWrongKeyExchange() {
    openDoorLock();

    // TODO: SchlüsselLED des falschen Schlüssels einschalten alle anderen ausschalten
    // TODO: Schlüsselbolzen des falschen Schlüssels öffne alle anderen schließen

    strcpy(textRow[0], "Tauschen Sie den");
    strcpy(textRow[1], "Schluessel der roten");
    strcpy(textRow[2], "LED mir dem einge-");
    strcpy(textRow[3], "scannten Schluessel.");
}
void wrongKeyExchange() {
    // state repetition:
    blinkStatusLed(RED);

    // state changeconditions:

    // TODO: IF Abfrage ob der Sonsor des falschen Schlüssels LOW ist und dann nächste Zeile überprüfen
    // TODO: If Abfrage ob der Sensor des falschen Schlüssels HIGH ist, wenn ja changeStateTo(waitingForWrongKey) und SD aktualisieren
}

void initiateWaitingForWrongKey() {
    strcpy(textRow[0], "Scanne den eben");
    strcpy(textRow[1], "entnommenen");
    strcpy(textRow[2], "Schluessel und gebe");
    strcpy(textRow[3], "ihn zurück.");
}
void waitingForWrongKey() {  // FRAGE: Zustand kann umgangen werden, in dem der Schrank nach Beendigung des Zustands: "wrongKeyExchange" geschlossen wird. Ist das so gewollt?

    // state repetition:
    blinkStatusLed(YELLOW);

    // state changeconditions:

    if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            // TODO: else: SchlüsselLED des Schlüssels der zurückgegeben werden soll einschalten alle anderen ausschalten
            // TODO: Schlüsselbolzen des Schlüssels der zurückgegeben werden soll öffnen, alle anderen schließen
        } else {
            // TODO?: print a warning for a wrong RFID device
        }
    }
    // else if () {  // TODO: Warte bis Schlüsselsensor HIGH ist, dann Zeige Meldung:
    strcpy(textRow[0], "Vorgang");
    strcpy(textRow[1], "abgeschlossen.");
    strcpy(textRow[2], "Schliesse Tuer.");
    strcpy(textRow[3], "");
    //}
}

void initiateLoggedInKeySearch() {
    setStatusLed(GREEN);

    // TODO: Lese gesuchte keyNumberVar und Mitarbeiter aus SD Karte aus und schreibe in Textvariablen

    sprintf(textRow[0], "Schluesselnummer xx");
    strcpy(textRow[1], "liegt bei");
    strcpy(textRow[2], "Mitarbeiter");
    sprintf(textRow[3], "yy");
}
void loggedInKeySearch() {
    // state repetition:
    keyNumberVar = 0;                        // Zurücksetzen der keyNumberVar auf 0, für die nächste Suche eines Schlüssels
    static const long interval = 5000;       // Intervall in Millisekunden (hier 5 Sekunden)
    unsigned long currentMillis = millis();  // aktuelle Millisekunden speichern
    // TODO: Variable einführen die den zuletzt angemeldeten mitarbeiterID speichert um diese wieder aufrufen zu können beim wechsel.

    // state changeconditions:

    if (currentMillis - currentStateEnteredTime >= interval) {
        changeStateTo(LOGGED_IN);  // Wenn das Intervall abgelaufen ist, wechsle in den Zustand "loggedIn" mit der information welcher Mitarbeiter zuletzt angemeldet war
    } else if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            changeStateTo(LOGGED_IN_KEY_RETURN);  // Wenn ein Schlüssel gescannt wird, wechsle in den Zustand "loggedInKeyReturn" mit der information welcher Mitarbeiter zuletzt angemeldet war
        } else {
            // print a warning for a wrong RFID device
        }
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestKeySearch() {
    setStatusLed(RED);

    // TODO: Lese gesuchte keyNumberVar und Mitarbeiter aus SD Karte aus und schreibe in Textvariablen

    sprintf(textRow[0], "Schluesselnummer xx");
    strcpy(textRow[1], "liegt bei");
    strcpy(textRow[2], "Mitarbeiter");
    sprintf(textRow[3], "yy");
}
void guestKeySearch() {
    // state repetition:
    keyNumberVar = 0;                        // Zurücksetzen der keyNumberVar auf 0, für die nächste Suche eines Schlüssels
    static const long interval = 5000;       // Intervall in Millisekunden (hier 5 Sekunden)
    unsigned long currentMillis = millis();  // aktuelle Millisekunden speichern

    // state changeconditions:

    if (currentMillis - currentStateEnteredTime >= interval) {
        changeStateTo(READY);  // Wenn das Intervall abgelaufen ist, wechsle in den Zustand "READY"

    } else if (isRfidPresented()) {
        long rfidId = getRfidId();
        if (isRfidKey(rfidId)) {
            // TODO if Abfrage ob Schlüssel der eingescannt wurde bereits zurückgegeben wurde, wenn ja changeStateTo(WRONG_KEY_EXCHANGE)
            changeStateTo(GUEST_KEY_RETURN);  // Wenn ein Schlüssel gescannt wird, wechsle in den Zustand "guestKeyReturn"
        } else if (isRfidEmployee(rfidId)) {
            changeStateTo(LOGGED_IN);  // Wenn ein RFID-Chip gescant wird, wechsle in den Zustand "loggedIn"
        } else {
            // print a warning for a wrong RFID device
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions:

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

boolean isDoorReadyForClosing() {
    // 5s time to open the door manually, before trying to close it automatically
    return digitalRead(doorLockSensor) == HIGH && millis() - currentStateEnteredTime > 5000;
}

// Funktion zum öffnen des Türschlosses
void openDoorLock() {
    digitalWrite(doorLock, HIGH);  // Türschloss auf
}

// Funktion zum schließen des Türschlosses
void closeDoorLock() {
    digitalWrite(doorLock, LOW);  // Türschloss zu
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

                keyNumber = (firstKey - '0') * 10 + (key - '0');  // Extrahiere numerischen Wert aus den beiden Zahlen und rechne zusammen

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
        firstKey = '\0';       // Setze die erste Ziffer zurück
        changeStateTo(state);  // Initialisiere den aktuellen State neu um das Display auf den Ausgangszustand zurückzusetzen
    }

    return keyNumber;  // Gib die Zahlenkombination zurück
}

// RFID Funktionen
boolean isRfidPresented() {
    return (rfidReader.PICC_IsNewCardPresent() && rfidReader.PICC_ReadCardSerial());
}

boolean isRfidKey(long rfidId) {
    return database.isIdKey(rfidId);
}

boolean isRfidEmployee(long rfidId) {
    return database.isIdEmployee(rfidId);
}

long getRfidId() {
    long code = 0;
    for (byte i = 0; i < rfidReader.uid.size; i++) {
        code = ((code + rfidReader.uid.uidByte[i]) * 10);
    }
    return code;
}
