#ifndef __SPARK_TASK1_H_
#define __SPARK_TASK1_H_

#include <string>
#include <vector>
#include <memory>
#include <jni.h>
#include "Task1.h"

class SparkMatchEngine {
private:
    // Query structure definition
    struct Query {
        QueryID first;
        std::string second;
        MatchType match_type;
        unsigned int match_dist;
    };

    // JVM related members
    JavaVM* jvm;
    JNIEnv* env;
    jobject sparkContext;
    
    // Data members
    std::vector<Query> active_queries;
    std::vector<std::pair<DocID, std::vector<QueryID>>> pending_results;
    
    // Internal methods
    bool initJVM();
    void destroyJVM();
    bool matchQueryToDoc(const std::string& query, const std::string& doc,
                        MatchType match_type, unsigned int match_dist);
    unsigned int computeHammingDistance(const std::string& a, const std::string& b);
    unsigned int computeEditDistance(const std::string& a, const std::string& b, unsigned int match_dist);
public:
    SparkMatchEngine();
    ~SparkMatchEngine();
    
    ErrorCode initialize();
    ErrorCode destroy();
    ErrorCode startQuery(QueryID query_id, const std::string& query_str,
                        MatchType match_type, unsigned int match_dist);
    ErrorCode endQuery(QueryID query_id);
    ErrorCode matchDocument(DocID doc_id, const std::string& doc_str);
    ErrorCode getNextAvailRes(DocID& doc_id, unsigned int& num_res,
                            std::vector<QueryID>& query_ids);
};

// C API implementations
extern "C" {
    ErrorCode InitializeIndex();
    ErrorCode DestroyIndex();
    ErrorCode StartQuery(QueryID query_id, const char* query_str,
                        MatchType match_type, unsigned int match_dist);
    ErrorCode EndQuery(QueryID query_id);
    ErrorCode MatchDocument(DocID doc_id, const char* doc_str);
    ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res,
                            QueryID** p_query_ids);
}

#endif // __SPARK_TASK1_H_