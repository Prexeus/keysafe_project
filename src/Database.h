#include <Arduino.h>
#include <SD.h>
#include <string.h>

#include "SimpleFunctions.h"
#include "SimpleMap.h"
#include "SimpleQueue.h"

struct EmployeeData {
    String name;
    boolean keyNumber1;
    boolean keyNumber2;
    boolean keyNumber3;
    boolean keyNumber4;
    boolean keyNumber5;
    boolean keyNumber6;
    boolean keyNumber7;
    boolean keyNumber8;
    boolean keyNumber9;
};

struct UnloggedKeyChange {
    long keyId;
    boolean isPresent;
    long employeeId;
};

class Database {
   private:
    static const int maxRowCount = 70;
    SimpleMap<long, int> keyMap;
    SimpleMap<long, EmployeeData> employeeMap;

    SimpleQueue<const char*, maxRowCount> getRowQueue(char string[]) {
        SimpleQueue<const char*, maxRowCount> resultQueue;
        const char* rowString = strtok(string, "\n");
        while (rowString != NULL) {
            resultQueue.push(rowString);
            rowString = strtok(NULL, "\n");
        }
        return resultQueue;
    }

    void insertInKeyMap(const char* rowString) {
        const char* keyId = strtok((char*)rowString, ";");
        const char* keyNumber = strtok((char*)rowString, ";");
        keyMap.insert(atoi(keyId), atoi(keyNumber));
    }

    void insertInEmplyeeMap(const char* rowString) {
        const char* employeeId = strtok((char*)rowString, ";");
        const char* employeeName = strtok(NULL, ";");
        const char* key1 = strtok(NULL, ";");
        const char* key2 = strtok(NULL, ";");
        const char* key3 = strtok(NULL, ";");
        const char* key4 = strtok(NULL, ";");
        const char* key5 = strtok(NULL, ";");
        const char* key6 = strtok(NULL, ";");
        const char* key7 = strtok(NULL, ";");
        const char* key8 = strtok(NULL, ";");
        const char* key9 = strtok(NULL, ";");
        EmployeeData employeeData = {
            .name = employeeName,
            .keyNumber1 = atoi(key1),
            .keyNumber2 = atoi(key2),
            .keyNumber3 = atoi(key3),
            .keyNumber4 = atoi(key4),
            .keyNumber5 = atoi(key5),
            .keyNumber6 = atoi(key6),
            .keyNumber7 = atoi(key7),
            .keyNumber8 = atoi(key8),
            .keyNumber9 = atoi(key9),
        };
        employeeMap.insert(atoi(employeeId), employeeData);
    }

    char* getSdString(const char* fileName) {
        File file = SD.open(fileName);
        if (file) {
            int fileSize = file.size();
            char* fileContent = new char[fileSize + 1];  // +1 for the null terminator of the fileContent string
            file.read(fileContent, fileSize);
            fileContent[fileSize] = '\0';  // Add null terminator to the end of the string
            file.close();
            return fileContent;
        }
    }

   public:
    Database(SDClass sdCard) {
        char* keyInputString = getSdString("keyData.csv");
        SimpleQueue<const char*, maxRowCount> keyQueue = getRowQueue(keyInputString);
        while (!keyQueue.isEmpty()) {
            insertInKeyMap(keyQueue.pop());
        }
        delete[] keyInputString;

        char* employeeInputString = getSdString("employeeData.csv");
        SimpleQueue<const char*, maxRowCount> employeeQueue = getRowQueue(employeeInputString);
        while (!employeeQueue.isEmpty()) {
            insertInEmplyeeMap(employeeQueue.pop());
        }
        delete[] employeeInputString;
    }

    void printKeyMap() {
        keyMap.print();
    }

    void printEmployeeMap() {
        employeeMap.print();
    }

    boolean isIdKey(long id) {
        return keyMap.containsKey(id);
    }

    boolean isIdEmployee(long id) {
        return employeeMap.containsKey(id);
    }

    int getKeyNumber(long id) {
        return keyMap.get(id);
    }

    String getEmployeeName(long id) {
        return employeeMap.get(id).name;
    }

    void logChanges(SimpleQueue<int, 10>* unloggedChanges, long* keyLendingArray) {
        
    }
};

