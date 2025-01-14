#include "SparkTask1.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>
#include <jni.h>
#include <iostream>

// Global instance of our engine
static std::unique_ptr<SparkMatchEngine> engine;

// Helper function to split string into words
std::vector<std::string_view> splitIntoWords(const std::string& str) {
    static thread_local std::vector<std::string_view> words;
    words.clear();
    
    const char* start = str.data();
    const char* end = start + str.length();
    const char* word_start = start;
    
    while (word_start < end) {
        // Skip spaces
        while (word_start < end && *word_start == ' ') ++word_start;
        if (word_start == end) break;
        
        // Find word end
        const char* word_end = word_start;
        while (word_end < end && *word_end != ' ') ++word_end;
        
        words.emplace_back(word_start, word_end - word_start);
        word_start = word_end;
    }
    
    return words;
}

bool SparkMatchEngine::initJVM() {
    return true; // For now, we'll skip JVM initialization and just use threading
}

void SparkMatchEngine::destroyJVM() {
    // Nothing to do for now
}

SparkMatchEngine::SparkMatchEngine() : jvm(nullptr), env(nullptr), sparkContext(nullptr) {
    initJVM();
}

SparkMatchEngine::~SparkMatchEngine() {
    destroyJVM();
}

ErrorCode SparkMatchEngine::initialize() {
    active_queries.clear();
    pending_results.clear();
    return EC_SUCCESS;
}

ErrorCode SparkMatchEngine::destroy() {
    active_queries.clear();
    pending_results.clear();
    return EC_SUCCESS;
}

ErrorCode SparkMatchEngine::startQuery(QueryID query_id, const std::string& query_str,
                                     MatchType match_type, unsigned int match_dist) {
    Query query;
    query.first = query_id;
    query.second = query_str;
    query.match_type = match_type;
    query.match_dist = match_dist;
    active_queries.push_back(query);
    return EC_SUCCESS;
}

ErrorCode SparkMatchEngine::endQuery(QueryID query_id) {
    auto it = std::remove_if(active_queries.begin(), active_queries.end(),
                            [query_id](const auto& q) { 
                                return q.first == query_id; 
                            });
    active_queries.erase(it, active_queries.end());
    return EC_SUCCESS;
}

unsigned int SparkMatchEngine::computeHammingDistance(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return 0x7FFFFFFF;
    unsigned int distance = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        if (a[i] != b[i]) ++distance;
    }
    return distance;
}

unsigned int SparkMatchEngine::computeEditDistance(const std::string& a, const std::string& b, unsigned int match_dist) {
    const size_t m = a.length();
    const size_t n = b.length();
    
    // Early exit for empty strings or too different lengths
    if (m == 0) return n;
    if (n == 0) return m;
    if (abs(static_cast<int>(m - n)) > match_dist) return 0x7FFFFFFF;

    // Use static thread_local to avoid repeated allocations
    static thread_local std::vector<unsigned int> prev;
    static thread_local std::vector<unsigned int> curr;
    
    // Resize if needed
    if (prev.size() <= n) {
        prev.resize(n + 1);
        curr.resize(n + 1);
    }
    
    // Initialize first row
    for (size_t j = 0; j <= n; j++) {
        prev[j] = j;
    }
    
    // Fill the matrix while keeping track of minimum value in current row
    for (size_t i = 1; i <= m; i++) {
        curr[0] = i;
        unsigned int minInRow = curr[0];
        
        for (size_t j = 1; j <= n; j++) {
            curr[j] = std::min({
                prev[j] + 1,                    // deletion
                curr[j-1] + 1,                  // insertion
                prev[j-1] + (a[i-1] != b[j-1])  // substitution
            });
            minInRow = std::min(minInRow, curr[j]);
        }
        
        // Early exit if we can't possibly get under match_dist
        if (minInRow > match_dist) return 0x7FFFFFFF;
        
        // Swap vectors (avoid copy)
        prev.swap(curr);
    }
    
    return prev[n];
}

