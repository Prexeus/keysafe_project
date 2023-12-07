#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

// pin definitions:
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfidReader(SS_PIN, RST_PIN);

// function declarations:
int myFunction(int, int);

// static variables declarations:
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

// global variable declarations:
State state = TESTING;

// initialization code:
void setup() {
    Serial.begin(9600);

    SPI.begin();
    rfidReader.PCD_Init();
}

// state-machine loop:
void loop() {
    switch (state) {
        case INACTIVE:
            break;
        case TESTING:
            break;
        case READY:

            if (isRfidPresented()) {
              long rfidId = getRfidId();
              if(isRfidKey(rfidId)){
                
              } else if(isRfidEmployee(rfidId)) {

              } else {
                // prinz a warning for a wrong RFID device
              }
              
            }

            break;
        case LOGGED_IN:
            break;
        case LOGGED_IN_KEY_RETURN:
            break;
        case GUEST_KEY_RETURN:
            break;
        case GUEST_WAITING:
            break;
        case WRONG_KEY_EXCHANGE:
            break;
        case WAITING_FOR_WRONG_KEY:
            break;
        case LOGGED_IN_KEY_SEARCH:
            break;
        case GUEST_KEY_SEARCH:
            break;
        default:
            break;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// function definitions:


int myFunction(int x, int y) {
    return x + y;
}

// database functions



// RFID functions
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