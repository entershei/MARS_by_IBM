#include <cstdio>
#include <cstdint>
#include <iostream>
#include <ios>
#include <vector>

int main() {
    freopen("for_tables", "r", stdin);
    freopen("out_tables", "w", stdout);

    uint32_t x;
    std::vector<uint32_t> tables;

    while(std::cin >> std::hex >> x) {
        tables.push_back(x);
    }

    std::cout << " = {";
    for (int i = 0; i < tables.size() - 1; ++i) {
        std::cout << tables[i] << ", ";
        if (i > 0 && i % 6 == 0) {
            std::cout << "\n   ";
        }
    }

    std::cout << tables[tables.size() - 1] << "};\n";

    fclose(stdout);
}