#include "../include/EditMatcher.hpp"

using namespace std;

EditMatcher::EditMatcher(): trie(), queries(), tokens(), words(), wordsleft() {}

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
    // Check every word in Document
    for (size_t i = 0; i < wordsByLength.size(); ++i) {
      for (const auto& dword : wordsByLength[i]) {
        cout << dword << endl;
        // Exact word is found, no need to calculate distance
        if (words.find(dword) != words.end()){
          cout << "found" << endl;
          for (const auto &id : words[dword]){
            if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
            else intermediates[id] += 1;
            if (intermediates[id] == wordsleft[id]) results.push_back(id);
          }
          continue;
        }
        cout << "Calculating trie" << endl;
        trie.print();
        // Found
        if (trie.nearest(dword)){
          while (!trie.results.empty()){
            pair<string,uint8_t> res = trie.results.top();
            for(const auto &id: words[res.first]){
              if (queries[id] <= res.second){
                if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
                else intermediates[id] += 1;
                if (intermediates[id] == wordsleft[id]) results.push_back(id);
              }
            }
            trie.results.pop();
          }
        }
      }
    }
    return results;
}