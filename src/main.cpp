// Project Description...

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// included libraries:
#include <Arduino.h>
#include <Keypad.h>             // Library for Keypad
#include <LiquidCrystal_I2C.h>  // Library for I2C Display
#include <SD.h>                 // Library for SD Card Reader
#include <SPI.h>

#include "RfidReader.h"
#include "Database.h"
#include "SimpleFunctions.h"

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
#define alarmSiren 2      // Pin-Definition für die Alarmsirene //TODO: Pin-Definition anpassen
#define doorLock 7        // Pin-Definition für das Türschloss //TODO: Pin-Definition anpassen
#define doorLockSensor 8  // Pin-Deginition für den Türschloss-Sensor //TODO: Pin-Definition anpassen

// RFID-Reader
#define PN5180_NSS 53   //TODO: Pin-Definition anpassen
#define PN5180_BUSY 48
#define PN5180_RST 49
RfidReader rfidReader(PN5180_NSS, PN5180_BUSY, PN5180_RST);


// SD-Card Reader
Database* database;
#define sdOut 53 //TODO: Pin-Definition anpassen

// Status-LED
#define statusLedRed 22     // Pin für den roten Kanal
#define statusLedGreen 23  // Pin für den grünen Kanal
#define statusLedBlue 24   // Pin für den blauen Kanal

enum LedColor {
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

// Shift-Registers
#define clockPinSerialOutSr 2 //TODO: Pin-Definition anpassen
#define latchPinSerialOutSr 3 //TODO: Pin-Definition anpassen
#define dataPinSerialOutSr 6 //TODO: Pin-Definition anpassen

#define clockPinKeyReedSr 1 //TODO: Pin-Definition anpassen
#define latchPinKeyReedSr 2 //TODO: Pin-Definition anpassen
#define dataPinKeyReedSr 3 //TODO: Pin-Definition anpassen

// Keypad
const byte rowAmount = 4;     // vier Reihen
const byte columnAmount = 3;  // drei Spalten

const char keys[rowAmount][columnAmount] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte pinRows[rowAmount] = {8, 7, 6, 5};    // verbinde die Reihen mit diesen Pins
byte pinColumn[columnAmount] = {4, 3, 2};  // verbinde die Spalten mit diesen Pins

Keypad keypad = Keypad(makeKeymap(keys), pinRows, pinColumn, rowAmount, columnAmount);

char firstKey = '\0';
unsigned long lastKeyPressTime = 0;
const unsigned long resetTimeout = 5000;  // Zeitlimit für die Eingabe in Millisekunden (hier 5 Sekunden)
int keyNumberVar = 0;                     // Globale Variable für die Zahlenkombination des Keypads
byte buttonState = 0;                     // Tasterstatus


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state deklarations:
enum State {
    STARTING,
    INACTIVE,

    READY,

    LOGGED_IN,

    LOGGED_IN_KEY_RETURN,

    GUEST_KEY_RETURN,
    GUEST_WAITING,

    WRONG_KEY_EXCHANGE,

    LOGGED_IN_KEY_SEARCH,
    GUEST_KEY_SEARCH,

