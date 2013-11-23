#include "yalp.hh"
#include "yalp/mem.hh"
#include "yalp/read.hh"
#include <fstream>
#include <iostream>

using namespace std;
using namespace yalp;

static int nnew, ndelete;

void* operator new(size_t size) {
  ++nnew;
  return malloc(size);
}

void operator delete(void* p) noexcept {
  if (p == NULL)
    cout << "delete called for NULL" << endl;
  ++ndelete;
  free(p);
}

void* operator new[](size_t size) {
  ++nnew;
  return malloc(size);
}

void operator delete[](void* p) noexcept {
  if (p == NULL)
    cout << "delete called for NULL" << endl;
  ++ndelete;
  free(p);
}

class MyAllocator : public Allocator {
public:
  int nalloc, nfree;
  MyAllocator() : nalloc(0), nfree(0) {}

  virtual void* alloc(size_t size) override {
    ++nalloc;
    return ::malloc(size);
  }
  virtual void* realloc(void* p, size_t size) override {
    if (p == NULL)
      ++nalloc;
    return ::realloc(p, size);
  }
  virtual void free(void* p) override {
    ::free(p);
    ++nfree;
  }
};

void runBinary(State* state, std::istream& istrm) {
  Reader reader(state, istrm);

  Svalue bin;
  ReadError err;
  while ((err = reader.read(&bin)) == READ_SUCCESS) {
    state->runBinary(bin);
  }
  if (err != END_OF_FILE)
    cerr << "Read error: " << err << endl;
}

int main(int argc, char* argv[]) {
  MyAllocator myAllocator;
  State* state = State::create(&myAllocator);

  bool bOutMemResult = false;
  int ii;
  for (ii = 1; ii < argc; ++ii) {
    char* arg = argv[ii];
    if (arg[0] != '-')
      break;
    switch (arg[1]) {
    case 'm':
      bOutMemResult = true;
      break;
    default:
      cerr << "Unknown option: " << arg << endl;
    }
  }

  if (ii >= argc) {
    runBinary(state, cin);
  } else {
    std::ifstream strm(argv[ii]);
    runBinary(state, strm);
  }

  state->release();

  if (bOutMemResult) {
    printf("#new: %d, #delete: %d\n", nnew, ndelete);
    printf("#alloc: %d, #free: %d\n", myAllocator.nalloc, myAllocator.nfree);
  }
  return 0;
}
