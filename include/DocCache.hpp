#ifndef WORDSTORAGE_HPP
#define WORDSTORAGE_HPP

#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "../include/Task2.h"

class WordStorage {
    public:
        std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> wordsByLength;
        std::unordered_map<std::string, std::array<uint8_t,26>> frequencies;

        WordStorage();
        void updateDocument(const char* doc_str);
        bool exists(std::string word);
};



#endif //WORDSTORAGE_HPP
