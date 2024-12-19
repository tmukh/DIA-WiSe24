import heapq
from enum import Enum
from tqdm import tqdm


# Error codes
class ErrorCode(Enum):
    EC_SUCCESS = 0
    EC_NO_AVAIL_RES = 1
    EC_FAILURE = 2
# MatchType enumeration
class MatchType(Enum):
    MT_EXACT = 0
    MT_HAMMING_DIST = 1
    MT_EDIT_DIST = 2
# Global structures to manage queries and documents
queries = {}
documents = {}
results_heap = []
# StartQuery implementation
def start_query(query_id, query_str, match_type, match_dist):
    if query_id in queries:
        return ErrorCode.EC_FAILURE
    queries[query_id] = {
        "query_str": query_str.split(),
        "match_type": MatchType(match_type),
        "match_dist": match_dist
    }
    return ErrorCode.EC_SUCCESS
# EndQuery implementation
def end_query(query_id):
    if query_id not in queries:
        return ErrorCode.EC_FAILURE

    del queries[query_id]
    return ErrorCode.EC_SUCCESS
# Utility for word matching
def is_match(word1, word2, match_type, match_dist):
    if match_type == MatchType.MT_EXACT:
        return word1 == word2
    elif match_type == MatchType.MT_HAMMING_DIST:
        if len(word1) != len(word2):
            return False
        return sum(c1 != c2 for c1, c2 in zip(word1, word2)) <= match_dist
    elif match_type == MatchType.MT_EDIT_DIST:
        m, n = len(word1), len(word2)
        dp = [[0] * (n + 1) for _ in range(m + 1)]
        for i in range(m + 1):
            for j in range(n + 1):
                if i == 0:
                    dp[i][j] = j
                elif j == 0:
                    dp[i][j] = i
                elif word1[i - 1] == word2[j - 1]:
                    dp[i][j] = dp[i - 1][j - 1]
                else:
                    dp[i][j] = 1 + min(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1])
        return dp[m][n] <= match_dist
# MatchDocument implementation
def match_document(doc_id, doc_str):
    if doc_id in documents:
        return ErrorCode.EC_FAILURE
    doc_words = doc_str.split()
    matched_queries = []
    for query_id, query_data in queries.items():
        query_words = query_data["query_str"]
        match_type = query_data["match_type"]
        match_dist = query_data["match_dist"]
        if all(any(is_match(qw, dw, match_type, match_dist) for dw in doc_words) for qw in query_words):
            matched_queries.append(query_id)
    if matched_queries:
        heapq.heappush(results_heap, (doc_id, sorted(matched_queries)))
    documents[doc_id] = doc_str
    return ErrorCode.EC_SUCCESS
def get_next_avail_res():
    if not results_heap:
        return ErrorCode.EC_NO_AVAIL_RES, None, 0, None
    doc_id, query_ids = heapq.heappop(results_heap)
    num_res = len(query_ids)
    return ErrorCode.EC_SUCCESS, doc_id, num_res, query_ids
def parse_and_execute(file_path, output_file):
    lines = open(file_path, 'r').readlines()
    results = []
    with open(output_file, 'w') as output, tqdm(total=len(lines), desc="Processing lines") as pbar:
        for line in lines:
            parts = line.strip().split()
            action = parts[0]
            if action == "s":  # StartQuery
                query_id = int(parts[1])
                match_type = int(parts[2])
                match_dist = int(parts[3])
                num_words = int(parts[4])
                query_str = " ".join(parts[5:5 + num_words])
                result = start_query(query_id, query_str, match_type, match_dist)
                results.append(f"StartQuery({query_id}, {query_str}, {match_type}, {match_dist}) -> {result}")
                output.write(results[-1] + "\n")
            elif action == "e":  # EndQuery
                query_id = int(parts[1])
                result = end_query(query_id)
                results.append(f"EndQuery({query_id}) -> {result}")
                output.write(results[-1] + "\n")
            elif action == "m":  # MatchDocument
                doc_id = int(parts[1])
                num_words = int(parts[2])
                doc_str = " ".join(parts[3:3 + num_words])
                result = match_document(doc_id, doc_str)
                results.append(f"MatchDocument({doc_id}, {doc_str}) -> {result}")
                output.write(results[-1] + "\n")
            elif action == "r":  # GetNextAvailRes (with expected results)
                doc_id = int(parts[1])
                num_results = int(parts[2])
                query_ids = list(map(int, parts[3:3 + num_results]))
                result, fetched_doc_id, fetched_num_res, fetched_query_ids = get_next_avail_res()
                if result == ErrorCode.EC_SUCCESS:
                    is_doc_id_match = fetched_doc_id == doc_id
                    is_num_res_match = fetched_num_res == num_results
                    is_query_ids_match = query_ids == sorted(fetched_query_ids)

                    result_str = (
                        f"GetNextAvailRes() -> DocID: {fetched_doc_id}, Expected DocID: {doc_id}, "
                        f"NumRes: {fetched_num_res}, Expected NumRes: {num_results}, "
                        f"QueryIDs: {sorted(fetched_query_ids)}, Expected QueryIDs: {query_ids}, "
                        f"Matches: {'Yes' if is_doc_id_match and is_num_res_match and is_query_ids_match else 'No'}"
                    )
                    results.append(result_str)
                    output.write(result_str + "\n")
                else:
                    result_str = "GetNextAvailRes() -> No Available Results"
                    results.append(result_str)
                    output.write(result_str + "\n")
            else:
                results.append(f"Unknown action: {action}")
                output.write(results[-1] + "\n")
            pbar.update(1)
    return results
parse_and_execute("../big_test.txt", "big_out.txt")