#include "DocCache.hpp"
#include <sstream>
#include <cstdint>
#include <string>

WordStorage::WordStorage():wordsByLength(), frequencies() {}

void WordStorage::updateDocument(const char* doc_str){
  	// Clearing previous document
    frequencies.clear();
	for (auto& vec : wordsByLength) {
        vec.clear(); vec.shrink_to_fit();
    }

    // Tokenization
	std::istringstream stream(doc_str);
	std::string docToken;
  	while (stream >> docToken) {
        // Deduplicate document words
        if (frequencies.find(docToken) != frequencies.end()) continue;
        // Store words according to length
  		size_t length = docToken.length();
		wordsByLength[length-MIN_WORD_LENGTH].emplace_back(docToken);
        // Calculate frequency
        auto freq = frequencies[docToken];
  		memset(&freq, 0, 26 * sizeof(uint8_t));
        for (size_t i=0; i<length; i++) freq[docToken[i]-'a']++;
	}
}