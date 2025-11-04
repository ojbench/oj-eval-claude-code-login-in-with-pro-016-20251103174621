#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <fstream>
#include <cstring>
#include <algorithm>
#include <vector>

const int MAX_KEY_LEN = 65;
const int BLOCK_SIZE = 4096;

// B+ Tree parameters
const int M = 85;  // Max children per internal node
const int L = 85;  // Max entries per leaf node

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

struct LeafNode {
    int n;  // Number of keys
    Key keys[L];
    int next;  // Next leaf node offset (-1 if none)

    LeafNode() : n(0), next(-1) {}
};

struct InternalNode {
    int n;  // Number of keys
    Key keys[M - 1];
    int children[M];  // Child offsets

    InternalNode() : n(0) {
        for (int i = 0; i < M; i++) {
            children[i] = -1;
        }
    }
};

class BPTree {
private:
    std::fstream file;
    std::string filename;
    int root;
    int leaf_count;
    int internal_count;
    bool is_leaf_root;

    void writeHeader() {
        file.seekp(0);
        file.write((char*)&root, sizeof(int));
        file.write((char*)&leaf_count, sizeof(int));
        file.write((char*)&internal_count, sizeof(int));
        file.write((char*)&is_leaf_root, sizeof(bool));
    }

    void readHeader() {
        file.seekg(0);
        file.read((char*)&root, sizeof(int));
        file.read((char*)&leaf_count, sizeof(int));
        file.read((char*)&internal_count, sizeof(int));
        file.read((char*)&is_leaf_root, sizeof(bool));
    }

    int allocLeafNode() {
        int offset = sizeof(int) * 3 + sizeof(bool) +
                     internal_count * sizeof(InternalNode) +
                     leaf_count * sizeof(LeafNode);
        leaf_count++;
        return offset;
    }

    int allocInternalNode() {
        int offset = sizeof(int) * 3 + sizeof(bool) +
                     internal_count * sizeof(InternalNode);
        internal_count++;
        return offset;
    }

    void writeLeafNode(int offset, const LeafNode& node) {
        file.seekp(offset);
        file.write((char*)&node, sizeof(LeafNode));
    }

    void readLeafNode(int offset, LeafNode& node) {
        file.seekg(offset);
        file.read((char*)&node, sizeof(LeafNode));
    }

    void writeInternalNode(int offset, const InternalNode& node) {
        file.seekp(offset);
        file.write((char*)&node, sizeof(InternalNode));
    }

    void readInternalNode(int offset, InternalNode& node) {
        file.seekg(offset);
        file.read((char*)&node, sizeof(InternalNode));
    }

    void insertIntoLeaf(LeafNode& leaf, const Key& key) {
        int pos = 0;
        while (pos < leaf.n && leaf.keys[pos] < key) {
            pos++;
        }

        for (int i = leaf.n; i > pos; i--) {
            leaf.keys[i] = leaf.keys[i - 1];
        }
        leaf.keys[pos] = key;
        leaf.n++;
    }

    Key splitLeaf(int leaf_offset, LeafNode& leaf, int& new_leaf_offset) {
        new_leaf_offset = allocLeafNode();
        LeafNode new_leaf;

        int mid = L / 2;
        new_leaf.n = leaf.n - mid;
        for (int i = 0; i < new_leaf.n; i++) {
            new_leaf.keys[i] = leaf.keys[mid + i];
        }
        leaf.n = mid;

        new_leaf.next = leaf.next;
        leaf.next = new_leaf_offset;

        writeLeafNode(leaf_offset, leaf);
        writeLeafNode(new_leaf_offset, new_leaf);

        return new_leaf.keys[0];
    }

    Key splitInternal(int node_offset, InternalNode& node, int& new_node_offset) {
        new_node_offset = allocInternalNode();
        InternalNode new_node;

        int mid = (M - 1) / 2;
        Key promote_key = node.keys[mid];

        new_node.n = node.n - mid - 1;
        for (int i = 0; i < new_node.n; i++) {
            new_node.keys[i] = node.keys[mid + 1 + i];
        }
        for (int i = 0; i <= new_node.n; i++) {
            new_node.children[i] = node.children[mid + 1 + i];
        }
        node.n = mid;

        writeInternalNode(node_offset, node);
        writeInternalNode(new_node_offset, new_node);

        return promote_key;
    }

