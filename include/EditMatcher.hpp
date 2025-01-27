#ifndef EDITMATCHER_HPP
#define EDITMATCHER_HPP

#include <string>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <algorithm>
#include <sstream>
#include <cstdint>
#include "Trie.hpp"
#include "Task2.h"
#include <thread>
#include <mutex>

class EditMatcher {
public:
    // Reorder members to match initialization order
    std::unordered_map<QueryID, std::vector<std::string>> tokens;
    std::unordered_map<std::string, std::vector<QueryID>> words;
    std::unordered_map<QueryID, uint8_t> wordsleft;
    std::unordered_map<QueryID, uint8_t> queries;
    std::unordered_set<std::string> DW;
    // Move trie to the end
    Trie trie;

    // Updated constructor to match new order
    EditMatcher(): 
        tokens(), 
        words(), 
        wordsleft(), 
        queries(), 
        DW(), 
        trie() {}
    
    void addQuery(QueryID query_id, const char* q_str, unsigned int dist);
    void removeQuery(QueryID q_id);
    void printQueries();
    std::vector<std::pair<std::string, uint8_t>> partialMatchQueries(
        std::vector<std::string> vec,
        std::vector<std::pair<std::string, uint8_t>>& matches);
    std::vector<QueryID> matchQueries(
        std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH>& wordsByLength);
    std::vector<QueryID> ThreadMatching(
        std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &wordsByLength);
};

#endif //EDITMATCHER_HPP