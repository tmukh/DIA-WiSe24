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
vector<QueryID> EditMatcher::ThreadMatching(array<vector<string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH> &wordsByLength){
	vector<string> vec1; vector<string> vec2; vector<string> vec3; vector<string> vec4;
    vector<pair<string, uint8_t>> results1, results2, results3, results4;
    vector<pair<string, uint8_t>> final_matches;
	vec1.reserve(wordsByLength.size() * 1/4);
    vec2.reserve(wordsByLength.size() * 1/4);
    vec3.reserve(wordsByLength.size() * 1/4);
    vec4.reserve(wordsByLength.size() * 1/4);

    size_t current_vec = 0;
    for (const auto& bucket : wordsByLength) {
        for (const auto& word : bucket) {
            switch(current_vec) {
                case 0: vec1.push_back(word); break;
                case 1: vec2.push_back(word); break;
                case 2: vec3.push_back(word); break;
                case 3: vec4.push_back(word); break;
            }

            current_vec = (current_vec + 1) % 4;
        }
    }
    thread t1(&EditMatcher::partialMatchQueries, this, std::ref(vec1), std::ref(results1));
	thread t2(&EditMatcher::partialMatchQueries, this, std::ref(vec2), std::ref(results2));
	thread t3(&EditMatcher::partialMatchQueries, this, std::ref(vec3), std::ref(results3));
	thread t4(&EditMatcher::partialMatchQueries, this, std::ref(vec4), std::ref(results4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    final_matches.reserve(results1.size() + results2.size() + results3.size() + results4.size());
    final_matches.insert(final_matches.end(), results1.begin(), results1.end());
    final_matches.insert(final_matches.end(), results2.begin(), results2.end());
    final_matches.insert(final_matches.end(), results3.begin(), results3.end());
    final_matches.insert(final_matches.end(), results4.begin(), results4.end());

    unordered_map<string, uint8_t> unique_words;
    for (const pair<string, uint8_t>& pair : final_matches) {
    	if (unique_words.count(pair.first) == 0 ||
        	pair.second < unique_words[pair.first]) {
        	unique_words[pair.first] = pair.second;
    	}
    }
    vector<QueryID> results;
    unordered_map<QueryID, uint8_t> intermediates;
    for(const auto &res: unique_words){
      	for(const auto &id: words[res.first]){
            if (res.second <= queries[id]){
                //cout << "ID: " << id << " " << unsigned(queries[id]) << "<" << unsigned(res->second)<< " increasing " << unsigned(intermediates[id]) << endl;
                if (intermediates.find(id) == intermediates.end()) intermediates[id] = 1;
                else intermediates[id] += 1;
                if (intermediates[id] == wordsleft[id]) results.push_back(id);
            }
        }
    }
    return results;
}

vector<pair<string, uint8_t>>  EditMatcher::partialMatchQueries(vector<string> vec, vector<pair<string, uint8_t>>& matches){
  	unordered_map<string, uint8_t> results;
    // Check every word in Document
  	for (const auto& dword : vec) {
    	if (trie.nearest(dword, results)){  // Found
      		while (!results.empty()) {
        		auto res = results.begin();
                matches.push_back(*res);
        		res = results.erase(res);
      		}
    	}
  	}
    return matches;
}

vector<QueryID> EditMatcher::matchQueries(array<vector<string>, MAX_WORD_LENGTH-MIN_WORD_LENGTH>& wordsByLength){
	vector<QueryID> QueryResults;
    unordered_map<string, uint8_t> results;
    unordered_map<QueryID, uint8_t> intermediates;
    unordered_map<string, uint8_t> skip;
    // Check every word in Document
    for (size_t i = 0; i < wordsByLength.size(); ++i) {
      for (const auto& dword : wordsByLength[i]) {
        // Found
        if (trie.nearest(dword, results)){
          while (!results.empty()) {
            auto previous_val = 4;
            auto res = results.begin();
            //cout << "Found (" << dword << ") matched with (" << res->first << ")" << endl;
            if (skip.find(res->first) != skip.end()) {
              if (skip[res->first] <= res->second){
                results.erase(res);
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
                if (intermediates[id] == wordsleft[id]) QueryResults.push_back(id);
              }
            }
            res = results.erase(res);
          }
        }
      }
    }
    return QueryResults;
}

void EditMatcher::printQueries(){
  for (const auto& q: queries){
    cout << q.first << endl;
  }
}