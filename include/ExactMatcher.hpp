#ifndef EXACTMATCHER_HPP
#define EXACTMATCHER_HPP

#include <string>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstdint>
#include "Task2.h"

class ExactMatcher {
  public:
    // Stores query info
    std::unordered_set<QueryID> queries;
    // Store words per query (Use for easy tracking during deletion)
    std::unordered_map<QueryID, std::vector<std::string>> tokens;
    // Stores deduplicated words and which query they belong to (Used for iteration when matching)
    std::unordered_map<std::string, std::vector<QueryID>> words;
    // Stores how many words are left to match in order for this query to match (Used to check how many queries match)
    std::unordered_map<QueryID, uint8_t> wordsleft;
    ExactMatcher();
    void addQuery(QueryID query_id, const char* q_str);
    void removeQuery(QueryID q_id);
    std::vector<QueryID> matchQueries(std::unordered_map<std::string, std::array<uint8_t,26>> &freq);
};



#endif //EXACTMATCHER_HPP
