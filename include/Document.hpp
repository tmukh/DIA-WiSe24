#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <bits/stdc++.h>
#include "Task2.h"

using namespace std;

/// Struct representing document details
struct Document {
    DocID doc_id;
    uint32_t num_res;
    QueryID* query_ids;

    Document(const DocID doc_id, vector<QueryID>& q_ids): doc_id(doc_id), num_res(q_ids.size()) {
      sort(q_ids.begin(),q_ids.end());
      query_ids = reinterpret_cast<QueryID*>(malloc(num_res * sizeof(QueryID)));
      memcpy(query_ids, q_ids.data(), num_res * sizeof(QueryID));
    }

    void show() const {
        cout << "doc_id: " << doc_id;
        cout << ", num_res: " << num_res << endl;
    }
};

#endif //DOCUMENT_H