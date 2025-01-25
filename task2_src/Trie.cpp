#include "../include/Trie.hpp"
bool TrieNode::findNearest(const string& word,
                           uint8_t i, uint8_t error,
                           unordered_map<string, uint8_t> &results,
                           string& currentPrefix) {
  if (leaf) {
    // if the searched word is longer than the leaf.
    uint8_t length = word.length();
    uint8_t res = abs((int)(length - i)) + error;
    if (res <= maxError){
      if (results.find(currentPrefix)==results.end()) results[currentPrefix] = res;
      else results[currentPrefix] = min(results[currentPrefix], res);
    }
  }
  if (i>=word.length()){
    if (error>=maxError) return false;
    // if the searched word is shorter than the leaf
    for(int j = 0; j < 26; j++){
      if (children[j] != nullptr){
        char letter = 'a' + j;
        currentPrefix.push_back(letter);
        // Deletion
        children[j]->findNearest(word, i, error+1, results, currentPrefix);
        currentPrefix.pop_back();
      }
    }
  }
  if (error >= maxError){
    // Cant make any more mistakes
    if (children[word[i]-'a'] == nullptr) return false;
    // Correct moves
    else {
      currentPrefix.push_back(word[i]);
      bool temp = children[word[i]-'a']->findNearest(word, i+1, error, results, currentPrefix);
      currentPrefix.pop_back();
      return temp;
    }
  }
  else {
    for(int j = 0; j < 26; j++){
      if (children[j] != nullptr){
        char letter = 'a' + j;
        currentPrefix.push_back(letter);
        // Correct letter
        if (j == (word[i]-'a')) children[j]->findNearest(word, i+1, error, results, currentPrefix);
        // Incorrect letter, which warrants insertion, deletion and change
        // change
        children[j]->findNearest(word, i+1, error+1, results, currentPrefix);
        // Deletion: The document word has this letter deleted
        children[j]->findNearest(word, i, error+1, results, currentPrefix);
        // Insertion: The document word has extra letters before this letter so we need to push the index further
        size_t index = word.find(letter, i);
        if (index != string::npos && index > i) {
          children[j]->findNearest(word, index+1, error+index-i, results, currentPrefix);
        }
        currentPrefix.pop_back();
      }
    }
    return !results.empty();
  }
}
void Trie::insert(string word){
  TrieNode* node = root;
  int index = -1;
  int depth = 0;
  for (char c : word) {
    index = c - 'a';
    node->depth = depth;
    if (!node->children[index]) node->children[index] = new TrieNode();
    node = node->children[index];
    depth++;
  }
  node->leaf = true;
  node->depth = depth;


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
  string prefix;
  return root->findNearest(word, 0 , 0, results, prefix);
}