#include "SparkTask1.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>
#include <jni.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <linux/limits.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <atomic>

class Logger {
public:
    enum Level { ERROR, INFO, DEBUG };
    static Level logLevel;
    
    static void log(const std::string& message, Level level = ERROR) {
        if (level > logLevel) return;
        
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::ofstream logFile("spark_matcher_debug.log", std::ios::app);
        logFile << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X")
                << " - " << message << std::endl;
    }
};


Logger::Level Logger::logLevel = Logger::ERROR;
namespace fs = std::filesystem;


static std::unique_ptr<SparkMatchEngine> engine;

bool SparkMatchEngine::initJVM() {
    Logger::log("Starting JVM");
    std::string sparkHome = "/opt/spark";
    Logger::log("Checking spark home directory");  
    std::string javaHome = "/usr/lib/jvm/java-17-openjdk";

    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Error getting current working directory" << std::endl;
        return false;
    }
    std::string classpath = "-Djava.class.path=";
    
    
    std::string sparkJarDir = sparkHome + "/jars";

    try {
        Logger::log("Starting jar directory scan");  
        for (const auto& entry : fs::directory_iterator(sparkJarDir)) {
            if (entry.path().extension() == ".jar") {
                classpath += entry.path().string() + ":";
            }
        }
        Logger::log("Finished jar directory scan");  
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error processing Spark jars directory: " << e.what() << std::endl;
        return false;
    }

    
    std::string matcherJarPath = std::string(cwd) + "/spark-matcher/target/spark-matcher-1.0-SNAPSHOT.jar";
    classpath += matcherJarPath;

    
    JavaVMOption* options = new JavaVMOption[10];
    int optionCount = 0;

    options[optionCount++].optionString = const_cast<char*>(classpath.c_str());
    options[optionCount++].optionString = const_cast<char*>("-Xmx4g");
    
    
    
    options[optionCount++].optionString = const_cast<char*>("--add-opens=java.base/sun.nio.ch=ALL-UNNAMED");
    options[optionCount++].optionString = const_cast<char*>("-Dlog4j.configuration=file:src/main/resources/log4j.properties");
    
    
    
    
    

    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = optionCount;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_TRUE;

    
    jint rc = JNI_CreateJavaVM(&jvm, reinterpret_cast<void**>(&env), &vm_args);
    
    
    delete[] options;

    if (rc != JNI_OK) {
        std::cerr << "JVM Creation Failed. Error code: " << rc << std::endl;
        return false;
    }
    
    std::cout << "JVM created successfully" << std::endl;  
    
    try {
        
        if (!env) {
            std::cerr << "JNIEnv is null after JVM creation" << std::endl;
            return false;
        }

        
        jclass sparkConfClass = env->FindClass("org/apache/spark/SparkConf");
        if (!sparkConfClass) {
            std::cerr << "Failed to find SparkConf class" << std::endl;
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            return false;
        }
        
        
        jmethodID sparkConfConstructor = env->GetMethodID(sparkConfClass, "<init>", "()V");
        if (!sparkConfConstructor) {
            std::cerr << "Failed to find SparkConf constructor" << std::endl;
            return false;
        }
        
        jobject sparkConf = env->NewObject(sparkConfClass, sparkConfConstructor);
        if (!sparkConf) {
            std::cerr << "Failed to create SparkConf object" << std::endl;
            return false;
        }
        
jmethodID setLogLevel = env->GetMethodID(sparkConfClass, "set", 
    "(Ljava/lang/String;Ljava/lang/String;)Lorg/apache/spark/SparkConf;");

jstring logKey = env->NewStringUTF("spark.log.level");
jstring logValue = env->NewStringUTF("OFF");
env->CallObjectMethod(sparkConf, setLogLevel, logKey, logValue);
env->DeleteLocalRef(logKey);
env->DeleteLocalRef(logValue);


options[optionCount++].optionString = const_cast<char*>("-Dlog4j2.configurationFile=src/main/resources/log4j2.properties");
options[optionCount++].optionString = const_cast<char*>("-Dlog4j.configuration=src/main/resources/log4j2.properties");
        
        jmethodID setAppName = env->GetMethodID(sparkConfClass, "setAppName",
            "(Ljava/lang/String;)Lorg/apache/spark/SparkConf;");
        if (!setAppName) {
            std::cerr << "Failed to find setAppName method" << std::endl;
            return false;
        }
        jmethodID setMaster = env->GetMethodID(sparkConfClass, "setMaster",
            "(Ljava/lang/String;)Lorg/apache/spark/SparkConf;");
        if (!setMaster) {
            std::cerr << "Failed to find setMaster method" << std::endl;
            return false;
        }
        
        jstring masterUrl = env->NewStringUTF("local[*]");
        jstring appName = env->NewStringUTF("DocumentMatcher");
        
        env->CallObjectMethod(sparkConf, setMaster, masterUrl);
        env->CallObjectMethod(sparkConf, setAppName, appName);
        
        env->DeleteLocalRef(masterUrl);
        env->DeleteLocalRef(appName);
        
        
        jclass javaSparkContextClass = env->FindClass("org/apache/spark/api/java/JavaSparkContext");
        if (!javaSparkContextClass) {
            std::cerr << "Failed to find JavaSparkContext class" << std::endl;
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            return false;
        }
        
        jmethodID contextConstructor = env->GetMethodID(javaSparkContextClass, "<init>", 
            "(Lorg/apache/spark/SparkConf;)V");
        if (!contextConstructor) {
            std::cerr << "Failed to find JavaSparkContext constructor" << std::endl;
            return false;
        }
        
        sparkContext = env->NewGlobalRef(
            env->NewObject(javaSparkContextClass, contextConstructor, sparkConf)
        );
        
        
        sparkContextClass = (jclass)env->NewGlobalRef(javaSparkContextClass);
        queryClass = (jclass)env->NewGlobalRef(env->FindClass("com/spark/matcher/Query"));
        matcherClass = (jclass)env->NewGlobalRef(env->FindClass("com/spark/matcher/DocumentMatcher"));
        
        if (!sparkContext || !sparkContextClass || !queryClass || !matcherClass) {
            std::cerr << "Failed to initialize one or more required classes" << std::endl;
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            return false;
        }
        
        std::cout << "Spark context created successfully" << std::endl;  
        
        
        env->DeleteLocalRef(sparkConfClass);
        env->DeleteLocalRef(sparkConf);
        env->DeleteLocalRef(javaSparkContextClass);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Standard exception during Spark initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown exception during Spark initialization" << std::endl;
        return false;
    }
}

