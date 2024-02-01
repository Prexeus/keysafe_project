template <typename K, typename V, size_t Capacity>
#include <Arduino.h>
#include <String.h>
class SimpleMap {
   private:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node* head;
    size_t count;

   public:
    SimpleMap() : head(nullptr), count(0) {}

    ~SimpleMap() {
        clear();
    }

    V get(const K& key) const {
        Node* current = head;
        while (current != nullptr) {
            if (current->key == key) {
                return current->value;
            }
            current = current->next;
        }
        // Return a default-constructed V if the key is not found
        return V();
    }

    void insert(const K& key, const V& value) {
        if (count >= Capacity) {
            // Capacity exceeded
            return;
        }

        Node* newNode = new Node(key, value);
        newNode->next = head;
        head = newNode;
        count++;
    }

    bool find(const K& key, V& value) const {
        Node* current = head;
        while (current != nullptr) {
            if (current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next;
        }
        return false;
    }

    void remove(const K& key) {
        Node* current = head;
        Node* prev = nullptr;

        while (current != nullptr && current->key != key) {
            prev = current;
            current = current->next;
        }

        if (current == nullptr) {
            return;  // Key not found
        }

        if (prev == nullptr) {
            head = current->next;
        } else {
            prev->next = current->next;
        }

        delete current;
        count--;
    }

    void clear() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        count = 0;
    }

    bool containsKey(const K& key) const {
        Node* current = head;
        while (current != nullptr) {
            if (current->key == key) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    bool containsValue(const V& value) const {
        Node* current = head;
        while (current != nullptr) {
            if (current->value == value) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    size_t size() const {
        return count;
    }

    bool isEmpty() const {
        return count == 0;
    }
};
