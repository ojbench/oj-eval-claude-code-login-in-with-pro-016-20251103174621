#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <fstream>
#include <cstring>
#include <algorithm>
#include <vector>

const int MAX_KEY_LEN = 65;
const int ORDER = 150;  // B+ Tree order

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

    bool operator<=(const Key& other) const {
        return *this < other || *this == other;
    }

    bool keyEqual(const char* s) const {
        return strcmp(str, s) == 0;
    }
};

struct Node {
    bool is_leaf;
    int num_keys;
    Key keys[ORDER - 1];
    int children[ORDER];  // File offsets or -1
    int next;  // For leaf nodes, points to next leaf

    Node() : is_leaf(true), num_keys(0), next(-1) {
        for (int i = 0; i < ORDER; i++) {
            children[i] = -1;
        }
    }
};

class BPTree {
private:
    std::fstream file;
    std::string filename;
    int root_offset;
    int node_count;

    void writeHeader() {
        file.seekp(0);
        file.write((char*)&root_offset, sizeof(int));
        file.write((char*)&node_count, sizeof(int));
    }

    void readHeader() {
        file.seekg(0);
        file.read((char*)&root_offset, sizeof(int));
        file.read((char*)&node_count, sizeof(int));
    }

    int allocNode() {
        int offset = sizeof(int) * 2 + node_count * sizeof(Node);
        node_count++;
        return offset;
    }

    void writeNode(int offset, const Node& node) {
        file.seekp(offset);
        file.write((char*)&node, sizeof(Node));
    }

    void readNode(int offset, Node& node) {
        file.seekg(offset);
        file.read((char*)&node, sizeof(Node));
    }

    int findLeaf(const Key& key) {
        int current = root_offset;
        Node node;

        while (true) {
            readNode(current, node);

            if (node.is_leaf) {
                return current;
            }

            int i = 0;
            while (i < node.num_keys && !(key < node.keys[i])) {
                i++;
            }
            current = node.children[i];
        }
    }

    void insertNonFull(int offset, const Key& key) {
        Node node;
        readNode(offset, node);

        if (node.is_leaf) {
            // Check for duplicate
            for (int i = 0; i < node.num_keys; i++) {
                if (node.keys[i] == key) {
                    return;
                }
            }

            int i = node.num_keys - 1;
            while (i >= 0 && node.keys[i] < key) {
                node.keys[i + 1] = node.keys[i];
                i--;
            }
            node.keys[i + 1] = key;
            node.num_keys++;
            writeNode(offset, node);
        } else {
            int i = node.num_keys - 1;
            while (i >= 0 && !(key < node.keys[i])) {
                i--;
            }
            i++;

            Node child;
            readNode(node.children[i], child);

            if (child.num_keys == ORDER - 1) {
                splitChild(offset, i);
                readNode(offset, node);
                if (!(key < node.keys[i])) {
                    i++;
                }
            }
            insertNonFull(node.children[i], key);
        }
    }

    void splitChild(int parent_offset, int index) {
        Node parent;
        readNode(parent_offset, parent);

        int child_offset = parent.children[index];
        Node child;
        readNode(child_offset, child);

        Node new_child;
        new_child.is_leaf = child.is_leaf;

        int mid = (ORDER - 1) / 2;

        if (child.is_leaf) {
            new_child.num_keys = ORDER - 1 - mid;
            for (int j = 0; j < new_child.num_keys; j++) {
                new_child.keys[j] = child.keys[mid + j];
            }
            new_child.next = child.next;
            child.num_keys = mid;
        } else {
            new_child.num_keys = ORDER - 1 - mid - 1;
            for (int j = 0; j < new_child.num_keys; j++) {
                new_child.keys[j] = child.keys[j + mid + 1];
            }
            for (int j = 0; j <= new_child.num_keys; j++) {
                new_child.children[j] = child.children[j + mid + 1];
            }
            child.num_keys = mid;
        }

        int new_child_offset = allocNode();

        if (child.is_leaf) {
            child.next = new_child_offset;
        }

        for (int j = parent.num_keys; j > index; j--) {
            parent.children[j + 1] = parent.children[j];
        }
        parent.children[index + 1] = new_child_offset;

        for (int j = parent.num_keys - 1; j >= index; j--) {
            parent.keys[j + 1] = parent.keys[j];
        }

        if (child.is_leaf) {
            parent.keys[index] = new_child.keys[0];
        } else {
            parent.keys[index] = child.keys[mid];
        }

        parent.num_keys++;

        writeNode(parent_offset, parent);
        writeNode(child_offset, child);
        writeNode(new_child_offset, new_child);
    }

public:
    BPTree(const std::string& fname) : filename(fname), root_offset(-1), node_count(0) {
        std::ifstream check(filename);
        bool exists = check.good();
        check.close();

        if (exists) {
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            readHeader();
        } else {
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

            Node root;
            root_offset = allocNode();
            writeNode(root_offset, root);
            writeHeader();
        }
    }

    ~BPTree() {
        if (file.is_open()) {
            writeHeader();
            file.close();
        }
    }

    void insert(const char* index, int value) {
        Key key(index, value);

        Node root;
        readNode(root_offset, root);

        if (root.num_keys == ORDER - 1) {
            Node new_root;
            new_root.is_leaf = false;
            new_root.num_keys = 0;

            int new_root_offset = allocNode();
            new_root.children[0] = root_offset;

            writeNode(new_root_offset, new_root);
            splitChild(new_root_offset, 0);

            root_offset = new_root_offset;
            writeHeader();
        }

        insertNonFull(root_offset, key);
    }

    void find(const char* index, std::vector<int>& result) {
        result.clear();

        Key search_key(index, 0);
        int leaf_offset = findLeaf(search_key);

        Node leaf;
        while (leaf_offset != -1) {
            readNode(leaf_offset, leaf);

            bool found = false;
            for (int i = 0; i < leaf.num_keys; i++) {
                if (leaf.keys[i].keyEqual(index)) {
                    result.push_back(leaf.keys[i].value);
                    found = true;
                } else if (found) {
                    std::sort(result.begin(), result.end());
                    return;
                }
            }

            if (!found && leaf.num_keys > 0) {
                if (strcmp(leaf.keys[0].str, index) > 0) {
                    break;
                }
            }

            leaf_offset = leaf.next;
        }

        std::sort(result.begin(), result.end());
    }

    void remove(const char* index, int value) {
        Key key(index, value);
        int leaf_offset = findLeaf(key);

        Node leaf;
        readNode(leaf_offset, leaf);

        for (int i = 0; i < leaf.num_keys; i++) {
            if (leaf.keys[i] == key) {
                for (int j = i; j < leaf.num_keys - 1; j++) {
                    leaf.keys[j] = leaf.keys[j + 1];
                }
                leaf.num_keys--;
                writeNode(leaf_offset, leaf);
                return;
            }
        }
    }
};

#endif
