#include <iostream>

#include "../../engine/src/core/str.h"


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: <TargetChars>\n";
        return 1;
    }
    StrHs hs(argv[1]);
    std::cout << hs.hash << "\n";
}