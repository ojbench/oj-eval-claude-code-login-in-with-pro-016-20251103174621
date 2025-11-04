#include <iostream>
#include <string>
#include <vector>
#include "bptree.hpp"

int main() {
    BPTree tree("data.db");

    int n;
    std::cin >> n;

    for (int i = 0; i < n; i++) {
        std::string cmd;
        std::cin >> cmd;

        if (cmd == "insert") {
            std::string index;
            int value;
            std::cin >> index >> value;
            tree.insert(index.c_str(), value);
        } else if (cmd == "delete") {
            std::string index;
            int value;
            std::cin >> index >> value;
            tree.remove(index.c_str(), value);
        } else if (cmd == "find") {
            std::string index;
            std::cin >> index;

            std::vector<int> result;
            tree.find(index.c_str(), result);

            if (result.empty()) {
                std::cout << "null" << std::endl;
            } else {
                for (size_t j = 0; j < result.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << result[j];
                }
                std::cout << std::endl;
            }
        }
    }

    return 0;
}