    bool insertRecursive(int node_offset, bool is_leaf, const Key& key, Key& promote_key, int& new_child_offset, bool& split) {
        if (is_leaf) {
            LeafNode leaf;
            readLeafNode(node_offset, leaf);

            // Check if key already exists
            for (int i = 0; i < leaf.n; i++) {
                if (leaf.keys[i] == key) {
                    return false;  // Duplicate
                }
            }

            insertIntoLeaf(leaf, key);

            if (leaf.n >= L) {
                promote_key = splitLeaf(node_offset, leaf, new_child_offset);
                split = true;
            } else {
                writeLeafNode(node_offset, leaf);
                split = false;
            }
            return true;
        } else {
            InternalNode node;
            readInternalNode(node_offset, node);

            int pos = 0;
            while (pos < node.n && !(key < node.keys[pos])) {
                pos++;
            }

            bool child_is_leaf = (node_offset == root && is_leaf_root) ? false :
                                 (node.children[pos] >= sizeof(int) * 3 + sizeof(bool) + internal_count * sizeof(InternalNode));

            Key child_promote_key;
            int child_new_offset;
            bool child_split;

            if (!insertRecursive(node.children[pos], child_is_leaf, key, child_promote_key, child_new_offset, child_split)) {
                return false;
            }

            if (child_split) {
                for (int i = node.n; i > pos; i--) {
                    node.keys[i] = node.keys[i - 1];
                    node.children[i + 1] = node.children[i];
                }
                node.keys[pos] = child_promote_key;
                node.children[pos + 1] = child_new_offset;
                node.n++;

                if (node.n >= M - 1) {
                    promote_key = splitInternal(node_offset, node, new_child_offset);
                    split = true;
                } else {
                    writeInternalNode(node_offset, node);
                    split = false;
                }
            } else {
                split = false;
            }
            return true;
        }
    }

public:
    BPTree(const std::string& fname) : filename(fname), root(-1), leaf_count(0), internal_count(0), is_leaf_root(true) {
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

            root = allocLeafNode();
            LeafNode leaf;
            writeLeafNode(root, leaf);
            writeHeader();
        }
    }

    ~BPTree() {
        if (file.is_open()) {
            file.close();
        }
    }

    void insert(const char* index, int value) {
        Key key(index, value);
        Key promote_key;
        int new_child_offset;
        bool split;

        if (!insertRecursive(root, is_leaf_root, key, promote_key, new_child_offset, split)) {
            return;  // Duplicate, ignore
        }

        if (split) {
            int new_root = allocInternalNode();
            InternalNode new_root_node;
            new_root_node.n = 1;
            new_root_node.keys[0] = promote_key;
            new_root_node.children[0] = root;
            new_root_node.children[1] = new_child_offset;
            writeInternalNode(new_root, new_root_node);
            root = new_root;
            is_leaf_root = false;
            writeHeader();
        }
    }

    void find(const char* index, std::vector<int>& result) {
        result.clear();
        if (root == -1) return;

        int current = root;
        bool is_leaf = is_leaf_root;

        while (!is_leaf) {
            InternalNode node;
            readInternalNode(current, node);

            int pos = 0;
            Key search_key(index, 0);
            while (pos < node.n && !(search_key < node.keys[pos])) {
                pos++;
            }

            current = node.children[pos];
            is_leaf = (current >= sizeof(int) * 3 + sizeof(bool) + internal_count * sizeof(InternalNode));
        }

        LeafNode leaf;
        readLeafNode(current, leaf);

        for (int i = 0; i < leaf.n; i++) {
            if (leaf.keys[i].keyEqual(index)) {
                result.push_back(leaf.keys[i].value);
            }
        }

        // Check next leaves for same key
        while (leaf.next != -1) {
            readLeafNode(leaf.next, leaf);
            bool found = false;
            for (int i = 0; i < leaf.n; i++) {
                if (leaf.keys[i].keyEqual(index)) {
                    result.push_back(leaf.keys[i].value);
                    found = true;
                } else if (found) {
                    break;
                }
            }
            if (!found) break;
        }

        std::sort(result.begin(), result.end());
    }

    void remove(const char* index, int value) {
        // Simple delete without rebalancing for now
        Key key(index, value);

        int current = root;
        bool is_leaf = is_leaf_root;

        while (!is_leaf) {
            InternalNode node;
            readInternalNode(current, node);

            int pos = 0;
            while (pos < node.n && !(key < node.keys[pos])) {
                pos++;
            }

            current = node.children[pos];
            is_leaf = (current >= sizeof(int) * 3 + sizeof(bool) + internal_count * sizeof(InternalNode));
        }

        LeafNode leaf;
        readLeafNode(current, leaf);

        for (int i = 0; i < leaf.n; i++) {
            if (leaf.keys[i] == key) {
                for (int j = i; j < leaf.n - 1; j++) {
                    leaf.keys[j] = leaf.keys[j + 1];
                }
                leaf.n--;
                writeLeafNode(current, leaf);
                return;
            }
        }
    }
};

#endif