    RFID_TEACH_IN
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global variables:
State state = STARTING;

SimpleQueue<int, 30> unloggedChanges;
long keyLendingArray[keySlotCount];  // saves the employeeId if the key is lent, else 0
boolean isKeyPresentArray[keySlotCount];
boolean newIsKeyPresentArray[keySlotCount];

boolean* isEmployeePermissionedArray;

int opendKeyLock = -1;              // saves the keyNumber of the key that is currently opened, else -1
boolean redKeyLeds[keySlotCount];
boolean greenKeyLeds[keySlotCount];

long currentKeyId;
long currentKeyNumber;
long currentEmployeeId;

unsigned long currentStateEnteredTime;

static const long adminTeachInRfid = 1234567890;  //TODO: RFID-Nummer für Admin anpassen

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// function declarations:
void changeStateTo(State newState);
void initiateInactive();
void inactive();
void initiateReady();
void ready();
void initiateLoggedIn();
void loggedIn();
void initiateLoggedInKeyReturn();
void loggedInKeyReturn();
void initiateGuestKeyReturn();
void guestKeyReturn();
void initiateGuestWaiting();
void guestWaiting();
void initiateWrongKeyExchange();
void wrongKeyExchange();
void initiateLoggedInKeySearch();
void loggedInKeySearch();
void initiateGuestKeySearch();
void guestKeySearch();
void setStatusLed(LedColor color); 
void setStatusLed(int red, int green, int blue);
void blinkStatusLed(LedColor color);
void checkSirene();
boolean isDoorReadyForClosing();
void openDoorLock();
void closeDoorLock();
char charAt(char* text, int pos);
void taskText(char* text, byte line);
char keypadReadout();
boolean isIdKey(long rfidId);
boolean isIdEmployee(long rfidId);
int getTakenKey();
void updateKeyLedsAndLocks();
void updateNewIsKeyPresentArray();
void initiateRfidTeachIn();
void rfidTeachIn();


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup:
void setup() {
    Serial.begin(9600);
    SPI.begin();

    // LCD Display initialisation
    lcd.init();
    lcd.noBacklight();
    // Willkommenstext anzeigen
    strcpy(textRow[0], "IFU Stuttgart       ");
    strcpy(textRow[1], "Schluesselausgabe   ");
    strcpy(textRow[2], "2024 Version 1.0    ");
    strcpy(textRow[3], "Tuer schliessen!    ");
    // Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays
    taskText(textRow[0], 0);
    taskText(textRow[1], 1);
    taskText(textRow[2], 2);
    taskText(textRow[3], 3);

    // SD-Card Reader initialisation
    pinMode(sdOut, OUTPUT);
    if (!SD.begin(sdOut)) {
        Serial.println("Couldn't initialize SD card reader!");
        while (true) {
        }
    }
    database = new Database(SD);

    // Shift-Registers
    pinMode(clockPinSerialOutSr, OUTPUT);
    pinMode(latchPinSerialOutSr, OUTPUT);

    pinMode(dataPinSerialOutSr, OUTPUT);

    pinMode(clockPinKeyReedSr, OUTPUT);
    pinMode(latchPinKeyReedSr, OUTPUT);
    pinMode(dataPinKeyReedSr, INPUT);

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
    checkSirene();
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
        case LOGGED_IN_KEY_SEARCH:
            loggedInKeySearch();
            break;
        case GUEST_KEY_SEARCH:
            guestKeySearch();
            break;
        case RFID_TEACH_IN:
            rfidTeachIn();
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
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    opendKeyLock = -1;
    updateKeyLedsAndLocks();
    openDoorLock();  // Türschloss zu Beginn öffnen falls bei Start Tür nicht zu.

    updateNewIsKeyPresentArray();
    for (int i = 0; i < keySlotCount; i++) {
        isKeyPresentArray[i] = newIsKeyPresentArray[i];
    }

    strcpy(textRow[0], "Bitte Tuere        ");
    strcpy(textRow[1], "schliessen!         ");
    strcpy(textRow[2], "                    ");
    strcpy(textRow[3], "                    ");
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
    database->logChanges(unloggedChanges, keyLendingArray);
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    opendKeyLock = -1;
    updateKeyLedsAndLocks();

    strcpy(textRow[0], "Scannen Sie Ihren   ");
    strcpy(textRow[1], "RFID-Chip oder      ");
    strcpy(textRow[2], "einen               ");
    strcpy(textRow[3], "Schluessel          ");
}
void ready() {
    // state repetition:

    // state changeconditions:
    keyNumberVar = keypadReadout();  // Auslesen des Keypads
    if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {
            currentKeyId = rfidId;
            currentKeyNumber = database->getKeyNumber(currentKeyId);
            if (isKeyPresentArray[currentKeyNumber]) {  // Schlüssel Platz belegt?
                changeStateTo(WRONG_KEY_EXCHANGE);
            } else {
                changeStateTo(GUEST_KEY_RETURN);
            }
        } else if (isIdEmployee(rfidId)) {
            currentEmployeeId = rfidId;
            changeStateTo(LOGGED_IN);
        } else if (rfidId == adminTeachInRfid) {
            changeStateTo(RFID_TEACH_IN);
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(GUEST_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in guestKeySearch
    }
}

void initiateLoggedIn() {
    setStatusLed(GREEN);
    openDoorLock();
    isEmployeePermissionedArray = database->getEmployeeKeyPermissions(currentEmployeeId);
    for (int i = 0; i < keySlotCount; i++) {
        if (isKeyPresentArray[i]) {
            if (isEmployeePermissionedArray[i]) {
                redKeyLeds[i] = false;
                greenKeyLeds[i] = true;
            } else {
                redKeyLeds[i] = true;
                greenKeyLeds[i] = false;

            }
        } else {
            redKeyLeds[i] = false;
            greenKeyLeds[i] = false;
        }
    }
    updateKeyLedsAndLocks();

    strcpy(textRow[0], "Schluessel eingeben ");
    strcpy(textRow[1], "u. entnehmen o. zur ");
    strcpy(textRow[2], "Rueckgabe Schlues-  ");
    strcpy(textRow[3], "sel einscannen      ");
}
void loggedIn() {
    // state repetition:
    updateNewIsKeyPresentArray();
    int takenKey = getTakenKey();
    if (takenKey != -1) {  // got Key taken?
        isKeyPresentArray[takenKey] = 0;
        keyLendingArray[takenKey] = currentEmployeeId;
        redKeyLeds[takenKey] = false;
        greenKeyLeds[takenKey] = false;
        opendKeyLock = -1;
        updateKeyLedsAndLocks();
    }

    // state changeconditions:
    keyNumberVar = keypadReadout();  // Auslesen des Keypads

    if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {  // Schlüssel gescannt?
            currentKeyId = rfidId;
            currentKeyNumber = database->getKeyNumber(currentKeyId);
            if (isKeyPresentArray[currentKeyNumber]) {  // Schlüssel Platz belegt?
                changeStateTo(WRONG_KEY_EXCHANGE);
            } else {
                changeStateTo(LOGGED_IN_KEY_RETURN);
            }
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(LOGGED_IN_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in "loggedInKeySearch"
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateLoggedInKeyReturn() {
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    redKeyLeds[currentKeyNumber] = true;
    opendKeyLock = currentKeyNumber;
    updateKeyLedsAndLocks();
    openDoorLock();

    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schluessel zurück  ");
    strcpy(textRow[3], "                    ");
}
void loggedInKeyReturn() {
    // state repetition:
    blinkStatusLed(GREEN);

    // state changeconditions:
    updateNewIsKeyPresentArray();
    if (newIsKeyPresentArray[currentKeyNumber] == true) {
        isKeyPresentArray[currentKeyNumber] = true;
        keyLendingArray[currentKeyNumber] = 0;
        opendKeyLock = -1;
        changeStateTo(LOGGED_IN);
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestKeyReturn() {
    openDoorLock();
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    redKeyLeds[currentKeyNumber] = true;
    greenKeyLeds[currentKeyNumber] = false;
    opendKeyLock = currentKeyNumber;
    updateKeyLedsAndLocks();

    strcpy(textRow[0], "Stecken Sie         ");
    strcpy(textRow[1], "den gescannten      ");
    strcpy(textRow[2], "Schluessel zurueck  ");
    strcpy(textRow[3], "                    ");
}
void guestKeyReturn() {
    // state repetition:
    blinkStatusLed(BLUE);

    // state changeconditions:
    updateNewIsKeyPresentArray();
    if (newIsKeyPresentArray[currentKeyNumber] == true) {
        isKeyPresentArray[currentKeyNumber] = true;
        keyLendingArray[currentKeyNumber] = 0;
        opendKeyLock = -1;
        changeStateTo(GUEST_WAITING);
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestWaiting() {
    setStatusLed(BLUE);
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    updateKeyLedsAndLocks();

    strcpy(textRow[0], "Vorgang             ");
    strcpy(textRow[1], "abgeschlossen. Neuen");
    strcpy(textRow[2], "Schluessel scannen  ");
    strcpy(textRow[3], "o. Tuere schliessen.");
}
void guestWaiting() {
    // state repetition:

    // state changeconditions:
    if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {
            currentKeyId = rfidId;
            currentKeyNumber = database->getKeyNumber(currentKeyId);
            if (isKeyPresentArray[currentKeyNumber]) {  // Schlüssel Platz belegt?
                changeStateTo(WRONG_KEY_EXCHANGE);
            } else {
                changeStateTo(GUEST_KEY_RETURN);
            }
        } else if (isIdEmployee(rfidId)) {
            currentEmployeeId = rfidId;
            changeStateTo(LOGGED_IN);
        }
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateWrongKeyExchange() {
    openDoorLock();
    for (int i = 0; i < keySlotCount; i++) {
        redKeyLeds[i] = false;
        greenKeyLeds[i] = false;
    }
    redKeyLeds[currentKeyNumber] = true;
    greenKeyLeds[currentKeyNumber] = false;
    opendKeyLock = currentKeyNumber;
    updateKeyLedsAndLocks();

    strcpy(textRow[0], "Entnehmen Sie den   ");
    strcpy(textRow[1], "Schluessel an der   ");
    strcpy(textRow[2], "roten LED           ");
    strcpy(textRow[3], "                    ");
}
void wrongKeyExchange() {
    // state repetition:
    blinkStatusLed(RED);

    // state changeconditions:
    updateNewIsKeyPresentArray();
    if (newIsKeyPresentArray[currentKeyNumber] == false) {
        isKeyPresentArray[currentKeyNumber] = false;
        keyLendingArray[currentKeyNumber] = keyLendingArray[database->getKeyNumber(currentKeyId)];
        opendKeyLock = -1;
        changeStateTo(GUEST_WAITING);
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateLoggedInKeySearch() {
    setStatusLed(GREEN);
    currentKeyNumber = keyNumberVar;
    keyNumberVar = 0;                        // Zurücksetzen der keyNumberVar auf 0, für die nächste Suche eines Schlüssels

    if (isKeyPresentArray[currentKeyNumber]) {  // Ist Schlüssel vorhanden?
        if (isEmployeePermissionedArray[currentKeyNumber]) {        // Ist Schlüssel für Mitarbeiter freigegeben?
            strcpy(textRow[0], "Schluessel        " + currentKeyNumber);  
            strcpy(textRow[1], "kann entnommen      ");
            strcpy(textRow[2], "werden              ");
            sprintf(textRow[3], "                    ");

            opendKeyLock = currentKeyNumber;

        } else {                            // Schlüssel ist nicht für Mitarbeiter freigegeben
            strcpy(textRow[0], "Keine Berechtigung  ");
            strcpy(textRow[1], "fuer Schluessel     ");
            strcpy(textRow[2], "                  " + currentKeyNumber);
            sprintf(textRow[3], "                    ");
            opendKeyLock = -1;
        }

    } else {      // Schlüssel ist nicht vorhanden

        strcpy(textRow[2], "                    "); // Textzeile 3 wird geleert damit der Mitarbeitername korrekt angezeigt werden kann
        taskText(textRow[2], 2);                    // Textzeile 3 wird aktualisiert

        sprintf(textRow[0], "Schluessel        " + currentKeyNumber);
        strcpy(textRow[1], "liegt bei           ");
        strcpy(textRow[2], database->getEmployeeName(keyLendingArray[currentKeyNumber]));
        sprintf(textRow[3], "                    ");
        opendKeyLock = -1;
    }
    updateKeyLedsAndLocks();

}
void loggedInKeySearch() {
    // state repetition:
    static const long interval = 5000;       // Intervall in Millisekunden (hier 5 Sekunden)
    unsigned long currentMillis = millis();  // aktuelle Millisekunden speichern

    // state changeconditions:
    updateNewIsKeyPresentArray();
    int takenKey = getTakenKey();
    keyNumberVar = keypadReadout();  // Auslesen des Keypads
    if (takenKey != -1) {            // got Key taken?
        isKeyPresentArray[takenKey] = 0;
        keyLendingArray[takenKey] = currentEmployeeId;
        opendKeyLock = -1;
        changeStateTo(LOGGED_IN);
    } else if (currentMillis - currentStateEnteredTime >= interval) {
        changeStateTo(LOGGED_IN);  // Wenn das Intervall abgelaufen ist, wechsle in den Zustand "loggedIn" mit der information welcher Mitarbeiter zuletzt angemeldet war
    } else if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {  // Schlüssel gescannt?
            currentKeyId = rfidId;
            currentKeyNumber = database->getKeyNumber(currentKeyId);
            if (isKeyPresentArray[currentKeyNumber]) {  // Schlüssel Platz belegt?
                changeStateTo(WRONG_KEY_EXCHANGE);
            } else {
                changeStateTo(LOGGED_IN_KEY_RETURN);
            }
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(LOGGED_IN_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in "loggedInKeySearch"
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateGuestKeySearch() {
    setStatusLed(RED);
    currentKeyNumber = keyNumberVar;
    keyNumberVar = 0;                        // Zurücksetzen der keyNumberVar auf 0, für die nächste Suche eines Schlüssels

    if (isKeyPresentArray[currentKeyNumber]) {  // Ist Schlüssel vorhanden?
        strcpy(textRow[0], "Schluessel        " + currentKeyNumber);  
        strcpy(textRow[1], "ist im Schrank      ");
        strcpy(textRow[2], "                   ");
        sprintf(textRow[3], "                    ");

    } else {      // Schlüssel ist nicht vorhanden

        strcpy(textRow[2], "                    "); // Textzeile 3 wird geleert damit der Mitarbeitername korrekt angezeigt werden kann
        taskText(textRow[2], 2);                    // Textzeile 3 wird aktualisiert

        sprintf(textRow[0], "Schluessel        " + currentKeyNumber);
        strcpy(textRow[1], "liegt bei           ");
        strcpy(textRow[2], database->getEmployeeName(keyLendingArray[currentKeyNumber]));
        sprintf(textRow[3], "                    ");
        opendKeyLock = -1;
    }
}

void guestKeySearch() {
    // state repetition:
    static const long interval = 5000;       // Intervall in Millisekunden (hier 5 Sekunden)
    unsigned long currentMillis = millis();  // aktuelle Millisekunden speichern

    // state changeconditions:
    if (currentMillis - currentStateEnteredTime >= interval) {
        changeStateTo(READY);  // Wenn das Intervall abgelaufen ist, wechsle in den Zustand "READY"
    } else if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {
            currentKeyId = rfidId;
            currentKeyNumber = database->getKeyNumber(currentKeyId);
            if (isKeyPresentArray[currentKeyNumber]) {  // Schlüssel Platz belegt?
                changeStateTo(WRONG_KEY_EXCHANGE);
            } else {
                changeStateTo(GUEST_KEY_RETURN);
            }
        } else if (isIdEmployee(rfidId)) {
            currentEmployeeId = rfidId;
            changeStateTo(LOGGED_IN);
        }
    } else if (isDoorReadyForClosing()) {
        changeStateTo(READY);
    }
}

void initiateRfidTeachIn() {
    strcpy(textRow[0], "Schluessel oder     ");
    strcpy(textRow[1], "Mitarbeiterchip     ");
    strcpy(textRow[2], "zum Anlernen        ");
    strcpy(textRow[3], "einscannen          ");
}

void rfidTeachIn() {
    // state repetition:
    blinkStatusLed(WHITE);

    // state changeconditions:
    static const long interval = 10000;       // Intervall in Millisekunden (hier 10 Sekunden)
    unsigned long currentMillis = millis();  // aktuelle Millisekunden speichern

    // state changeconditions:
    if (currentMillis - currentStateEnteredTime >= interval) {
        changeStateTo(READY);  // Wenn das Intervall abgelaufen ist, wechsle in den Zustand "READY"
    } else if (rfidReader.isTagPresented()) {
        long rfidId = rfidReader.getLastReadId();
        if (isIdKey(rfidId)) {
        } else if (isIdEmployee(rfidId)) {
        } else if (rfidId == adminTeachInRfid) {
        } else {
            database->addNewTeachedRfid(rfidId);
            changeStateTo(READY);
        }
    } else if (keyNumberVar != 0) {
        changeStateTo(GUEST_KEY_SEARCH);  // Wenn eine Zahlenkombination eingegeben wurde wechsle in guestKeySearch
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
    unsigned int blinkDuration = 500;  // in ms
    if (millis() % (blinkDuration * 2) < blinkDuration) {
        setStatusLed(color);
    } else {
        setStatusLed(OFF);
    }
}

void checkSirene() {
    unsigned int interval = 20000;
    if (state != READY) {                                      // Wenn der Status "READY" ist, wird die Sirene nicht aktiviert
        if (millis() - currentStateEnteredTime >= interval) {  // Wenn seit 20 Sekunden der Status nicht gewechselt wurde, geht die Sirene an
            digitalWrite(alarmSiren, HIGH);
        } else {
            digitalWrite(alarmSiren, LOW);
        }
    } else {
        digitalWrite(alarmSiren, LOW);
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
char charAt(char* text, int pos)
// scrolling-logic coded here
{
    if (pos < ledLine)
        return ' ';  // scroll in
    else if (pos >= ledLine && pos < ledLine + int(strlen(text)))
        return text[pos - ledLine];  // scroll text
    else
        return ' ';  // scroll out
}

// Zeitlicher Ablauf wann gescrollt werden muss und ob die Textlänge ein Scrollen erfordert prüfen und ggf. Scrollen durchführen
void taskText(char* text, byte line)
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
    if (ledScrollDir[line] == true && positionCounter[line] > int(strlen(text) + ledLine)) {  // für nach links scrollen
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
            if (positionCounter[line] == int(strlen(text)) + ledLine) positionCounter[line] = 0;
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

    if (isDigit(key)) {  // Überprüfe, ob die gedrückte Taste eine Zahl ist

        if (firstKey == '\0') {  // Überprüfe, ob die erste Ziffer bereits gedrückt wurde
            // Speichere die erste gedrückte Zahl
            firstKey = key;
            lastKeyPressTime = currentTime;  // Setze die Zeit für die erste Ziffer

            // Text für LCD Display
            strcpy(textRow[0], "Schluesselnummer:   ");
            sprintf(textRow[1], "                  0%c", firstKey);
            strcpy(textRow[2], "                    ");
            strcpy(textRow[3], "                    ");

        } else {  // Zwei Zahlen nacheinander wurden gedrückt

            keyNumber = (firstKey - '0') * 10 + (key - '0');  // Extrahiere numerischen Wert aus den beiden Zahlen und rechne zusammen

            if (keyNumber != 0 && keyNumber <= 50) { //Überprüft ob die eingegebene Zahl zwischen 1 und 50 liegt
            // Text für LCD Display
            strcpy(textRow[0], "Schluesselnummer:   ");
            sprintf(textRow[1], "                %d", keyNumber); 
            strcpy(textRow[2], "                    ");
            strcpy(textRow[3], "                    ");

            Serial.print(keyNumber);

            }
            else{
            strcpy(textRow[0], "Schluesselnummer:   ");
            sprintf(textRow[1], "existiert nicht     ");
            strcpy(textRow[2], "                    ");
            strcpy(textRow[3], "                    ");
            }

            firstKey = '\0';  // Setze firstKey zurück, um auf die nächste Zahlenkombination zu warten
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
boolean isIdKey(long rfidId) {
    return database->isIdKey(rfidId);
}

boolean isIdEmployee(long rfidId) {
    return database->isIdEmployee(rfidId);
}

/**
 * @return Index of changed Key, or -1 if no Key changed.
 */
int getTakenKey() {
    return getNotEqualIndex(isKeyPresentArray, newIsKeyPresentArray);
}

// TODO Test
void updateKeyLedsAndLocks() {
    for (int i = 0; i < keySlotCount; i++) {
        digitalWrite(latchPinSerialOutSr, LOW);
        digitalWrite(dataPinSerialOutSr, redKeyLeds[i]);
        digitalWrite(clockPinSerialOutSr, HIGH);
        digitalWrite(clockPinSerialOutSr, LOW);
        digitalWrite(latchPinSerialOutSr, HIGH);

        digitalWrite(latchPinSerialOutSr, LOW);
        digitalWrite(dataPinSerialOutSr, greenKeyLeds[i]);
        digitalWrite(clockPinSerialOutSr, HIGH);
        digitalWrite(clockPinSerialOutSr, LOW);
        digitalWrite(latchPinSerialOutSr, HIGH);

        digitalWrite(latchPinSerialOutSr, LOW);
        if(i == opendKeyLock){
            digitalWrite(dataPinSerialOutSr, true);
        } else {
            digitalWrite(dataPinSerialOutSr, false);
        }
        digitalWrite(clockPinSerialOutSr, HIGH);
        digitalWrite(clockPinSerialOutSr, LOW);
        digitalWrite(latchPinSerialOutSr, HIGH);
    }
}

void updateNewIsKeyPresentArray() {
    digitalWrite(latchPinKeyReedSr, LOW);
    for (int i = 0; i < keySlotCount; i++) {
        digitalWrite(clockPinKeyReedSr, HIGH);
        // TODO TEST if mirrored
        newIsKeyPresentArray[i] = digitalRead(dataPinKeyReedSr);
        digitalWrite(clockPinKeyReedSr, LOW);
    }
    digitalWrite(latchPinKeyReedSr, HIGH);
}
