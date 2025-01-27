#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <bits/stdc++.h>
#include <vector>
#include "Task1.h"
using namespace std;
/// Structs:
// Keeps all information related to an active query
struct Query
{
	QueryID query_id;
	char tokens[MAX_QUERY_LENGTH];
	MatchType match_type;
	unsigned int match_dist;
    Query(QueryID query_id_,
          MatchType match_type_,
          unsigned int match_dist_,
		  char *tokens_) : query_id(query_id_), match_type(match_type_), match_dist(match_dist_)
        {
      strcpy(tokens, tokens_);
    }
};
// Keeps all query ID results associated with a dcoument
struct Document
{
	DocID doc_id;
	unsigned int num_res;
	QueryID* query_ids;
    Document(DocID doc_id_, unsigned int num_res_) : doc_id(doc_id_), num_res(num_res_){}
};

/// Data structures to hold the data
// Keeps all currently active queries
vector<Query> queries;
// Keeps all currently available results that has not been returned yet
vector<Document> docs;

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

ErrorCode InitializeIndex(){return EC_SUCCESS;}

ErrorCode DestroyIndex(){return EC_SUCCESS;}

ErrorCode StartQuery(QueryID query_id, const char* query_str, MatchType match_type, unsigned int match_dist){
	Query query{query_id, match_type, match_dist, convert_to_chr(query_str)};
	queries.push_back(query);
	return EC_SUCCESS;
}

ErrorCode EndQuery(QueryID query_id){
	unsigned int i, n=queries.size();
	for(i=0;i<n;i++){
		if(queries[i].query_id==query_id){
			queries.erase(queries.begin()+i);
			break;
		}
	}
	return EC_SUCCESS;
}

ErrorCode MatchDocument(DocID doc_id, const char* doc_str){
	char cur_doc_str[MAX_DOC_LENGTH];
	strcpy(cur_doc_str, doc_str);

	unsigned int i, n=queries.size();
	vector<unsigned int> query_ids;

	// Iterate on all active queries to compare them with this new document
	for(i=0;i<n;i++)
	{
		bool matching_query=true;
		Query* quer=&queries[i];

		int iq=0;
		while(quer->tokens[iq] && matching_query)
		{
			while(quer->tokens[iq]==' ') iq++;
			if(!quer->tokens[iq]) break;
			char* qword=&quer->tokens[iq];

			int lq=iq;
			while(quer->tokens[iq] && quer->tokens[iq]!=' ') iq++;
			char qt=quer->tokens[iq];
			quer->tokens[iq]=0;
			lq=iq-lq;

			bool matching_word=false;

			int id=0;
			while(cur_doc_str[id] && !matching_word)
			{
				while(cur_doc_str[id]==' ') id++;
				if(!cur_doc_str[id]) break;
				char* dword=&cur_doc_str[id];

				int ld=id;
				while(cur_doc_str[id] && cur_doc_str[id]!=' ') id++;
				char dt=cur_doc_str[id];
				cur_doc_str[id]=0;

				ld=id-ld;

				if(quer->match_type==MT_EXACT_MATCH)
				{
					if(strcmp(qword, dword)==0) matching_word=true;
				}
				else if(quer->match_type==MT_HAMMING_DIST)
				{
					unsigned int num_mismatches=HammingDistance(qword, lq, dword, ld);
					if(num_mismatches<=quer->match_dist) matching_word=true;
				}
				else if(quer->match_type==MT_EDIT_DIST)
				{
					unsigned int edit_dist=EditDistance(qword, lq, dword, ld);
					if(edit_dist<=quer->match_dist) matching_word=true;
				}

				cur_doc_str[id]=dt;
			}

			quer->tokens[iq]=qt;

			if(!matching_word)
			{
				// This query has a word that does not match any word in the document
				matching_query=false;
			}
		}

		if(matching_query)
		{
			// This query matches the document
			query_ids.push_back(quer->query_id);
		}
	}

	Document doc(doc_id, query_ids.size());
	if(doc.num_res) doc.query_ids=(unsigned int*)malloc(doc.num_res*sizeof(unsigned int));
	for(i=0;i<doc.num_res;i++) doc.query_ids[i]=query_ids[i];
	docs.push_back(doc);

	return EC_SUCCESS;
}

ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids){
	*p_doc_id=0; *p_num_res=0; *p_query_ids=0;
	if(docs.size()==0) return EC_NO_AVAIL_RES;
	*p_doc_id=docs[0].doc_id;
    *p_num_res=docs[0].num_res;
    *p_query_ids=docs[0].query_ids;
	docs.erase(docs.begin());
	return EC_SUCCESS;
}