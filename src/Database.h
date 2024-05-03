/**
 * @file Database.h
 * @brief Definition of the Database class for managing key and employee data.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "SimpleMap.h"
#include "SimpleQueue.h"

static const int keySlotCount = 50;
static const int maxEmployeeCount = 60;

/**
 * @brief Structure to represent employee data.
 */
struct EmployeeData {
    const char* name;
    boolean employeeKeyPermissions[keySlotCount];
};

/**
 * @brief Structure to represent unlogged changes to keys.
 */
struct UnloggedKeyChange {
    long keyId;        /**< The ID of the key. */
    boolean isPresent; /**< Indicates whether the key is present. */
    long employeeId;   /**< The ID of the employee. */
};

/**
 * @brief Class for managing key and employee data, aswell as SD-card handling.
 */
class Database {
   private:
    static const int maxRowCount = 70;
    SimpleMap<long, int, keySlotCount> keyMap;                   /**< Map for keys. */
    SimpleMap<long, EmployeeData, maxEmployeeCount> employeeMap; /**< Map for employee data. */

    /**
     * @brief Splits the given string into rows and returns a queue of row strings.
     *
     * @param string The string to be split into rows.
     * @param skipHeader Flag to skip the first row (header) of the CSV.
     * @return SimpleQueue<const char*, maxRowCount> A queue of row strings.
     */
    SimpleQueue<const char*, maxRowCount> getRowQueue(char string[], bool skipHeader = true) {
        SimpleQueue<const char*, maxRowCount> resultQueue;
        const char* rowString = strtok(string, "\n");

        // Skip header if specified
        if (skipHeader) {
            rowString = strtok(NULL, "\n");
        }

        while (rowString != NULL) {
            resultQueue.push(rowString);
            rowString = strtok(NULL, "\n");
        }
        return resultQueue;
    }

    /**
     * @brief Inserts key data into the key map.
     *
     * @param rowString The row string containing key data.
     */
    void insertInKeyMap(const char* rowString) {
        const char* keyId = strtok((char*)rowString, ";");
        const char* keyNumber = strtok(NULL, ";");
        keyMap.insert(atoi(keyId), atoi(keyNumber));
    }

    /**
     * @brief Inserts employee data into the employee map.
     *
     * @param rowString The row string containing employee data.
     */
    void insertInEmployeeMap(const char* rowString) {
        const char* employeeId = strtok((char*)rowString, ";");
        const char* employeeName = strtok(NULL, ";");
        boolean employeeKeyPermissions[keySlotCount];
        for (int i = 0; i < keySlotCount; i++) {
            const char* permissionStr = strtok(NULL, ";");
            employeeKeyPermissions[i] = atoi(permissionStr);
        }
        EmployeeData employeeData = {
            .name = employeeName,
            .employeeKeyPermissions = {0}};
        memcpy(employeeData.employeeKeyPermissions, employeeKeyPermissions, keySlotCount);
        employeeMap.insert(atoi(employeeId), employeeData);
    }

    /**
     * @brief Reads the content of the SD card file and returns it as a string.
     *
     * @param fileName The name of the file to read.
     * @return char* The content of the file as a string or an empty char array if the file couldn't be opened.
     */
    char* getSdString(const char* fileName) {
        File file = SD.open(fileName);
        if (file) {
            int fileSize = file.size();
            char* fileContent = new char[fileSize + 1];  // +1 for the null terminator of the fileContent string
            file.read(fileContent, fileSize);
            fileContent[fileSize] = '\0';  // Add null terminator to the end of the string
            file.close();
            return fileContent;
        } else {
            return new char[1]{0};  // Return empty char array
        }
    }

    // TODO Uncomment and complete the implementation
    /*
    String getFormattedTime(DateTime now) {
        return String(now.minute(), DEC) + ":" +
               String(now.hour(), DEC) + ", " +
               String(now.day(), DEC) + "." +
               String(now.month(), DEC) + "." +
               String(now.year(), DEC) % 100;  // Show only the last two digits of the year
    }
    */

