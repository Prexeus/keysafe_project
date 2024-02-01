#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "SimpleFunctions.h"
#include "SimpleMap.h"
#include "SimpleQueue.h"

static const int keySlotCount = 50;

struct EmployeeData {
    const char* name;
    boolean employeeKeyPermissions[keySlotCount];
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

    /**
     * @brief Splits the given string into rows and returns a queue of row strings.
     *
     * @param string The string to be split into rows
     * @return SimpleQueue<const char*, maxRowCount> A queue of row strings
     */
    SimpleQueue<const char*, maxRowCount> getRowQueue(char string[]) {
        SimpleQueue<const char*, maxRowCount> resultQueue;
        const char* rowString = strtok(string, "\n");
        while (rowString != NULL) {
            resultQueue.push(rowString);
            rowString = strtok(NULL, "\n");
        }
        return resultQueue;
    }

    /**
     * @brief Inserts key data into the key map.
     *
     * @param rowString The row string containing key data
     */
    void insertInKeyMap(const char* rowString) {
        const char* keyId = strtok((char*)rowString, ";");
        const char* keyNumber = strtok(NULL, ";");  // TODO check if const char* keyNumber = strtok((char*)rowString, ";");
        keyMap.insert(atoi(keyId), atoi(keyNumber));
    }

    /**
     * @brief Inserts employee data into the employee map.
     *
     * @param rowString The row string containing employee data
     */
    void insertInEmployeeMap(const char* rowString) {
        const char* employeeId = strtok((char*)rowString, ";");
        const char* employeeName = strtok(NULL, ";");
        boolean employeeKeyPermissions[keySlotCount];
        for (int i = 0; i < keySlotCount; i++) {
            employeeKeyPermissions[i] = atoi(strtok(NULL, ";"));
        }
        EmployeeData employeeData = {
            .name = employeeName,
            .employeeKeyPermissions = employeeKeyPermissions};
        employeeMap.insert(atoi(employeeId), employeeData);
    }

    /**
     * @brief Reads the content of the SD card file and returns it as a string.
     *
     * @param fileName The name of the file to read
     * @return char* The content of the file as a string
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
        }
    }

    // TODO: Uncomment and complete the implementation
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
        keyMap.print();
    }

    /**
     * @brief Prints the content of the employee map.
     */
    void printEmployeeMap() {
        employeeMap.print();
    }

    /**
     * @brief Checks if the given id is a registered key.
     *
     * @param id The id to check
     * @return true if the id is a registered key
     */
    boolean isIdKey(long id) {
        return keyMap.containsKey(id);
    }

    /**
     * @brief Checks if the given id is a registered employee.
     *
     * @param id The id to check
     * @return true if the id is a registered employee
     */
    boolean isIdEmployee(long id) {
        return employeeMap.containsKey(id);
    }

    /**
     * @brief Returns the keyNumber of the given keyId.
     *
     * @param id The keyId
     * @return int The keyNumber
     */
    int getKeyNumber(long id) {
        return keyMap.get(id);
    }

    /**
     * @brief Returns the name of the given employeeId.
     *
     * @param id The employeeId
     * @return const char* The employeeName
     */
    const char* getEmployeeName(long id) {
        return employeeMap.get(id).name;
    }

    /**
     * @brief Returns the keyPermissionArray of the given employeeId.
     *
     * @param id The employeeId
     * @return boolean* The keyPermissionArray
     */
    boolean* getEmployeeKeyPermissions(long id) {
        return employeeMap.get(id).employeeKeyPermissions;
    }

    /**
     * @brief Logs the changes made to key lending and returns.
     *
     * @param unloggedChanges A queue of unlogged key changes
     * @param keyLendingArray An array containing information about key lending
     * //TODO @param
     */
    void logChanges(SimpleQueue<int, 30> unloggedChanges, long* keyLendingArray /*, //TODO Uhrzeit*/) {
        if (!unloggedChanges.isEmpty()) {
            File protocol = SD.open("protocol.csv");  // allows writing and reading
            if (protocol) {
                do {
                    int keyNumber = unloggedChanges.pop();
                    long employeeId = keyLendingArray[keyNumber];
                    if (employeeId != 0) {  // TODO test writing
                        protocol.println(
                            /*TODO timer +*/ ";" + String(keyNumber) + ";" + employeeMap.get(employeeId).name + ";" + "lending");
                    } else {
                        protocol.println(
                            /*TODO timer +*/ ";" + String(keyNumber) + ";" + ";" + "return");
                    }
                } while (!unloggedChanges.isEmpty());
            } else {
                Serial.println("Couldn't write protocol.csv!");
            }
            protocol.close();
        }
    }
};
