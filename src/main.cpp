//geladene Bibliotheken
#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h> // Bibliothek für I2C Display
#include <Keypad.h>

// Anzahl der im LCD Display vorhandenen Zeichen pro Zeile (verwendet wird ein 20x4 Zeichen I2C Display)
const byte LEDLINE = 20;
LiquidCrystal_I2C lcd(0x27,20,4); //Einstellen der LCD Adresse auf 0x27, sowie für 20 Zeichen und 4 Zeilen

// Definieren der Scrollgeschwindigkeit für die 4 LED Zeilen. es sind individuelle Geschwinidkeit je Zeile möglich
const int ledScrollSpeed[4]={300,300,300,300};
// Definieren der Scrollrichtung für ein 4 Zeilen Display (true bedeutet von rechts nach links))
const boolean ledScrollDir[4]={true,true,true,true};   
// Definition der Textvariablen des LCD Displays - 2-dimensionales Array für 4 Zeilen mit je 178 Zeichen --> //Da das LCD Display 20 Zeilen hat müssen im String immer mindestens 20 Zeichen angegeben sein --> Ggf. mit Leerzeichen auf 20 Zeichen auffüllen! Sonst kann es zu Artefakten auf dem Display vom vorherigen Text kommen!
char TextZeile[4][178];
char LastTextZeile[4][178];  //{{0},{0},{0},{0}};    letzten Zustand der Textzeile merken um Veränderungen zu erkennen!

// Pin-Definition für die Alarmsirene
#define alarm_siren 2

// Pin-Definition für das Türschloss
#define door_lock 7

// Pin-Deginition für den Türschloss-Sensor
#define door_lock_sensor 8

// Pin-Definitionen für das RFID-Lesegerät
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfidReader(SS_PIN, RST_PIN);

// Pin-Definitionen für die Status-LED
#define StatusLED_red 9                     // Pin für den roten Kanal
#define StatusLED_green 10                  // Pin für den grünen Kanal
#define StatusLED_blue 11                   // Pin für den blauen Kanal

// Pin-Definitionen für das Keypad
    const byte ROW_NUM    = 4; // vier Reihen
    const byte COLUMN_NUM = 3; // drei Spalten

    char keys[ROW_NUM][COLUMN_NUM] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
    };

    byte pin_rows[ROW_NUM] = {9, 8, 7, 6};    // verbinde die Reihen mit diesen Pins
    byte pin_column[COLUMN_NUM] = {5, 4, 3}; // verbinde die Spalten mit diesen Pins

    Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

    char firstKey = '\0';
    unsigned long lastKeyPressTime = 0;
    const unsigned long resetTimeout = 5000; // Zeitlimit für die Eingabe in Millisekunden (hier 5 Sekunden)


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deklaration der Zustände:
typedef enum {
    INACTIVE,
    TESTING,

    READY,

    LOGGED_IN,

    LOGGED_IN_KEY_RETURN,

    GUEST_KEY_RETURN,
    GUEST_WAITING,

    WRONG_KEY_EXCHANGE,
    WAITING_FOR_WRONG_KEY,

    LOGGED_IN_KEY_SEARCH,
    GUEST_KEY_SEARCH

} State;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globale Deklaration der Variablen:
State state = INACTIVE;                      // Zustandsvariable

bool should_StatusLED_blink = false;        // Varibale zur Steuerung des Blinkens

int StatusLED_red_var = 255;                // Standartfarbe der Status-LED festlegen (Zahlenwert zwischen 0-255 möglich)        
int StatusLED_green_var = 255;              // Standartfarbe der Status-LED festlegen (Zahlenwert zwischen 0-255 möglich)       
int StatusLED_blue_var = 255;               // Standartfarbe der Status-LED festlegen (Zahlenwert zwischen 0-255 möglich)       

