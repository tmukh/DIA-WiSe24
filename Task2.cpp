#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <bits/stdc++.h>
#include <unordered_map>
#include <vector>
using namespace std;

/// Definitions:
///
/// Maximum document length in characters.
#define MAX_DOC_LENGTH (1<<22)
/// Maximum word length in characters.
#define MAX_WORD_LENGTH 31
/// Minimum word length in characters.
#define MIN_WORD_LENGTH 4
/// Maximum number of words in a query.
#define MAX_QUERY_WORDS 5
/// Maximum query length in characters.
#define MAX_QUERY_LENGTH ((MAX_WORD_LENGTH+1)*MAX_QUERY_WORDS)
/// Query ID type.
typedef unsigned int QueryID;
/// Document ID type.
typedef unsigned int DocID;
/// Matching types:
typedef enum{MT_EXACT_MATCH, MT_HAMMING_DIST, MT_EDIT_DIST}MatchType;
/// Error codes:
typedef enum{EC_SUCCESS, EC_NO_AVAIL_RES, EC_FAIL}ErrorCode;

/// Structs:
///
/// Struct representing query details
struct Query {
    QueryID query_id;
    char query_str[MAX_QUERY_LENGTH]{};
    MatchType match_type;
    unsigned int match_dist;

    Query(const QueryID query_id, const char* str, const MatchType match_type, const unsigned int match_dist)
        : query_id(query_id), match_type(match_type), match_dist(match_dist) {
        strcpy(query_str, str);
    }

    void show() const {
        cout << "query_id: " << query_id;
        cout << ", query_str: " << query_str;
        cout << ", match_type: " << match_type;
        cout << ", match_dist: " << match_dist << endl;
    }
};

/// Struct representing document details
struct Document {
    DocID doc_id;
    unsigned int num_res;
    QueryID* query_ids;
    Document(const DocID doc_id, const unsigned int num_res, QueryID* query_ids)
    : doc_id(doc_id), num_res(num_res), query_ids(query_ids) {}

    void show() const {
        cout << "doc_id: " << doc_id;
        cout << ", num_res: " << num_res;
    }
};

/// Data structures to hold the data
///
/// QueryId->QueryPointer Dictionary
unordered_map<QueryID, shared_ptr<Query>> queryMap;

///////////////////////////////////////////////////////////////////////////////////////////////

// Computes edit distance between a null-terminated string "a" with length "na"
//  and a null-terminated string "b" with length "nb"
int EditDistance(const char* a, const int na, const char* b, const int nb){
    static int T[2][MAX_WORD_LENGTH+1];
    int ib;
    int cur=0;
    int ia = 0;
    for(ib=0;ib<=nb;ib++)
        T[cur][ib]=ib;
    cur=1-cur;

    for(ia=1;ia<=na;ia++){
        constexpr int oo=0x7FFFFFFF;
        for(ib=0;ib<=nb;ib++)
            T[cur][ib]=oo;

        int ib_st=0;
        const int ib_en=nb;
        ib=0;
        T[cur][ib]=ia;
        ib_st++;

        for(ib=ib_st;ib<=ib_en;ib++){
            int ret=oo;
            const int d1=T[1-cur][ib]+1;
            const int d2=T[cur][ib-1]+1;
            int d3=T[1-cur][ib-1];
            if(a[ia-1]!=b[ib-1]) d3++;

            if(d1<ret) ret=d1;
            if(d2<ret) ret=d2;
            if(d3<ret) ret=d3;

            T[cur][ib]=ret;
        }
        cur=1-cur;
    }
    return T[1-cur][nb];
}

///////////////////////////////////////////////////////////////////////////////////////////////

// Computes Hamming distance between a null-terminated string "a" with length "na"
//  and a null-terminated string "b" with length "nb"
unsigned int HammingDistance(const char* a, const int na, const char* b, const int nb){
    int j;
    constexpr int oo=0x7FFFFFFF;
    if(na!=nb) return oo;

    unsigned int num_mismatches=0;
    for(j=0;j<na;j++) if(a[j]!=b[j]) num_mismatches++;

    return num_mismatches;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode InitializeIndex(){return EC_SUCCESS;}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode DestroyIndex(){return EC_SUCCESS;}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initialize an individual query
ErrorCode StartQuery(QueryID        query_id,
                     const char*    query_str,
                     MatchType      match_type,
                     unsigned int   match_dist) {
    if (queryMap.find(query_id) != queryMap.end()) return EC_FAIL;
    const auto query = make_shared<Query>(query_id, query_str, match_type, match_dist);
    queryMap[query->query_id] = query;
    return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// End an individual query
ErrorCode EndQuery(QueryID query_id) {
    if (queryMap.find(query_id) == queryMap.end()) return EC_FAIL;
    queryMap.erase(query_id);
    return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode MatchDocument(DocID doc_id, const char* doc_str);

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids);

///////////////////////////////////////////////////////////////////////////////////////////////
// Read the current file and process the instructions line by line
void ReadFile(const string& filename)
{
    // Open file stream
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // read line by line
    string line,token;
    int counter = 0;
    while (getline(file,line)) {
        if (counter >=3) break;

        istringstream lineStream(line);
        vector<string> tokens;
        // push all space separated tokens into a vector
        while (lineStream >> token) {
            tokens.push_back(token);
        }

        // Interpret line argument
        switch (tokens[0]) {
            case 's':
                auto result = StartQuery(tokens[1], tokens[2], tokens[3], tokens[4]);
            case 'e':
                auto result = EndQuery(tokens[1]);
            case 'm':
                break;
            case 'e':
                break;
            default:
                break;
        }
        counter++;
    }

    file.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////

int main(int, char**){
    InitializeIndex();
    ReadFile("/home/awsam/Desktop/apps/DIA/DIA-WiSe24/big_test.txt");
    DestroyIndex();
}
