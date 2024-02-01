#include <Arduino.h>

/**
 * @brief Finds the index of the first not-equal element in two boolean arrays.
 *
 * Compares boolean arrays and returns the index of the first difference.
 * If arrays are equal, it returns -1.
 *
 * @param array1 Pointer to the first boolean array.
 * @param array2 Pointer to the second boolean array.
 * @return Index of the first not-equal element, or -1 if arrays are equal.
 */
int getNotEqualIndex(bool* array1, bool* array2) {
    for (int i = 0; i < sizeof(&array1); i++) {
        if (array1[i] != array2[i]) {
            return i;
        }
    }
    return -1;
};

boolean equals(boolean* array1, boolean* array2) {
    for (int i = 0; i < sizeof(&array1); i++) {
        if (array1[i] != array2[i]) {
            return false;
        }
    }
    return true;
};
