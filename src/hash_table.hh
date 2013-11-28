//=============================================================================
/// Hash table
/*
 * Map Key to Value in O(1) using hash function.
 *
 * Variable declaration:
 *   HashTable<K, V, P> ht;
 *
 * Put element:
 *   ht.put(k, v);
 *
 * Get element:
 *   V* pv = ht.get(k);
 *
 * Remove element for key:
 *   ht.remove(k);
 *
 * Each:
 *   for (HashTable<K, V, P>::Iterator it = ht.begin();
 *        it != ht.end(); ++it) {
 *     // Key access: it->key
 *     // Value access: it->value
 *   }
 *
 *
 * Policy:
 *   Policy class determines the behavior of hash table.
 *   it must implement following 2 static functions:
 *
 *   struct Policy {
 *     static unsigned int hash(const Key a) {
 *       // return some unsigned integer to distribute keys.
 *     }
 *     static bool equal(const Key a, const Key b) {
 *       // return true if 2 keys are same.
 *     }
 *   };
 */
//=============================================================================

#ifndef _HASH_TABLE_HH_
#define _HASH_TABLE_HH_

namespace yalp {

#ifndef NULL
#define NULL  (0)
#endif

// Hash table
template <class Key, class Value>
class HashTable {
public:
  struct Policy {
    // return some unsigned integer to distribute keys.
    virtual unsigned int hash(const Key a) = 0;
    // return true if 2 keys are same.
    virtual bool equal(const Key a, const Key b) = 0;
  };

  explicit HashTable(Policy* policy)
    : policy_(policy), array_(NULL), arraySize_(0)
    , entryCount_(0), conflictCount_(0) {
  }

  ~HashTable() {
    if (array_ != NULL)
      delete[] array_;
  }

  unsigned int getCapacity() const  { return arraySize_; }
  int getEntryCount() const  { return entryCount_; }
  int getConflictCount() const  { return conflictCount_; }
  int getMaxDepth() const  { return calcMaxDepth(); }

  void put(const Key key, const Value& value) {
    Link* link = find(key);
    if (link == NULL)
      link = createLink(key);
    link->value = value;
  }

  const Value* get(const Key key) const {
    Link* link = find(key);
    if (link == NULL)
      return NULL;
    return &link->value;
  }

  bool remove(const Key key) const {
    Link* prev;
    unsigned int index;
    Link* link = find(key, &prev, &index);
    if (link == NULL)
      return false;
    if (prev == NULL)
      array_[index] = link->next;
    else
      prev->next = link->next;
    delete link;
    return true;
  }

private:
  struct Link {
    Link* next;
    Key key;
    Value value;
  };

public:
  // Iterator
  class Iterator {
  public:
    const Iterator& operator++() {
      link = link->next;
      if (link == NULL) {
        do {
          if (++index >= ht->arraySize_)
            break;
          link = ht->array_[index];
        } while (link == NULL);
      }
      return *this;
    }

    bool operator==(const Iterator& it) const  { return link == it.link; }
    bool operator!=(const Iterator& it) const  { return link != it.link; }

    const Link* operator->() const  { return link; }

  private:
    Iterator(const HashTable* ht, unsigned int index, Link* link) {
      this->ht = ht;
      this->index = index;
      this->link = link;
    }

    const HashTable* ht;
    unsigned int index;
    Link* link;
    friend class HashTable;
  };

  Iterator begin() const {
    Link* link = NULL;
    unsigned int index = 0;
    for (index = 0; index < arraySize_; ++index) {
      link = array_[index];
      if (link != NULL)
        break;
    }
    Iterator it(this, index, link);
    return it;
  }
  Iterator end() const {
    return Iterator(this, arraySize_, NULL);
  }

private:
  Link* find(const Key key, Link** pPrev = NULL, unsigned int* pIndex = NULL) const {
    if (array_ == NULL)
      return NULL;
    unsigned int hash = policy_->hash(key);
    unsigned int index = hash % arraySize_;
    Link* prev = NULL;
    for (Link* link = array_[index]; link != NULL;
         prev = link, link = link->next) {
      if (policy_->equal(key, link->key)) {
        if (pPrev != NULL)
          *pPrev = prev;
        if (pIndex != NULL)
          *pIndex = index;
        return link;
      }
    }
    return NULL;
  }

  Link* createLink(const Key key) {
    if (array_ == NULL) {
      unsigned int newSize = 113;  // Prime number
      Link** newArray = new Link*[newSize];
      for (unsigned int i = 0; i < newSize; ++i)
        newArray[i] = NULL;
      array_ = newArray;
      arraySize_ = newSize;
    }
    unsigned int hash = policy_->hash(key);
    Link* link = new Link();
    unsigned int index = hash % arraySize_;
    link->next = array_[index];
    link->key = key;
    if (array_[index] != NULL)
      ++conflictCount_;
    array_[index] = link;
    ++entryCount_;
    return link;
  }

  int calcMaxDepth() const {
    int max = 0;
    for (unsigned int i = 0; i < arraySize_; ++i) {
      int depth = 0;
      for (Link* link = array_[i]; link != NULL; link = link->next)
        ++depth;
      if (depth > max)
        max = depth;
    }
    return max;
  }

  Policy* policy_;
  Link** array_;
  unsigned int arraySize_;
  int entryCount_;  // Number of entries.
  int conflictCount_;  // Number of hash index conflicts.
};

// Hash function
unsigned int strHash(const char* s);

}  // namespace yalp

#endif