void SparkMatchEngine::destroyJVM() {
    std::lock_guard<std::mutex> lock(jvm_mutex);
    Logger::log("Starting JVM cleanup sequence");
    
    try {
        if (env && !env->ExceptionCheck()) {
            
            if (sparkContext && sparkContextClass) {
                jmethodID stopMethod = env->GetMethodID(sparkContextClass, "stop", "()V");
                if (stopMethod) {
                    env->CallVoidMethod(sparkContext, stopMethod);
                }
            }
            
            
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
            }

            
            if (matcherClass) env->DeleteGlobalRef(matcherClass);
            if (queryClass) env->DeleteGlobalRef(queryClass);
            if (sparkContextClass) env->DeleteGlobalRef(sparkContextClass);
            if (sparkContext) env->DeleteGlobalRef(sparkContext);

            matcherClass = nullptr;
            queryClass = nullptr;
            sparkContextClass = nullptr;
            sparkContext = nullptr;
        }

        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (jvm) {
            Logger::log("Destroying JVM");
            jint result = jvm->DestroyJavaVM();
            if (result != JNI_OK) {
                Logger::log("Error destroying JVM: " + std::to_string(result));
            }
            jvm = nullptr;
            env = nullptr;
        }
    } catch (...) {
        Logger::log("Exception during JVM cleanup");
    }
}

SparkMatchEngine::SparkMatchEngine() : jvm(nullptr), env(nullptr), sparkContext(nullptr),
    sparkContextClass(nullptr), queryClass(nullptr), matcherClass(nullptr) {
        std::lock_guard<std::mutex> lock(jvm_mutex);
        initJVM();
}

SparkMatchEngine::~SparkMatchEngine() {
    static std::atomic<bool> destroying{false};
    if (destroying.exchange(true)) {
        Logger::log("Destructor already in progress, skipping");
        return;
    }
    
    Logger::log("Starting SparkMatchEngine destructor");
    
    try {
        
        destroy();
        
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        
        destroyJVM();
        
        Logger::log("SparkMatchEngine destructor completed successfully");
    } catch (const std::exception& e) {
        Logger::log("Exception in destructor: " + std::string(e.what()));
    } catch (...) {
        Logger::log("Unknown exception in destructor");
    }
    
    destroying = false;
}

