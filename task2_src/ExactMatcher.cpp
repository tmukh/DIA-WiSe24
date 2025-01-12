#include "../include/ExactMatcher.hpp"
using namespace std;
ExactMatcher::ExactMatcher():queries(), tokens(), words(), wordsleft() {}

void ExactMatcher::addQuery(QueryID query_id, const char* q_str){
    // Store query info
    queries.insert(query_id);
    tokens[query_id] = vector<string>{};
    // Tokenization
	istringstream stream(q_str);
	string qtoken;
  	while (stream >> qtoken) {
        // Store the word
        tokens[query_id].push_back(qtoken);
        // Deduplicate query words linked to query ids
        if (words.find(qtoken) == words.end()) words[qtoken] = vector<QueryID>{query_id};
        else words[qtoken].push_back(query_id);
    }
    wordsleft[query_id] = tokens.size();
}

void ExactMatcher::removeQuery(QueryID q_id){
  if (queries.find(q_id) == queries.end()) return;
    queries.erase(q_id);
    for (const string& token : tokens[q_id]){
      auto &qwd = words[token];
      qwd.erase(remove(qwd.begin(), qwd.end(), q_id), qwd.end());
    }
    tokens.erase(q_id);
    wordsleft.erase(q_id);
}

vector<QueryID> ExactMatcher::matchQueries(unordered_map<string, array<uint8_t,26>> &freq){
    vector<QueryID> results;
    unordered_map<QueryID, uint8_t> intermediates;
    for(const auto &[key, vals] : words){
      if (freq.find(key) == freq.end()){
        for (const auto &id : vals){
            intermediates[id] = MAX_QUERY_WORDS+1;
            // TODO: Blacklist all queries
        }
      }
      else{
        for (const auto &id : vals){
            if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
            else intermediates[id] += 1;
            if (intermediates[id] == wordsleft[id]) results.push_back(id);
        }
      }
    }
    return results;
}