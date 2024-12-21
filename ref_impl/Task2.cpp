#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <array>
#include <bits/stdc++.h>
#include <unordered_map>
#include <vector>
#include "../include/Task2.h"
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

/// Structs:
///
/// Struct representing query details
struct Query {
    QueryID query_id;
    vector<string> query_tokens;
    MatchType match_type;
    unsigned int match_dist;

    Query(const QueryID query_id, vector<string> tokens, const MatchType match_type, const unsigned int match_dist)
        : query_id(query_id), query_tokens(tokens),match_type(match_type), match_dist(match_dist) {
    }

    void show() const {
        cout << "query_id: " << query_id;
    	cout << ", query_str: ";
    	for (const auto& str : query_tokens) {
        	cout << str << " ";
        }
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
        cout << ", num_res: " << num_res << endl;
    }
};

/// Data structures to hold the data
///
/// QueryId->QueryPointer Dictionary
unordered_map<QueryID, shared_ptr<Query>> queryMap;
unordered_map<DocID, shared_ptr<Document>> docMap;

///////////////////////////////////////////////////////////////////////////////////////////////

char *convert_to_chr(const string & s)
{
    const auto pc = new char[s.size()+1];
    strcpy(pc, s.c_str());
    return pc;
}
QueryID convert_to_qid(const string & s)
{
    auto *pc = new char[s.size()+1];
    strcpy(pc, s.c_str());
    QueryID q = static_cast<QueryID>(stoul(pc));
    return q;
}

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

    // Tokenization
    vector<string> tokens;
	char* str = new char[strlen(query_str) + 1];
	strcpy(str, query_str);
	char* token = strtok(str, " ");
	while (token != nullptr) {
		tokens.emplace_back(token);
		token = strtok(nullptr, " ");
	}
    const auto query = make_shared<Query>(query_id, tokens, match_type, match_dist);
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

ErrorCode MatchDocument(DocID doc_id, const char* doc_str)
{
	vector<QueryID> query_ids;

	// Tokenization
	vector<string> DocTokens;
	char cur_doc_str[MAX_DOC_LENGTH];
	strcpy(cur_doc_str, doc_str);

	char* docToken = strtok(cur_doc_str, " ");
	while (docToken != nullptr) {
		DocTokens.emplace_back(docToken);
		docToken = strtok(nullptr, " ");
	}

	// Iterate on all active queries to compare them with this new document
	for (const auto& [key, quer] : queryMap) {
		bool matching_query=true;
		for (const string& qtoken : quer->query_tokens){
            if (!matching_query) break;

            bool matching_word=false;
			for (const auto& dtoken : DocTokens){
				if(quer->match_type==MT_EXACT_MATCH){
					if(strcmp(qtoken.c_str(), dtoken.c_str())==0){
                      matching_word=true;
                      break;
                    }
				}
				else if(quer->match_type==MT_HAMMING_DIST){
					unsigned int num_mismatches=HammingDistance(qtoken.c_str(), qtoken.length(), dtoken.c_str(), dtoken.length());
					if(num_mismatches<=quer->match_dist){
                        matching_word=true;
                        break;
                    }
				}
				else if(quer->match_type==MT_EDIT_DIST){
                    // Low-Overhead Filtering
                    unsigned int m = qtoken.length();
                    unsigned int n = dtoken.length();
                    unsigned int diff = abs((int)(m - n));
                    if (diff > quer->match_dist) continue;
                    // Frequency Histogram
                    vector<int> frequency;
                    frequency.assign(26,0);

                    for (size_t i=0; i<max(m,n); i++){
                      if (i < m) frequency[qtoken[i]-'a']++;
                      if (i < n) frequency[dtoken[i]-'a']--;
                    }
                    unsigned int sum_of_abs = accumulate(frequency.begin(), frequency.end(), 0, [](int sum, int val) {
                      return sum + abs(val);
                    });
                    if (sum_of_abs > 2*(quer->match_dist) - diff ) continue;
                    // Actually compute the distance
					unsigned int edit_dist=EditDistance(qtoken.c_str(), qtoken.length(), dtoken.c_str(), dtoken.length());
					if(edit_dist<=quer->match_dist){
                    	matching_word=true;
                        break;
                    }
				}
			}
			if(!matching_word)matching_query=false;
		}

		if(matching_query) query_ids.push_back(quer->query_id);
	}
	QueryID* matched_query_ids;
	if(query_ids.size()>0){
        matched_query_ids=(unsigned int*)malloc(query_ids.size()*sizeof(unsigned int));
		for(long unsigned int i=0;i<query_ids.size();i++) matched_query_ids[i]=query_ids[i];
        sort(matched_query_ids, matched_query_ids + query_ids.size());
		const auto doc = make_shared<Document>(doc_id, query_ids.size(), matched_query_ids);
//        doc->show();
		// Add this result to the set of undelivered results
		docMap[doc->doc_id] = doc;
		return EC_SUCCESS;
    } else return EC_FAIL;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids){
	// Get the first undeliverd resuilt from "docs" and return it
	*p_doc_id=0; *p_num_res=0; *p_query_ids=0;
	if (docMap.empty()) return EC_NO_AVAIL_RES;
	if (docMap.find(*p_doc_id) == docMap.end()) *p_doc_id = (DocID)(docMap.begin()->first);
	*p_doc_id = docMap[*p_doc_id]->doc_id; *p_num_res = docMap[*p_doc_id]->num_res; *p_query_ids=docMap[*p_doc_id]->query_ids;
	docMap.erase(*p_doc_id);
	return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

int main(int, char**){
}