template <typename K, typename V>
class SimpleMap {  // # include <map> from STL
   private:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node* head;

   public:
    SimpleMap() : head(nullptr) {}

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
        Node* newNode = new Node(key, value);
        newNode->next = head;
        head = newNode;
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
    }

    void clear() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
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
        size_t count = 0;
        Node* current = head;
        while (current != nullptr) {
            count++;
            current = current->next;
        }
        return count;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    void print() const {
        Node* current = head;
        while (current != nullptr) {
            Serial.print(current->key);
            Serial.print(" -> ");
            Serial.println(current->value);
            current = current->next;
        }
    }

    // Function to get a vector of all keys in the map
    std::vector<K> getKeys() const {
        std::vector<K> keys;
        Node* current = head;
        while (current != nullptr) {
            keys.push_back(current->key);
            current = current->next;
        }
        return keys;
    }

    // Function to get a vector of all values in the map
    std::vector<V> getValues() const {
        std::vector<V> values;
        Node* current = head;
        while (current != nullptr) {
            values.push_back(current->value);
            current = current->next;
        }
        return values;
    }

    // Function to update the value associated with a key
    void update(const K& key, const V& value) {
        Node* current = head;
        while (current != nullptr) {
            if (current->key == key) {
                current->value = value;
                return;
            }
            current = current->next;
        }
    }

    // Function to copy the contents of another SimpleMap into this map
    void merge(const SimpleMap<K, V>& otherMap) {
        Node* otherCurrent = otherMap.head;
        while (otherCurrent != nullptr) {
            insert(otherCurrent->key, otherCurrent->value);
            otherCurrent = otherCurrent->next;
        }
    }

    // Function to remove all nodes with a specific value
    void removeAllWithValue(const V& value) {
        Node* current = head;
        Node* prev = nullptr;

        while (current != nullptr) {
            if (current->value == value) {
                if (prev == nullptr) {
                    head = current->next;
                } else {
                    prev->next = current->next;
                }
                Node* temp = current;
                current = current->next;
                delete temp;
            } else {
                prev = current;
                current = current->next;
            }
        }
    }
};
