#include <Arduino.h>
#include <string.h>

#include "SimpleMap.h"
#include "SimpleQueue.h"

const int maxKeyCount = 50;

struct KeyData {
    unsigned int keyNumber;
};

struct EmployeeData {
    unsigned int keyNumber;
    unsigned int keyNumber2;
};

SimpleQueue<const char*, maxKeyCount> getRowQueue(char string[]) {
    SimpleQueue<const char*, maxKeyCount> resultQueue;
    const char* rowString = strtok(string, "\n");
    while (rowString != NULL) {
        resultQueue.push(rowString);
        rowString = strtok(NULL, "\n");
    }
    return resultQueue;
}

SimpleMap<unsigned int, KeyData> addToKeyMap(const char* rowString) {
    const char* cellString = strtok((char*)rowString, ";");
    SimpleMap<unsigned int, KeyData> map;
    while (cellString != NULL) {
        Serial.println(cellString);
        cellString = strtok(NULL, ";");
    }
    return map;
}

SimpleMap<unsigned int, KeyData> addToEmplyeeMap(const char* rowString) {
    const char* cellString = strtok((char*)rowString, ";");
    SimpleMap<unsigned int, EmployeeData> map;
    while (cellString != NULL) {
        Serial.println(cellString);
        cellString = strtok(NULL, ";");
    }
//    return map;
}


void setup() {
    Serial.begin(9600);

    char inputString[] = "1;2;3\n4;5;6\n7;8;9";

    SimpleQueue<const char*, maxKeyCount> keyQueue = getRowQueue(inputString);

    while (!keyQueue.isEmpty()) {
        addToKeyMap(keyQueue.pop());
    }
}

void loop() {
    // Hier kann Ihr Hauptprogrammcode stehen
}