ErrorCode SparkMatchEngine::initialize() {
    Logger::log("Initializing SparkMatchEngine");
    
    active_queries.clear();
    pending_results.clear();
    
    Logger::log("Active queries and pending results cleared");
    return EC_SUCCESS;
}
ErrorCode SparkMatchEngine::destroy() {
    std::lock_guard<std::mutex> lock(jvm_mutex);
    Logger::log("Starting SparkMatchEngine destroy sequence");
    
    
    active_queries.clear();
    pending_results.clear();

    try {
        if (env && !env->ExceptionCheck()) {
            
            if (sparkContext && sparkContextClass) {
                Logger::log("Stopping SparkContext");
                jmethodID stopMethod = env->GetMethodID(sparkContextClass, "stop", "()V");
                if (stopMethod) {
                    env->CallVoidMethod(sparkContext, stopMethod);
                    if (env->ExceptionCheck()) {
                        Logger::log("Exception during SparkContext stop");
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } catch (...) {
        Logger::log("Exception during Spark cleanup");
    }

    Logger::log("SparkMatchEngine destroy completed");
    return EC_SUCCESS;
}

ErrorCode SparkMatchEngine::startQuery(QueryID query_id, const std::string& query_str,
                                     MatchType match_type, unsigned int match_dist) {
    
    std::stringstream ss;
    ss << "Starting Query: ID=" << query_id 
       << ", String='" << query_str 
       << "', Type=" << match_type 
       << ", Distance=" << match_dist;
    Logger::log(ss.str());

    
    auto it = std::find_if(active_queries.begin(), active_queries.end(), 
        [query_id](const Query& q) { return q.first == query_id; });
    
    if (it != active_queries.end()) {
        Logger::log("Error: Query with ID " + std::to_string(query_id) + " already exists");
        return EC_FAIL;
    }

    Query query;
    query.first = query_id;
    query.second = query_str;
    query.match_type = match_type;
    query.match_dist = match_dist;
    
    active_queries.push_back(query);
    
    
    ss.str("");
    ss << "Active Queries after addition: " << active_queries.size();
    Logger::log(ss.str());

    return EC_SUCCESS;
}
ErrorCode SparkMatchEngine::endQuery(QueryID query_id) {
    Logger::log("Ending Query: ID=" + std::to_string(query_id));

    size_t initial_size = active_queries.size();
    
    auto it = std::remove_if(active_queries.begin(), active_queries.end(),
        [query_id](const Query& q) { 
            return q.first == query_id; 
        });
    
    if (it == active_queries.end() || std::distance(it, active_queries.end()) == 0) {
        Logger::log("Error: Query with ID " + std::to_string(query_id) + " not found");
        return EC_FAIL;
    }
    
    active_queries.erase(it, active_queries.end());
    
    std::stringstream ss;
    ss << "Queries after removal: " << active_queries.size() 
       << " (Removed: " << (initial_size - active_queries.size()) << ")";
    Logger::log(ss.str());

    return EC_SUCCESS;
}ErrorCode SparkMatchEngine::matchDocument(DocID doc_id, const std::string& doc_str) {
    std::lock_guard<std::mutex> lock(jvm_mutex);
    Logger::log("Starting matchDocument execution");
    
    if (active_queries.empty()) {
        Logger::log("No active queries to match against document");
        return EC_SUCCESS;
    }

    try {
        
        if (!env || !env->ExceptionCheck()) {
            Logger::log("Creating ArrayList for queries");
            
            
            jclass arrayListClass = env->FindClass("java/util/ArrayList");
            jobject queryList = nullptr;
            
            if (arrayListClass) {
                jmethodID constructor = env->GetMethodID(arrayListClass, "<init>", "()V");
                if (constructor) {
                    queryList = env->NewObject(arrayListClass, constructor);
                }
            }

            if (!queryList) {
                Logger::log("Failed to create ArrayList");
                return EC_FAIL;
            }

            
            jmethodID addMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
            for (const auto& query : active_queries) {
                jstring queryStr = env->NewStringUTF(query.second.c_str());
                jobject queryObj = env->NewObject(queryClass, env->GetMethodID(queryClass, "<init>", "(JLjava/lang/String;II)V"),
                    (jlong)query.first, queryStr, (jint)query.match_type, (jint)query.match_dist);
                
                if (queryObj) {
                    env->CallBooleanMethod(queryList, addMethod, queryObj);
                    env->DeleteLocalRef(queryObj);
                }
                env->DeleteLocalRef(queryStr);
            }

            
            Logger::log("Calling matchQueries");
            jstring docString = env->NewStringUTF(doc_str.c_str());
            jmethodID matchMethod = env->GetStaticMethodID(matcherClass, "matchQueries",
                "(Lorg/apache/spark/api/java/JavaSparkContext;Ljava/util/List;Ljava/lang/String;)[J");

            if (!matchMethod) {
                Logger::log("Failed to find matchQueries method");
                env->DeleteLocalRef(docString);
                env->DeleteLocalRef(queryList);
                return EC_FAIL;
            }

            jobject resultArray = env->CallStaticObjectMethod(matcherClass, matchMethod, 
                sparkContext, queryList, docString);

            
            if (env->ExceptionCheck()) {
                Logger::log("Exception occurred during matchQueries call");
                env->ExceptionDescribe();
                env->ExceptionClear();
                env->DeleteLocalRef(docString);
                env->DeleteLocalRef(queryList);
                return EC_FAIL;
            }

            
            if (resultArray) {
                jsize len = env->GetArrayLength((jlongArray)resultArray);
                jlong* elements = env->GetLongArrayElements((jlongArray)resultArray, nullptr);
                
                if (elements) {
                    std::vector<QueryID> matches;
                    matches.reserve(len);
                    for (jsize i = 0; i < len; i++) {
                        matches.push_back((QueryID)elements[i]);
                    }
                    
                    env->ReleaseLongArrayElements((jlongArray)resultArray, elements, JNI_ABORT);
                    
                    if (!matches.empty()) {
                        pending_results.emplace_back(doc_id, std::move(matches));
                    }
                }
                
                env->DeleteLocalRef(resultArray);
            }

            
            env->DeleteLocalRef(docString);
            env->DeleteLocalRef(queryList);
            
            Logger::log("matchDocument completed successfully");
            return EC_SUCCESS;
        }
    } catch (const std::exception& e) {
        Logger::log(std::string("Exception in matchDocument: ") + e.what());
    } catch (...) {
        Logger::log("Unknown exception in matchDocument");
    }

    return EC_FAIL;
}
ErrorCode SparkMatchEngine::getNextAvailRes(DocID& doc_id, unsigned int& num_res,
                                          std::vector<QueryID>& query_ids) {
    Logger::log("Retrieving Next Available Result");

    if (pending_results.empty()) {
        Logger::log("No pending results available");
        return EC_NO_AVAIL_RES;
    }
    
    auto& result = pending_results.front();
    doc_id = result.first;
    query_ids = result.second;
    num_res = query_ids.size();
    
    std::stringstream ss;
    ss << "Result Retrieved: Document ID=" << doc_id 
       << ", Matching Queries=" << num_res;
    Logger::log(ss.str());

    
    ss.str("");
    ss << "Matching Query IDs: ";
    for (auto qid : query_ids) {
        ss << qid << " ";
    }
    Logger::log(ss.str());

    pending_results.erase(pending_results.begin());
    
    return EC_SUCCESS;
}




extern "C" {
    void FreeQueryIds(QueryID* query_ids) {
    if (query_ids) {
        free(query_ids);
    }
}
    ErrorCode InitializeIndex() {
        engine = std::make_unique<SparkMatchEngine>();
        return engine->initialize();
    }
    
        ErrorCode DestroyIndex() {
        Logger::log("Starting DestroyIndex");
        
        if (!engine) {
            Logger::log("Engine already destroyed");
            return EC_FAIL;
        }
        
        try {
            
            auto result = engine->destroy();
            
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            
            {
                std::unique_ptr<SparkMatchEngine> tmp;
                tmp.swap(engine);  
                
                
                if (tmp) {
                    tmp.reset();
                }
            }
            
            Logger::log("DestroyIndex completed successfully");
            return result;
            
        } catch (const std::exception& e) {
            Logger::log("Exception during DestroyIndex: " + std::string(e.what()));
            return EC_FAIL;
        } catch (...) {
            Logger::log("Unknown exception during DestroyIndex");
            return EC_FAIL;
        }
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
        current_results.clear();
        
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