int key_number_var = 0;                     // Globale Variable für die Zahlenkombination des Keypads

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialisiere Code:
void setup() {
    Serial.begin(9600);
        SPI.begin();
    rfidReader.PCD_Init();

    //LCD Display initialisieren und Hintergrundbeleuchtung aktivieren
    lcd.init(); 
    lcd.backlight();

    // Willkommenstext anzeigen
    strcpy(TextZeile[0], "    IFU Stuttgart   "); //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "  Schlüsselausgabe "); //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "  2024 Version 1.0  "); //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "->Tür schließen!<-"); //Text für Zeile 4 des LCD Displays

    //Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays
    task_text(TextZeile[0], 0);
    task_text(TextZeile[1], 1);
    task_text(TextZeile[2], 2);
    task_text(TextZeile[3], 3);

    // Initialisiere die Pins der Status-LED als Ausgänge
    pinMode(StatusLED_red, OUTPUT);
    pinMode(StatusLED_green, OUTPUT);
    pinMode(StatusLED_blue, OUTPUT);

    // Initialisiere den Pin des Schrankschlosses als Ausgang
    pinMode(door_lock, OUTPUT);

    // Initialisiere den Pin des Schrankschloss-Sensors als Eingang
    pinMode(door_lock_sensor, INPUT);

    // Initialisiere den Pin der Alarmsirene als Ausgang
    pinMode(alarm_siren, OUTPUT);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino Loop:
void loop() {
    switch (state) {
        case INACTIVE:
            inactive();
            break;
        case READY:
            ready();
            break;
        case LOGGED_IN:
            logged_in();
            break;
        case LOGGED_IN_KEY_RETURN:
            logged_in_key_return();
            break;
        case GUEST_KEY_RETURN:
            guest_key_return();
            break;
        case GUEST_WAITING:
            guest_waiting();
            break;
        case WRONG_KEY_EXCHANGE:
            wrong_key_exchange();
            break;
        case WAITING_FOR_WRONG_KEY:
            waiting_for_wrong_key();
            break;
        case LOGGED_IN_KEY_SEARCH:
            logged_in_key_search();
            break;
        case GUEST_KEY_SEARCH:
            guest_key_search();
            break;
        default:
            break;
    }

    // Wenn die Bedingung erfüllt ist, führe die Status-LED Blink-Funktion aus, sonst setzte permanent die Farbe der Status-LED
    if (should_StatusLED_blink) {
        blinkStatusLED();
    }
    else {
        setcolor_status_led_on();
    }

    // Überprüfe ob die Schranktür geschlossen ist
    check_door_lock_sensor();

    //Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays am Ende der Void Loop Funktion!
    task_text(TextZeile[0],0);
    task_text(TextZeile[1],1);
    task_text(TextZeile[2],2);
    task_text(TextZeile[3],3);

}

///////////////////////////////////////////////////////////////////////////////////////////////
// Funktionen:

// Funktion zum Setzen der Farbe der Status-LED
void setcolor_status_led_on() {

    analogWrite(StatusLED_red, StatusLED_red_var);
    analogWrite(StatusLED_green, StatusLED_green_var);
    analogWrite(StatusLED_blue, StatusLED_blue_var);

}

// Funktion zum Ausschalten der Status-LED
void setcolor_status_led_off() {
    
    analogWrite(StatusLED_red, 0);
    analogWrite(StatusLED_green, 0);
    analogWrite(StatusLED_blue, 0);
}

// Funktion zum Blinken der Status-LED
void blink_status_led() {

    int duration = 500;                                     // Blinken alle 500 Millisekunden (0,5 Sekunden)
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= duration) {
    previousMillis = currentMillis;

    // Wechsle den Zustand der Status-LED (an/aus)
    static bool isStatusLEDOn = false;
    if (isStatusLEDOn) {
      setcolor_status_led_off(); // Schalte die LED aus
    } else {
      setcolor_status_led_on(); // Setze die Farbe der LED
    }

    isStatusLEDOn = !isStatusLEDOn;
  }

}


// Funktion zum öffnen des Türschlosses
void open_door_lock() {

    digitalWrite(door_lock, HIGH);      // Türschloss auf

}

// Funktion zum schließen des Türschlosses
void close_door_lock() {

    digitalWrite(door_lock, LOW);       // Türschloss zu

}

// Funktion zum überprüfen ob Tür geschlossen ist
void check_door_lock_sensor() {

    if (digitalRead(door_lock_sensor) == HIGH && state != wrong_key_exchange) { // Wenn der Türschloss-Sensor HIGH ist und der Zustand nicht "wrong_key_exchange" ist, da dieser Vorgang auf jeden Fall abgeschlossen werden muss bevor in "ready" gewechselt werden kann
        delay(3000);                    // Warte 3 Sekunden bis das Türschloss sich schließt
        close_door_lock();              // Türschloss schließen
        state = READY;
    }

}

