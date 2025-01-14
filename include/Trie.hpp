#ifndef TRIE_HPP
#define TRIE_HPP

#include <string>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "../include/Task2.h"

using namespace std;
class TrieNode {
  public:
    TrieNode* children[26];
    bool leaf;
    uint8_t maxError;
    uint8_t depth;
    TrieNode() : children{nullptr}, leaf(false), maxError(3){};
    bool findNearest(const string& word, uint8_t i, uint8_t error, unordered_map<string, uint8_t> &results, string& currentPrefix);
};

class Trie {
  private:
    TrieNode* root;
  public:
    unordered_map<string, uint8_t> results;
    Trie() : root(new TrieNode()) {}
    void insert(string word);
    void remove(string word);
    bool search(string word, bool prefix = false);
    bool nearest(const string& word);
    void print(TrieNode* node, string prefix) const;
    void print() const;
};


#endif //TRIE_HPP
