#ifndef HAMMINGMATCHER_HPP
#define HAMMINGMATCHER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <sstream>
#include "../include/Task2.h"

class HammingMatcher {
    public:
        // Stores query info
        std::unordered_map<QueryID, uint8_t> queries;
        // Store words per query (Use for easy tracking during deletion)
        std::unordered_map<QueryID, std::vector<std::string>> tokens;
        // Stores how many words are left to match in order for this query to match (Used to check how many queries match)
        std::unordered_map<QueryID, uint8_t> wordsleft;
        // Stores deduplicated words and which query they belong to (Used for iteration when matching)
		std::unordered_map<std::string, std::vector<QueryID>> words;
        // Stores dedplicated words by length to narrow down comparsions
        std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> wordsByLength;
        // Stores frequencies of words to apply filtering
        std::unordered_map<std::string, std::array<uint8_t,26>> frequencies;
        HammingMatcher();
        void addQuery(QueryID query_id, const char* q_str, unsigned int dist);
        void removeQuery(QueryID q_id);
        bool matchFrequencies(std::array<uint8_t,26> &f1, std::array<uint8_t,26> &f2, unsigned int dist);
        std::vector<QueryID> matchQueries(std::array<std::vector<std::string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &DocwordsByLength,
                                     std::unordered_map<std::string, std::array<uint8_t,26>> &freq);
};



#endif //HAMMINGMATCHER_HPP
