#include <Arduino.h>
#include <string.h>

class DataBase {
   public:
    void csvStringToMap(const char* csvString) {
        char columnDelimiter = ';';  // Spaltentrennzeichen
        char rowDelimiter = '\n';    // Zeilentrennzeichen

        // Überprüfe auf NULL-Pointer
        if (csvString == nullptr) {
            // Hier könnten auch entsprechende Fehlerbehandlungsmaßnahmen implementiert werden
            return;
        }

        // Kopiere den CSV-String, um ihn zu bearbeiten
        char* csvCopy = strdup(csvString);

        // Zähle die Anzahl der Zeilen und Spalten
        numRows = countTokens(csvCopy, rowDelimiter) + 1;               // Eine Zeile mehr als die Anzahl der Zeilentrennzeichen
        numCols = countTokens(csvCopy, columnDelimiter) / numRows + 1;  // Eine Spalte mehr als die Anzahl der Spaltentrennzeichen pro Zeile

        // Allokiere Speicher für die Datenmap
        dataMap = new unsigned int*[numRows];
        for (size_t i = 0; i < numRows; ++i) {
            dataMap[i] = new unsigned int[numCols];
        }

        // Tokenisiere den CSV-String nach Zeilen
        char* lineToken = strtok(csvCopy, &rowDelimiter);
        size_t rowIndex = 0;
        while (lineToken != nullptr) {
            // Tokenisiere jede Zeile nach Spalten mit dem angegebenen Trennzeichen
            char* token = strtok(lineToken, &columnDelimiter);

            // Extrahiere die Werte und speichere sie in der Datenmap
            size_t colIndex = 0;
            while (token != nullptr) {
                dataMap[rowIndex][colIndex] = atoi(token);

                // Bewege den Zeiger auf das nächste Token
                token = strtok(nullptr, &columnDelimiter);

                // Inkrementiere den Spaltenindex für die Datenmap
                colIndex++;
            }

            // Bewege den Zeiger auf die nächste Zeile
            lineToken = strtok(nullptr, &rowDelimiter);

            // Inkrementiere den Zeilenindex für die Datenmap
            rowIndex++;
        }

        // Befreie den kopierten String, um Speicherlecks zu vermeiden
        free(csvCopy);
    }

    void printDataMap() {
        for (size_t i = 0; i < numRows; ++i) {
            Serial.print("Key: ");
            Serial.println(dataMap[i][0]);  // Der erste Wert in jeder Zeile wird als Key betrachtet

            Serial.print("Values: [ ");
            for (size_t j = 1; j < numCols; ++j) {  // Die folgenden Werte in jeder Zeile werden als Values betrachtet
                Serial.print(dataMap[i][j]);
                Serial.print(" ");
            }
            Serial.println("]");
        }
    }

   private:
    size_t countTokens(char* str, char delimiter) {
        size_t count = 0;
        char* token = strtok(str, &delimiter);
        while (token != nullptr) {
            count++;
            token = strtok(nullptr, &delimiter);
        }
        return count;
    }

    unsigned int** dataMap;
    size_t numRows, numCols;
};

void setup() {
    Serial.begin(9600);

    const char* csvString = "123;456;789\n1011;1213;1415\n1617;1819;2021";

    // Erstelle eine Instanz der DataBase-Klasse
    DataBase myDatabase;

    // Rufe die Funktion auf, um die Datenmap zu erstellen
    myDatabase.csvStringToMap(csvString);

    // Ausgabe der resultierenden Datenmap
    myDatabase.printDataMap();
}

void loop() {
    // Der Loop wird hier nicht benötigt, da dies ein einfaches Beispiel ist.
}