//Funktion für die Scrollfunktion, um den Text in der Variable zu schieben!
char charAt(char *text, int pos)
// scrolling-logic coded here
{
  if (pos<LEDLINE) return ' '; // scroll in
  else if (pos>=LEDLINE && pos<LEDLINE+strlen(text))
    return text[pos-LEDLINE]; // scroll text
  else return ' ';  // scroll out
}

//Zeitlicher Ablauf wann gescrollt werden muss und ob die Textlänge ein Scrollen erfordert prüfen und ggf. Scrollen durchführen
void task_text(char *text, byte line)
// scroll the LED lines
{
  char currenttext[LEDLINE+1];
  static unsigned long nextscroll[4];
  static int positionCounter[4]; 
  int i;
  
  //Textinhalte des 2dimensionalen Arrays vergleichen. hats sich inhaltlich ewats verändert --> Position dieser Zeile auf Null setzen
  if (strcmp(text, LastTextZeile[line]) != 0){
    positionCounter[line] = 0;
    //Hier muss der aktuelle Text in die LastTextzeile[][] Variable kopiert werden!
    strcpy(LastTextZeile[line], text);
  }
  //Beim Wechsel der Textlänge kann der positionCounter des Textes beim Scrollen zu hoch sein und sich nicht mehr zurückstellen --> Dann würde der Text nicht angezeigt werden! --> Hier muss die Position  zurückgesetzt werden!
  if (ledScrollDir[line] == true && positionCounter[line] > (strlen(text) + LEDLINE)){ //für nach links scrollen
    positionCounter[line]=0;
  }
  else if (positionCounter[line]<0){ //für nach rechts scrollen
    positionCounter[line]=strlen(text)+LEDLINE-1;
  }
  

  
  //Prüfen ob Zeitpunkt zum Scrollen jetzt vorhanden und ob die Textlänge scrollen nötig macht!
  if (millis()>nextscroll[line] && strlen(text)>LEDLINE)
  {
    nextscroll[line]=millis()+ledScrollSpeed[line]; 

    for (i=0;i<LEDLINE;i++)
      currenttext[i]=charAt(text,positionCounter[line]+i);
    #include <LiquidCrystal.h> // Include the LCD library

    LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // Declare the 'lcd' object

    currenttext[LEDLINE]=0;    

    lcd.setCursor(0,line);
    lcd.print(currenttext);
    if (ledScrollDir[line])
    {
        positionCounter[line]++;
        if (positionCounter[line]==strlen(text)+LEDLINE) positionCounter[line]=0;
    }  
    else  
    {
        positionCounter[line]--;
        if (positionCounter[line]<0) positionCounter[line]=strlen(text)+LEDLINE-1;
    }  
  }
  else if (millis() > nextscroll[line] && strlen(text) <= LEDLINE) //geändert von if -> else if
  {
    nextscroll[line]=millis()+ ledScrollSpeed[line];
    lcd.setCursor(0,line);
    lcd.print(text);
  }
}

// Funktion zum auslesen des Keypads
char keypad_readout() {

    static int key_number = 0;
    char key = keypad.getKey();
    unsigned long currentTime = millis();

    if (key) { // Überprüfe, ob eine Taste gedrückt wurde

        if (isDigit(key)) { // Überprüfe, ob die gedrückte Taste eine Zahl ist

            if (firstKey == '\0') { // Überprüfe, ob die erste Ziffer bereits gedrückt wurde
                // Speichere die erste gedrückte Zahl
                firstKey = key;
                lastKeyPressTime = currentTime; // Setze die Zeit für die erste Ziffer

                // Text für LCD Display
                strcpy(TextZeile[0], "Suche Schlüssel:   ");        //Text für Zeile 1 des LCD Displays
                strcpy(TextZeile[1], firstKey + "                  ");                     //Text für Zeile 2 des LCD Displays
                strcpy(TextZeile[2], "                     ");      //Text für Zeile 3 des LCD Displays
                strcpy(TextZeile[3], "                     ");      //Text für Zeile 4 des LCD Displays

             } else {   // Zwei Zahlen nacheinander wurden gedrückt
                
                key_number = firstKey * 10 + key; 

                // Text für LCD Display
                strcpy(TextZeile[0], "Suche Schlüssel:   ");        //Text für Zeile 1 des LCD Displays
                strcpy(TextZeile[1], key_number + "                   ");                     //Text für Zeile 2 des LCD Displays
                strcpy(TextZeile[2], "                     ");      //Text für Zeile 3 des LCD Displays
                strcpy(TextZeile[3], "                     ");      //Text für Zeile 4 des LCD Displays
                
                firstKey = '\0'; // Setze firstKey zurück, um auf die nächste Zahlenkombination zu warten

            }
        }
    }

    // Überprüfe, ob die Zeitgrenze überschritten wurde. Wenn ja, setze die erste Ziffer zurück
    if (firstKey != '\0' && currentTime - lastKeyPressTime >= resetTimeout) {
        firstKey = '\0'; // Setze die erste Ziffer zurück
    }

    return key_number; // Gib die Zahlenkombination zurück

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


///////////////////////////////////////////////////////////////////////////////////////////////
// Zustandsfunktionen:
void inactive() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 255;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = true;

    open_door_lock();                   // Türschloss öffnen

}

