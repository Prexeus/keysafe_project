#include <Arduino.h>

// Einfache Queue-Implementierung
template <typename T, size_t SIZE>
class SimpleQueue {  // # include <QueueList.h>
   public:
    SimpleQueue() : front(0), rear(0), itemCount(0) {}

    void push(const T& item) {
        if (itemCount < SIZE) {
            data[rear] = item;
            rear = (rear + 1) % SIZE;
            itemCount++;
        }
    }

    T pop() {
        if (itemCount > 0) {
            T item = data[front];
            front = (front + 1) % SIZE;
            itemCount--;
            return item;
        }
        return T();  // Return default-constructed item if the queue is empty
    }

    bool isEmpty() const {
        return itemCount == 0;
    }

    bool isFull() const {
        return itemCount == SIZE;
    }

    size_t size() const {
        return itemCount;
    }

    void print() {
        for (size_t i = 0; i < SIZE; ++i) {
            Serial.print(data[i]);
            Serial.print(" ");
        }
        Serial.println();
    }

   private:
    T data[SIZE];
    size_t front;
    size_t rear;
    size_t itemCount;
};
