#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <fstream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>

const int MAX_KEY_LEN = 65;

struct Key {
    char str[MAX_KEY_LEN];
    int value;

    Key() : value(0) {
        memset(str, 0, MAX_KEY_LEN);
    }

    Key(const char* s, int v) : value(v) {
        memset(str, 0, MAX_KEY_LEN);
        strncpy(str, s, MAX_KEY_LEN - 1);
    }

    bool operator<(const Key& other) const {
        int cmp = strcmp(str, other.str);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }

    bool operator==(const Key& other) const {
        return strcmp(str, other.str) == 0 && value == other.value;
    }

    bool keyEqual(const char* s) const {
        return strcmp(str, s) == 0;
    }
};

// Use std::map for simple and correct implementation
class BPTree {
private:
    std::string filename;
    std::map<Key, bool> data;
    bool loaded;

    void loadFromFile() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count;
        file.read((char*)&count, sizeof(int));

        for (int i = 0; i < count; i++) {
            Key key;
            file.read((char*)&key, sizeof(Key));
            data[key] = true;
        }
        file.close();
        loaded = true;
    }

    void saveToFile() {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return;

        int count = data.size();
        file.write((char*)&count, sizeof(int));

        for (auto& pair : data) {
            file.write((char*)&pair.first, sizeof(Key));
        }
        file.close();
    }

public:
    BPTree(const std::string& fname) : filename(fname), loaded(false) {
        loadFromFile();
    }

    ~BPTree() {
        saveToFile();
    }

    void insert(const char* index, int value) {
        Key key(index, value);
        data[key] = true;
    }

    void find(const char* index, std::vector<int>& result) {
        result.clear();

        for (auto& pair : data) {
            if (pair.first.keyEqual(index)) {
                result.push_back(pair.first.value);
            }
        }
    }

    void remove(const char* index, int value) {
        Key key(index, value);
        data.erase(key);
    }
};

#endif
