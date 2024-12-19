#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <bits/stdc++.h>
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
///////////////////////////////////////////////////////////////////////////////////////////////
// Keeps all information related to an active query
struct Query
{
	QueryID query_id;
	char str[MAX_QUERY_LENGTH];
	MatchType match_type;
	unsigned int match_dist;
};

///////////////////////////////////////////////////////////////////////////////////////////////
// Keeps all query ID results associated with a dcoument
struct Document
{
	DocID doc_id;
	unsigned int num_res;
	QueryID* query_ids;
};

///////////////////////////////////////////////////////////////////////////////////////////////

/// Data structures to hold the data
// Keeps all currently active queries
vector<Query> queries;
// Keeps all currently available results that has not been returned yet
vector<Document> docs;

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

ErrorCode StartQuery(QueryID query_id, const char* query_str, MatchType match_type, unsigned int match_dist)
{
	Query query{};
	query.query_id=query_id;
	strcpy(query.str, query_str);
	query.match_type=match_type;
	query.match_dist=match_dist;
	// Add this query to the active query set
	queries.push_back(query);
	return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode EndQuery(QueryID query_id)
{
	// Remove this query from the active query set
	unsigned int i, n=queries.size();
	for(i=0;i<n;i++)
	{
		if(queries[i].query_id==query_id)
		{
			queries.erase(queries.begin()+i);
			break;
		}
	}
	return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode MatchDocument(DocID doc_id, const char* doc_str)
{
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
		while(quer->str[iq] && matching_query)
		{
			while(quer->str[iq]==' ') iq++;
			if(!quer->str[iq]) break;
			char* qword=&quer->str[iq];

			int lq=iq;
			while(quer->str[iq] && quer->str[iq]!=' ') iq++;
			char qt=quer->str[iq];
			quer->str[iq]=0;
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

			quer->str[iq]=qt;

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

	Document doc;
	doc.doc_id=doc_id;
	doc.num_res=query_ids.size();
	doc.query_ids=0;
	if(doc.num_res) doc.query_ids=(unsigned int*)malloc(doc.num_res*sizeof(unsigned int));
	for(i=0;i<doc.num_res;i++) doc.query_ids[i]=query_ids[i];
	// Add this result to the set of undelivered results
	docs.push_back(doc);

	return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids)
{
	// Get the first undeliverd result from "docs" and return it
	*p_doc_id=0; *p_num_res=0; *p_query_ids=0;
	if(docs.size()==0) return EC_NO_AVAIL_RES;
	*p_doc_id=docs[0].doc_id;
    *p_num_res=docs[0].num_res;
    *p_query_ids=docs[0].query_ids;
	docs.erase(docs.begin());
	return EC_SUCCESS;
}

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
    while (getline(file,line)) {
        istringstream lineStream(line);
        vector<string> tokens;
        // push all space separated tokens into a vector
        while (lineStream >> token) {
            tokens.push_back(token);
        }

        // Interpret line argument
    	ErrorCode result;
    	unsigned int len;

        // Start Query
    	cout << tokens[0] << endl;
        if(*tokens[0].c_str()=='s'){
            len = static_cast<unsigned int>(stoul(tokens[4]));
            auto first = tokens.begin() + 4;
            auto last = tokens.begin() + 4 + len;
        	vector<char*> words;
            transform(first, last, back_inserter(words), convert_to_chr);

            result = StartQuery(static_cast<QueryID>(stoul(tokens[1])),
                                *words.data(),
                                static_cast<MatchType>(stoul(tokens[2])),
                                static_cast<unsigned int>(stoul(tokens[3])));
            continue;
        }

        // End Query
        if(*tokens[0].c_str()=='e'){
          result = EndQuery(static_cast<QueryID>(stoul(tokens[1])));
          continue;
        }

        // Match the documents
        if(*tokens[0].c_str()=='m')
        {
          len = static_cast<unsigned int>(stoul(tokens[2]));
          auto first = tokens.begin() + 2;
          auto last = tokens.begin() + 2 + len;
          vector<char*> words;
          transform(first, last, back_inserter(words), convert_to_chr);
          result = MatchDocument(static_cast<DocID>(stoul(tokens[1])),*words.data());
          continue;
        }

        // Return results of matched queries
        if(*tokens[0].c_str()=='r'){
          len = static_cast<unsigned int>(stoul(tokens[2]));
          auto first = tokens.begin() + 2;
          auto last = tokens.begin() + 2 + len;
          vector<QueryID> words;
          transform(first, last, back_inserter(words), convert_to_qid);
          DocID doc = (static_cast<DocID>(stoul(tokens[1])));
        	auto fw = &words[0];
          result = GetNextAvailRes(&doc, &len, &(fw));
          continue;
        }
    }

    file.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////

int main(int, char**){
    InitializeIndex();
    ReadFile("/home/awsam/Desktop/apps/DIA/DIA-WiSe24/big_test.txt");
    DestroyIndex();
}