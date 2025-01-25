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
#include "Trie.hpp"
#include "../include/Task2.h"
#include <thread>
#include <mutex>

class EditMatcher {
public:
    // Stores query info
    std::unordered_map<QueryID, uint8_t> queries;
    // Store words per query (Use for easy tracking during deletion)
    std::unordered_map<QueryID, std::vector<std::string>> tokens;
    // Store dead words (unique to each query)
    std::unordered_set<std::string> DW;
    // Stores deduplicated words and which query they belong to (Used for iteration when matching)
    std::unordered_map<std::string, std::vector<QueryID>> words;
    // Stores how many words are left to match in order for this query to match (Used to check how many queries match)
    std::unordered_map<QueryID, uint8_t> wordsleft;
    // Data structure for search purposes
    Trie trie;

    EditMatcher(): trie(), queries(), tokens(), words(), wordsleft() {}
    void addQuery(QueryID query_id, const char* q_str,unsigned int dist);
    void removeQuery(QueryID q_id);
    void printQueries();
    std::vector<std::pair<std::string, uint8_t>>  partialMatchQueries(std::vector<std::string> vec,
                                                              std::vector<std::pair<std::string, uint8_t>>& matches);
    std::vector<QueryID> matchQueries(std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH>& wordsByLength);
    std::vector<QueryID> ThreadMatching(std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &wordsByLength);
};


#endif //EDITMATCHER_HPP
