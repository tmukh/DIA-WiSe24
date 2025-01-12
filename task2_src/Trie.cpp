#include "../include/Trie.hpp"

bool TrieNode::findNearest(const string& word, uint8_t i, uint8_t error, stack<pair<string, uint8_t>> &results) {
  if (leaf) {
    uint8_t res = (word.length() - 1 - i) + error;
    results.push({word, res});
    return true;
  }
  if (error == maxError){
    if (children[word[i]-'a'] == nullptr) return false;
    else return children[word[i]-'a']->findNearest(word, i+1, error, results);
  }
  else {
    for(unsigned int j = 0; j < 26; j++){
      if (children[j] != nullptr){
        if (i!=(word[i]-'a')) children[j]->findNearest(word, i+1, error+1, results);
        else children[j]->findNearest(word, i+1, error, results);
      }
    }
    return !results.empty();
  }
}

void Trie::insert(string word){
  TrieNode* node = root;
  int index = -1;
  for (char c : word) {
    index = c - 'a';
    if (!node->children[index]) node->children[index] = new TrieNode();
    node = node->children[index];
  }
  node->leaf = true;
}

void Trie::remove(string word){
  TrieNode* node = root;
  int index = -1;
  for (char c : word) {
    index = c - 'a';
    if (!node->children[index]) return;
    node = node->children[index];
  }
  if (node->leaf) node->leaf = false;
  if (all_of(begin(node->children), end(node->children),
             [](void* ptr) { return ptr == nullptr;})) node = nullptr;
}

bool Trie::search(string word, bool prefix){
  TrieNode* node = root;
  int index = -1;
  for (char c : word) {
    index = c - 'a';
    if (!node->children[index]) return false;
    node = node->children[index];
  }
  if (prefix) return true;
  return node->leaf;
}

void Trie::print(TrieNode* node, string prefix) const{
    if (node->leaf) cout << prefix << endl;
    for (int i = 0; i < 26; i++) {
        if (node->children[i]) {
            print(node->children[i],
                  prefix + char('a' + i));
        }
    }
}
void Trie::print() const { print(root, ""); }

bool Trie::nearest(const string& word){
  return root->findNearest(word, 0 , 0, results);
}