void ready() {
    
    //Farbeinstellung der Status-LED
    StatusLED_red_var = 255;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = false;

    close_door_lock();                                  // Türschloss schließen
    
    key_number_var = keypad_readout();                  // Auslesen des Keypads
    if(key_number_var != 0){                            // Wenn eine Zahlenkombination eingegeben wurde wechsle in guest_key_search
        state = guest_key_search;
    }

    // Überprüfe ob ein RFID-Tag präsentiert wird
   if (isRfidPresented()) {
        long rfidId = getRfidId();
        if(isRfidKey(rfidId)){
                
        } else if(isRfidEmployee(rfidId)) {

        } else {
        // prinz a warning for a wrong RFID device
        }
              
    }

    // Text für LCD Display
    strcpy(TextZeile[0], "Scannen Sie Ihren   "); //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "RFID-Chip oder      "); //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "einen               "); //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "Schlüsselanhänger.");   //Text für Zeile 4 des LCD Displays

}

void logged_in() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 0;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 255;
    should_StatusLED_blink = false;

    open_door_lock();                   // Türschloss öffnen

    // Text für LCD Display
    strcpy(TextZeile[0], "Schlüssel entnehmen");    //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "oder zur Rückgabe  ");    //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "Schlüsselband      ");    //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "einscannen.         ");   //Text für Zeile 4 des LCD Displays
    
}
   
void logged_in_key_return() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 0;
    StatusLED_green_var = 255;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = false;

    // Text für LCD Display
    strcpy(TextZeile[0], "Stecken Sie         ");   //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "den gescannten      ");   //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "Schlüssel zurück. ");     //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "                    ");   //Text für Zeile 4 des LCD Displays

}
            
void guest_key_return() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 0;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 255;
    should_StatusLED_blink = true;

    open_door_lock();                   // Türschloss öffnen

    // Text für LCD Display
    strcpy(TextZeile[0], "Stecken Sie         ");   //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "den gescannten      ");   //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "Schlüssel zurück. ");     //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "                    ");   //Text für Zeile 4 des LCD Displays

}
            
void guest_waiting() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 0;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 255;
    should_StatusLED_blink = false;

    // Text für LCD Display
    strcpy(TextZeile[0], "Vorgang             ");   //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "abgeschlossen. Neuen");   //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "Schlüssel eincannen");    //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "o. Tür schließen. ");     //Text für Zeile 4 des LCD Displays

}

void wrong_key_exchange() {
 
    //Farbeinstellung der Status-LED
    StatusLED_red_var = 255;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = true;

    open_door_lock();                   // Türschloss öffnen

    // Text für LCD Display
    strcpy(TextZeile[0], "Tauschen Sie den    ");    //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "Schlüssel der roten");     //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "LED mir dem einge-  ");    //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "scannten Schlüssel.");     //Text für Zeile 4 des LCD Displays

}

void waiting_for_wrong_key() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 255;
    StatusLED_green_var = 255;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = true;

    // Text für LCD Display
    strcpy(TextZeile[0], "Tauschen Sie den    ");    //Text für Zeile 1 des LCD Displays
    strcpy(TextZeile[1], "Schlüssel der roten");     //Text für Zeile 2 des LCD Displays
    strcpy(TextZeile[2], "LED mir dem einge-  ");    //Text für Zeile 3 des LCD Displays
    strcpy(TextZeile[3], "scannten Schlüssel.");     //Text für Zeile 4 des LCD Displays    

}

void logged_in_key_search() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 0;
    StatusLED_green_var = 255;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = false;

}

void guest_key_search() {

    //Farbeinstellung der Status-LED
    StatusLED_red_var = 255;
    StatusLED_green_var = 0;
    StatusLED_blue_var = 0;
    should_StatusLED_blink = false;

}