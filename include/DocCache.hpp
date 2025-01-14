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

using namespace std;

class WordStorage {
    public:
        array<vector<string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> wordsByLength;
        unordered_map<string, array<uint8_t,26>> frequencies;

        WordStorage();
        void updateDocument(const char* doc_str);
        bool exists(string word);
};



#endif //WORDSTORAGE_HPP
