#include "yalp.hh"
#include "yalp/mem.hh"
#include "yalp/read.hh"
#include <fstream>
#include <iostream>
#include <unistd.h>  // for isatty()

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

static void runBinary(State* state, std::istream& istrm) {
  Reader reader(state, istrm);

  Svalue bin;
  ReadError err;
  while ((err = reader.read(&bin)) == READ_SUCCESS) {
    state->runBinary(bin);
  }
  if (err != END_OF_FILE)
    cerr << "Read error: " << err << endl;
}

static void compileFile(State* state, const char* filename) {
  std::ifstream istrm(filename);
  Reader reader(state, istrm);

  Svalue exp;
  ReadError err;
  while ((err = reader.read(&exp)) == READ_SUCCESS) {
    Svalue code = state->compile(exp);
    code.output(state, cout);
    cout << endl;
  }
  if (err != END_OF_FILE)
    cerr << "Read error: " << err << endl;
}

static void repl(State* state, std::istream& istrm, bool tty, bool bCompileOnly) {
  if (tty)
    cout << "type ':q' to quit" << endl;
  Svalue q = state->intern(":q");
  Reader reader(state, istrm);
  for (;;) {
    if (tty)
      cout << "> " << std::flush;
    Svalue s;
    ReadError err = reader.read(&s);
    if (err == END_OF_FILE || s.eq(q))
      break;
    Svalue result;
    result = state->compile(s);
    if (!bCompileOnly)
      result = state->runBinary(result);
    if (tty || bCompileOnly) {
      result.output(state, cout);
      cout << endl;
    }
  }
  if (tty)
    cout << "bye" << endl;
}

int main(int argc, char* argv[]) {
  MyAllocator myAllocator;
  State* state = State::create(&myAllocator);

  bool bOutMemResult = false;
  bool bBinary = false;
  bool bCompileOnly = false;
  int ii;
  for (ii = 1; ii < argc; ++ii) {
    char* arg = argv[ii];
    if (arg[0] != '-')
      break;
    switch (arg[1]) {
    case 'm':
      bOutMemResult = true;
      break;
    case 'b':
      bBinary = true;
      break;
    case 'c':
      bCompileOnly = true;
      break;
    default:
      cerr << "Unknown option: " << arg << endl;
    }
  }

  state->runBinaryFromFile("boot.bin");

  if (ii >= argc) {
    if (bBinary)
      runBinary(state, cin);
    else
      repl(state, cin, isatty(0), bCompileOnly);
  } else {
    for (int i = ii; i < argc; ++i) {
      if (bCompileOnly)
        compileFile(state, argv[i]);
      else if (bBinary)
        state->runBinaryFromFile(argv[i]);
      else
        state->runFromFile(argv[i]);
    }
  }

  state->release();

  if (bOutMemResult) {
    printf("#new: %d, #delete: %d\n", nnew, ndelete);
    printf("#alloc: %d, #free: %d\n", myAllocator.nalloc, myAllocator.nfree);
  }
  return 0;
}
