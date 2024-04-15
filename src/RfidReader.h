#include <PN5180.h>
#include <PN5180ISO15693.h>

/**
 * @brief RFIDReader class for interfacing with an RFID reader.
 */
class RfidReader {
private:
    PN5180ISO15693 nfc; /**< Instance of the PN5180ISO15693 RFID reader. */
    uint8_t lastTagIdArray[8]; /**< Last RFID tag ID read by the reader. */

public:
    /**
     * @brief Constructor for the RFIDReader class.
     *
     * @param ssPin The slave select pin connected to the RFID reader.
     * @param busyPin The busy pin connected to the RFID reader.
     * @param resetPin The reset pin connected to the RFID reader.
     */
    RfidReader(uint8_t ssPin, uint8_t busyPin, uint8_t resetPin) : nfc(ssPin, busyPin, resetPin) {
        nfc.begin();
        nfc.reset();
        nfc.setupRF();
    }

    /**
     * @brief Checks if an RFID tag is present and updates the last RFID tag ID.
     *
     * @return bool True if an RFID tag is present, false otherwise.
     */
    bool isTagPresented() {
        // TODO ggf. könnte ein regelmäßiger Aufruf von reset(); imd setupRF(); notwendig sein [bei störungen]
        bool present = nfc.getInventory(lastTagIdArray);
        return present;
    }

    /**
     * @brief Converts the last read RFID tag ID from uint8_t* to long.
     *
     * @return long The last read RFID tag ID converted to a long.
     */
    long getLastReadId() {
        long convertedId = 0;
        for (int i = 0; i < 8; i++) {
            convertedId <<= 8; // shift left by 8 bits
            convertedId |= lastTagIdArray[i]; // add current byte to convertedId
        }
        return convertedId;
    }
};