   public:
    /**
     * @brief Constructor for the Database class.
     *
     * @param sdCard The SD card object for data source.
     */
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
            insertInEmployeeMap(employeeQueue.pop());
        }
        delete[] employeeInputString;
    }

    /**
     * @brief Prints the content of the key map.
     */
    void printKeyMap() {
        Serial.println("Key map:");
        for (int i = 0; i < keySlotCount; i++) {
            Serial.println("Key number: " + String(i) + ", key id: " + String(keyMap.get(i)));
        }
    }

    /**
     * @brief Prints the content of the employee map.
     */
    void printEmployeeMap() {
        Serial.println("Employee map:");
        for (int i = 0; i < maxEmployeeCount; i++) {
            Serial.println("Employee id: " + String(i) + ", employee name: " + String(employeeMap.get(i).name));
        }
    }

    /**
     * @brief Checks if the given id is a registered key.
     *
     * @param id The id to check.
     * @return true if the id is a registered key.
     */
    boolean isIdKey(long id) {
        return keyMap.containsKey(id);
    }

    /**
     * @brief Checks if the given id is a registered employee.
     *
     * @param id The id to check.
     * @return true if the id is a registered employee.
     */
    boolean isIdEmployee(long id) {
        return employeeMap.containsKey(id);
    }

    /**
     * @brief Returns the keyNumber of the given keyId.
     *
     * @param id The keyId.
     * @return int The keyNumber.
     */
    int getKeyNumber(long id) {
        return keyMap.get(id);
    }

    /**
     * @brief Returns the name of the given employeeId.
     *
     * @param id The employeeId.
     * @return const char* The employeeName.
     */
    const char* getEmployeeName(long id) {
        return employeeMap.get(id).name;
    }

    /**
     * @brief Returns the keyPermissionArray of the given employeeId.
     *
     * @param id The employeeId.
     * @return boolean* The keyPermissionArray.
     */
    boolean* getEmployeeKeyPermissions(long id) {
        return employeeMap.get(id).employeeKeyPermissions;
    }

    /**
     * @brief Logs the changes made to key lending and returns.
     *
     * @param unloggedChanges A queue of unlogged key changes.
     * @param keyLendingArray An array containing information about key lending.
     * //TODO @param
     */
    void logChanges(SimpleQueue<int, 30> unloggedChanges, long* keyLendingArray /*, //TODO Uhrzeit*/) {
        if (!unloggedChanges.isEmpty()) {
            File protocol = SD.open("protocol.csv");  // allows writing and reading
            if (protocol) {
                protocol.seek(protocol.size());
                do {
                    int keyNumber = unloggedChanges.pop();
                    long employeeId = keyLendingArray[keyNumber];
                    if (employeeId != 0) {
                        protocol.println(
                            /*TODO timer +*/ ";" + String(keyNumber) + ";" + employeeMap.get(employeeId).name + ";" + "lending");
                    } else {
                        protocol.println(
                            /*TODO timer +*/ ";" + String(keyNumber) + ";" + ";" + "return");
                    }
                } while (!unloggedChanges.isEmpty());
                protocol.close();
            } else {
                Serial.println("Couldn't write protocol.csv!");
            }
        }
    }

    /**
     * @brief Adds a new teached RFID to the teachedRfid.csv file.
     * 
     * @param rfidId The ID of the teached RFID.
     */
    void addNewTeachedRfid(long rfidId) {
        File file = SD.open("teachedRfid.csv", FILE_WRITE);
        if (file) {
            file.seek(file.size());
            file.print("\n");
            file.println(/*TODO timer +*/ ";" + rfidId);
            file.close();
        } else {
            Serial.println("Couldn't write keyData.csv!");
        }
    }
};

#endif  // DATABASE_H