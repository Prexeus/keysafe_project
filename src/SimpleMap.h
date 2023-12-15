template <typename K, typename V>
class SimpleMap { // # include <map> from STL
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
    
};
