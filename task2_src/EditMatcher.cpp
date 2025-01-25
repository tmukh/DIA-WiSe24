#include "../include/EditMatcher.hpp"

using namespace std;

void EditMatcher::addQuery(QueryID query_id, const char* q_str, unsigned int dist){
    // Store query info
    queries[query_id] = dist;
    uint8_t counter = 0;
    // Tokenization
	istringstream stream(q_str);
	string qtoken;
  	while (stream >> qtoken) {
        tokens[query_id].push_back(qtoken);
        // Store deduplicated words
        if (words.find(qtoken) == words.end()){
          words[qtoken] = vector<QueryID>{query_id};
          DW.insert(qtoken);
          trie.insert(qtoken);
        }
        else if ( words[qtoken].size()==1 ){
          DW.erase(qtoken);
          words[qtoken].push_back(query_id);
        }
        else words[qtoken].push_back(query_id);
        counter++;
    }
    wordsleft[query_id] = counter;
}

void EditMatcher::removeQuery(QueryID q_id){
  if (queries.find(q_id) == queries.end()) return;
    queries.erase(q_id);
    for (const string& token : tokens[q_id]){
      words[token].erase(remove(words[token].begin(), words[token].end(), q_id), words[token].end());
      if (DW.find(token) != DW.end()){
        DW.erase(token);
        trie.remove(token);
      }
    }
    tokens.erase(q_id);
    wordsleft.erase(q_id);
}

vector<QueryID> EditMatcher::matchQueries(array<vector<string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &wordsByLength){
    vector<QueryID> results;
    unordered_map<QueryID, uint8_t> intermediates;
    unordered_map<string, uint8_t> skip;
    // Check every word in Document
    for (size_t i = 0; i < wordsByLength.size(); ++i) {
      for (const auto& dword : wordsByLength[i]) {
        // Found
        if (trie.nearest(dword)){
          while (!trie.results.empty()) {
            auto previous_val = 4;
            auto res = trie.results.begin();
            //cout << "Found (" << dword << ") matched with (" << res->first << ")" << endl;
            if (skip.find(res->first) != skip.end()) {
              if (skip[res->first] <= res->second){
                trie.results.erase(res);
                continue;
              }
              previous_val = skip[res->first];
            }
            skip[res->first] = res->second;
            for(const auto &id: words[res->first]){
              if (res->second <= queries[id] && queries[id] < previous_val){
                //cout << "ID: " << id << " " << unsigned(queries[id]) << "<" << unsigned(res->second)<< " increasing " << unsigned(intermediates[id]) << endl;
                if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
                else intermediates[id] += 1;
                if (intermediates[id] == wordsleft[id]) results.push_back(id);
              }
            }
            res = trie.results.erase(res);
          }
        }
      }
    }
    return results;
}

void EditMatcher::printQueries(){
  for (const auto& q: queries){
    cout << q.first << endl;
  }
}