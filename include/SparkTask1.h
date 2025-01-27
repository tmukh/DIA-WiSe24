#ifndef __SPARK_TASK1_H_
#define __SPARK_TASK1_H_

#include <string>
#include <vector>
#include <memory>
#include <mutex>  
#include <jni.h>
#include "Task1.h"


class JNILocalRef {
    JNIEnv* env;
    jobject ref;
public:
    JNILocalRef(JNIEnv* e, jobject r) : env(e), ref(r) {}
    ~JNILocalRef() { if(ref) env->DeleteLocalRef(ref); }
    operator jobject() const { return ref; }
    jobject get() const { return ref; }
};

class SparkMatchEngine {
private:
    
    std::mutex jvm_mutex;

    
    struct Query {
        QueryID first;
        std::string second;
        MatchType match_type;
        unsigned int match_dist;
    };

    
    JavaVM* jvm;
    JNIEnv* env;
    jobject sparkContext;
    jclass sparkContextClass;
    jclass queryClass;
    jclass matcherClass;

    std::vector<Query> active_queries;
    std::vector<std::pair<DocID, std::vector<QueryID>>> pending_results;

    
    bool initJVM();
    void destroyJVM();

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


extern "C" {
    ErrorCode InitializeIndex();
    ErrorCode DestroyIndex();
    ErrorCode StartQuery(QueryID query_id, const char* query_str,
                        MatchType match_type, unsigned int match_dist);
    ErrorCode EndQuery(QueryID query_id);
    ErrorCode MatchDocument(DocID doc_id, const char* doc_str);
    ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res,
                            QueryID** p_query_ids);
    
    void FreeQueryIds(QueryID* query_ids);
}

#endif 