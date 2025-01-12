#ifndef TRIE_HPP
#define TRIE_HPP

#include <string>
#include <algorithm>
#include <iostream>
#include <stack>
#include <utility>
#include "../include/Task2.h"

using namespace std;
class TrieNode {
  public:
    TrieNode* children[26];
    bool leaf;
    uint8_t maxError;
    TrieNode() : children{nullptr}, leaf(false), maxError(3){};
    bool findNearest(const string& word, uint8_t i, uint8_t error, stack<pair<string, uint8_t>> &results);
};

class Trie {
  private:
    TrieNode* root;
  public:
    stack<pair<string, uint8_t>> results;
    Trie() : root(new TrieNode()) {}
    void insert(string word);
    void remove(string word);
    bool search(string word, bool prefix = false);
    bool nearest(const string& word);
    void print(TrieNode* node, string prefix) const;
    void print() const;
};


#endif //TRIE_HPP
