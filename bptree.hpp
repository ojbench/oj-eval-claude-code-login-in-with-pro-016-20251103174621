#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <fstream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>
#include <climits>

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

class BPTree {
private:
    std::string filename;
    std::map<Key, bool> data;

    void loadFromFile() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count;
        if (!file.read((char*)&count, sizeof(int))) {
            file.close();
            return;
        }

        data.clear();
        Key key;
        for (int i = 0; i < count; i++) {
            if (file.read((char*)&key, sizeof(Key))) {
                data[key] = true;
            }
        }
        file.close();
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
    BPTree(const std::string& fname) : filename(fname) {
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
        result.reserve(100);  // Reserve space to avoid reallocation

        // Use lower_bound and upper_bound for efficient range query
        Key lower_key(index, INT_MIN);
        Key upper_key(index, INT_MAX);

        auto it = data.lower_bound(lower_key);
        auto end_it = data.upper_bound(upper_key);

        while (it != end_it) {
            if (it->first.keyEqual(index)) {
                result.push_back(it->first.value);
            }
            ++it;
        }
    }

    void remove(const char* index, int value) {
        Key key(index, value);
        data.erase(key);
    }
};

#endif
