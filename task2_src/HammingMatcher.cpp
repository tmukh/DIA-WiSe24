
#include "../include/HammingMatcher.hpp"

using namespace std;

HammingMatcher::HammingMatcher(): queries(), wordsleft(), wordsByLength(), frequencies() {}

void HammingMatcher::addQuery(QueryID query_id, const char* q_str, unsigned int dist){
    // Store query info
    queries[query_id] = dist;
    tokens[query_id] = vector<string>{};

    // Tokenization
	istringstream stream(q_str);
	string qtoken;
  	while (stream >> qtoken) {
        // Store the word
        tokens[query_id].push_back(qtoken);
        auto len = qtoken.length();
        // Store deduplicated words
        if (words.find(qtoken) == words.end()){
        	words[qtoken] = vector<QueryID>{query_id};
        	if (wordsByLength[len-MIN_WORD_LENGTH].size() > 0) wordsByLength[len-MIN_WORD_LENGTH].push_back(qtoken);
        	else wordsByLength[len-MIN_WORD_LENGTH] = vector<string>{qtoken};
        }
        else words[qtoken].push_back(query_id);
        // Calculate the frequency of the word
        auto freq = frequencies[qtoken];
        memset(&freq, 0, 26 * sizeof(uint8_t));
        for (size_t i=0; i<len; i++) freq[qtoken[i]-'a']++;
    }
    wordsleft[query_id] = tokens[query_id].size();
}

void HammingMatcher::removeQuery(QueryID q_id){
  if (queries.find(q_id) == queries.end()) return;
    queries.erase(q_id);
    for (const string& token : tokens[q_id]){
      auto &qwd = words[token];
      auto len = token.length();
      if (qwd.size() < 1) {
        vector<string>& wbl = wordsByLength[len-MIN_WORD_LENGTH];
        wbl.erase(remove(wbl.begin(), wbl.end(), token), wbl.end());
        frequencies.erase(token);
      }
      qwd.erase(remove(qwd.begin(), qwd.end(), q_id), qwd.end());
    }
    tokens.erase(q_id);
    wordsleft.erase(q_id);
}

bool HammingMatcher::matchFrequencies(array<uint8_t,26> &f1, array<uint8_t,26> &f2, unsigned int dist){
  unsigned int sum = 0;
  for (unsigned int i=0; i<26; i++) sum += abs(f1[i] - f2[i]);
  return sum <=dist;
}

vector<QueryID> HammingMatcher::matchQueries(array<vector<string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &DocwordsByLength,
                             unordered_map<string, array<uint8_t,26>> &freq){
    vector<QueryID> results;
    unordered_map<QueryID, uint8_t> intermediates;
    // Go through all matching length words
    for (size_t i = 0; i < wordsByLength.size(); i++) {
      // Check every word in query
      for (const auto& qword : wordsByLength[i]) {
        unsigned int min_dist = MAX_WORD_LENGTH;
        // Check every word in document
        for (const auto& dword : DocwordsByLength[i]) {
          // Frequency filtering
          auto freq1 = frequencies[qword];
          auto freq2 = freq[dword];
          if (!matchFrequencies(freq1, freq2, 3)) continue;
          // Hamming distance
          unsigned int dresult=0;
          for(unsigned int j=0;j<qword.length();j++) if(qword[j]!=dword[j]) dresult++;
          if (dresult < min_dist) min_dist = dresult;
          if (min_dist <= 1) break;
          }
        // Check out
        for (const auto id : words[qword]){
          if (min_dist <= queries[id]){
            if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
            else intermediates[id] += 1;
            if (intermediates[id] == wordsleft[id]) results.push_back(id);
          }
        }
      }
    }
    return results;
}