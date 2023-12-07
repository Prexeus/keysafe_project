#include <MFRC522.h>
#include <SPI.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    long code = 0;  // Als neue Variable fügen wir „code“ hinzu, unter welcher später die UID als zusammenhängende Zahl ausgegeben wird. Statt int benutzen wir jetzt den Zahlenbereich „long“, weil sich dann eine größere Zahl speichern lässt.

    for (byte i = 0; i < mfrc522.uid.size; i++) {
        code = ((code + mfrc522.uid.uidByte[i]) * 10);  // Nun werden wie auch vorher die vier Blöcke ausgelesen und in jedem Durchlauf wird der Code mit dem Faktor 10 „gestreckt“. (Eigentlich müsste man hier den Wert 1000 verwenden, jedoch würde die Zahl dann zu groß werden.
    }

    Serial.print("Die Kartennummer lautet:");  // Zum Schluss wird der Zahlencode (Man kann ihn nicht mehr als UID bezeichnen) ausgegeben.
    Serial.println(code);
}