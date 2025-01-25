#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <array>
#include <bits/stdc++.h>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include "../include/Task2.h"
#include "../include/DocCache.hpp"
#include "../include/Document.hpp"
#include "../include/ExactMatcher.hpp"
#include "../include/HammingMatcher.hpp"
#include "../include/EditMatcher.hpp"
using namespace std;

struct ThreadResult {
    vector<QueryID> results;
    std::mutex mutex;

    void addResults(const vector<QueryID>& new_results) {
        lock_guard<std::mutex> lock(mutex);
        results.insert(results.end(), new_results.begin(), new_results.end());
    }
};

WordStorage docCache;
ExactMatcher exactMatcher;
HammingMatcher hammingMatcher;
EditMatcher editMatcher;
stack<shared_ptr<Document>> docStack;
///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode InitializeIndex(){
  exactMatcher = ExactMatcher();
  hammingMatcher = HammingMatcher();
  editMatcher = EditMatcher();
  docCache = WordStorage();
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode DestroyIndex(){
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initialize an individual query
ErrorCode StartQuery(QueryID        query_id,
                     const char*    query_str,
                     MatchType      match_type,
                     unsigned int   match_dist) {
  if (match_type == MatchType::MT_EXACT_MATCH) exactMatcher.addQuery(query_id, query_str);
  else if (match_type == MatchType::MT_HAMMING_DIST) hammingMatcher.addQuery(query_id, query_str, match_dist);
  else editMatcher.addQuery(query_id, query_str, match_dist);
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// End an individual query
ErrorCode EndQuery(QueryID query_id) {
  exactMatcher.removeQuery(query_id);
  editMatcher.removeQuery(query_id);
  hammingMatcher.removeQuery(query_id);
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Match the current document with all registered queries
ErrorCode MatchDocument(DocID doc_id, const char* doc_str){
  docCache.updateDocument(doc_str);
  vector<QueryID> result;
  // Calculate results
  vector<QueryID> vec1 = exactMatcher.matchQueries(docCache.frequencies);
  vector<QueryID> vec2 = hammingMatcher.matchQueries(docCache.wordsByLength, docCache.frequencies);
  vector<QueryID> vec3 = editMatcher.matchQueries(docCache.wordsByLength);
  // Concatenate results
  result.reserve(vec1.size() + vec2.size() + vec3.size());
  result.insert(result.end(), vec1.begin(), vec1.end());
  result.insert(result.end(), vec2.begin(), vec2.end());
  result.insert(result.end(), vec3.begin(), vec3.end());
  // Save results
  docStack.push(make_shared<Document>(doc_id, result));
  return EC_SUCCESS;
}

ErrorCode MatchDocumentThreaded(DocID doc_id, const char* doc_str){
  docCache.updateDocument(doc_str);
  ThreadResult threadResult;
  threadResult.results.reserve(1000);
  // initialize the workers
  auto exactMatchWorker = [&]() {threadResult.addResults(exactMatcher.matchQueries(docCache.frequencies));};
  auto hammingMatchWorker = [&]() {threadResult.addResults(hammingMatcher.matchQueries(docCache.wordsByLength, docCache.frequencies));};
  auto editMatchWorker = [&]() {threadResult.addResults(editMatcher.matchQueries(docCache.wordsByLength));};
  // Start the threads
  thread t1(exactMatchWorker);
  thread t2(hammingMatchWorker);
  thread t3(editMatchWorker);
  // retrieve results
  t1.join();
  t2.join();
  t3.join();
  // Save combined results
  docStack.push(make_shared<Document>(doc_id, threadResult.results));
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Get the first result from the stack of results
ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids){
  *p_doc_id=0; *p_num_res=0; *p_query_ids=0;
  if (docStack.empty()) return EC_NO_AVAIL_RES;
  shared_ptr<Document> doc = docStack.top();
  *p_doc_id = doc->doc_id; *p_num_res = doc->num_res; *p_query_ids = doc->query_ids;
  docStack.pop();
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////