bool SparkMatchEngine::matchQueryToDoc(const std::string& query, const std::string& doc,
                                     MatchType match_type, unsigned int match_dist) {
    static thread_local std::vector<std::string_view> query_words;
    static thread_local std::vector<std::string_view> doc_words;
    
    query_words = splitIntoWords(query);
    if (query_words.empty()) return false;
    
    doc_words = splitIntoWords(doc);
    if (doc_words.empty()) return false;
    
    // For exact match, use faster comparison
    if (match_type == MT_EXACT_MATCH) {
        for (const auto& qword : query_words) {
            bool matched = false;
            for (const auto& dword : doc_words) {
                if (qword.length() == dword.length() && 
                    memcmp(qword.data(), dword.data(), qword.length()) == 0) {
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }
        return true;
    }
    
    // For Hamming distance, words must be same length
    if (match_type == MT_HAMMING_DIST) {
        for (const auto& qword : query_words) {
            bool matched = false;
            for (const auto& dword : doc_words) {
                if (qword.length() == dword.length() && 
                    computeHammingDistance(std::string(qword), std::string(dword)) <= match_dist) {
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }
        return true;
    }
    
    // Edit distance with early exits
    for (const auto& qword : query_words) {
        bool matched = false;
        for (const auto& dword : doc_words) {
            // In matchQueryToDoc where we call computeEditDistance:
if (abs(static_cast<int>(qword.length() - dword.length())) <= match_dist &&
    computeEditDistance(std::string(qword), std::string(dword), match_dist) <= match_dist) {
    matched = true;
    break;
}
        }
        if (!matched) return false;
    }
    
    return true;
}


ErrorCode SparkMatchEngine::matchDocument(DocID doc_id, const std::string& doc_str) {
    std::vector<QueryID> matching_queries;
    
    // Only use parallelization if we have enough queries
    const size_t PARALLEL_THRESHOLD = 100;
    
    if (active_queries.size() < PARALLEL_THRESHOLD) {
        // Single-threaded processing for small workloads
        for (const auto& query : active_queries) {
            if (matchQueryToDoc(query.second, doc_str, query.match_type, query.match_dist)) {
                matching_queries.push_back(query.first);
            }
        }
    } else {
        // Get max threads minus 1 to leave one core free
        unsigned int num_threads = std::thread::hardware_concurrency() - 1;
        if (num_threads == 0) num_threads = 1;  // Safeguard for single core systems
        
        std::vector<std::thread> threads;
        std::mutex results_mutex;
        
        // Pre-allocate vectors to avoid reallocations
        matching_queries.reserve(active_queries.size() / 4);  // Estimate 25% match rate
        threads.reserve(num_threads);
        
        std::cout << "Processing doc " << doc_id << " using " << num_threads 
                  << " threads for " << active_queries.size() << " queries" << std::endl;
        
        // Ensure we don't create more threads than queries
        num_threads = std::min(num_threads, static_cast<unsigned int>(active_queries.size()));
        size_t queries_per_thread = (active_queries.size() + num_threads - 1) / num_threads;
        
        // Launch threads
        for (unsigned int i = 0; i < num_threads; ++i) {
            size_t start_idx = i * queries_per_thread;
            size_t end_idx = std::min(start_idx + queries_per_thread, active_queries.size());
            
            if (start_idx >= active_queries.size()) break;
            
            threads.emplace_back([this, &doc_str, &matching_queries, &results_mutex, 
                                start_idx, end_idx]() {
                // Local vector for this thread's matches with pre-allocation
                std::vector<QueryID> thread_matches;
                thread_matches.reserve((end_idx - start_idx) / 4);  // Estimate 25% match rate
                
                for (size_t j = start_idx; j < end_idx; ++j) {
                    const auto& query = active_queries[j];
                    if (matchQueryToDoc(query.second, doc_str, query.match_type, query.match_dist)) {
                        thread_matches.push_back(query.first);
                    }
                }
                
                // Single lock at the end instead of per-match
                if (!thread_matches.empty()) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    matching_queries.insert(matching_queries.end(), 
                                         thread_matches.begin(), thread_matches.end());
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Sort results before storing
    if (!matching_queries.empty()) {
        std::sort(matching_queries.begin(), matching_queries.end());
        pending_results.emplace_back(doc_id, std::move(matching_queries));
    }
    
    return EC_SUCCESS;
}

ErrorCode SparkMatchEngine::getNextAvailRes(DocID& doc_id, unsigned int& num_res,
                                          std::vector<QueryID>& query_ids) {
    if (pending_results.empty()) {
        return EC_NO_AVAIL_RES;
    }
    
    auto& result = pending_results.front();
    doc_id = result.first;
    query_ids = result.second;
    num_res = query_ids.size();
    
    pending_results.erase(pending_results.begin());
    return EC_SUCCESS;
}

// C API implementations
extern "C" {
    ErrorCode InitializeIndex() {
        engine = std::make_unique<SparkMatchEngine>();
        return engine->initialize();
    }
    
    ErrorCode DestroyIndex() {
        if (!engine) return EC_FAIL;
        auto result = engine->destroy();
        engine.reset();
        return result;
    }
    
    ErrorCode StartQuery(QueryID query_id, const char* query_str,
                        MatchType match_type, unsigned int match_dist) {
        if (!engine) return EC_FAIL;
        return engine->startQuery(query_id, query_str, match_type, match_dist);
    }
    
    ErrorCode EndQuery(QueryID query_id) {
        if (!engine) return EC_FAIL;
        return engine->endQuery(query_id);
    }
    
    ErrorCode MatchDocument(DocID doc_id, const char* doc_str) {
        if (!engine) return EC_FAIL;
        return engine->matchDocument(doc_id, doc_str);
    }
    
    ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res,
                            QueryID** p_query_ids) {
        if (!engine || !p_doc_id || !p_num_res || !p_query_ids) return EC_FAIL;
        
        static std::vector<QueryID> current_results;
        auto result = engine->getNextAvailRes(*p_doc_id, *p_num_res, current_results);
        
        if (result == EC_SUCCESS) {
            *p_query_ids = (QueryID*)malloc(current_results.size() * sizeof(QueryID));
            if (*p_query_ids) {
                std::copy(current_results.begin(), current_results.end(), *p_query_ids);
            } else {
                *p_num_res = 0;
                return EC_FAIL;
            }
        }
        
        return result;
    }
}