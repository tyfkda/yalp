//=============================================================================
/// Hash table
//=============================================================================

#ifndef _HASH_TABLE_HH_
#define _HASH_TABLE_HH_

namespace yalp {

#ifndef NULL
#define NULL  (0)
#endif

// Hash table
template <class Key, class Value, class Policy>
class HashTable {
public:
  explicit HashTable(const Policy& policy)
    : policy_(policy)
    , array_(NULL)
    , arraySize_(0) {
  }

  ~HashTable() {
    if (array_ != NULL)
      delete[] array_;
  }

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

  Link* find(const Key key, Link** pPrev = NULL, unsigned int* pIndex = NULL) const {
    if (array_ == NULL)
      return NULL;
    unsigned int hash = policy_.hash(key);
    unsigned int index = hash % arraySize_;
    Link* prev = NULL;
    for (Link* link = array_[index]; link != NULL;
         prev = link, link = link->next) {
      if (policy_.equal(key, link->key)) {
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
    unsigned int hash = policy_.hash(key);
    Link* link = new Link();
    unsigned int index = hash % arraySize_;
    link->next = array_[index];
    link->key = key;
    array_[index] = link;
    return link;
  }

  Policy policy_;
  Link** array_;
  unsigned int arraySize_;
};

}  // namespace yalp

#endif
