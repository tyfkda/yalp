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

static bool runBinary(State* state, std::istream& strm) {
  Reader reader(state, strm);
  Svalue bin;
  ReadError err;
  while ((err = reader.read(&bin)) == READ_SUCCESS) {
    state->runBinary(bin);
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  return true;
}

static bool compileFile(State* state, const char* filename) {
  std::ifstream strm(filename);
  if (!strm.is_open()) {
    std::cerr << "File not found: " << filename << std::endl;
    return false;
  }

  Svalue writess = state->referGlobal(state->intern("write/ss"));
  Reader reader(state, strm);
  Svalue exp;
  ReadError err;
  while ((err = reader.read(&exp)) == READ_SUCCESS) {
    Svalue code;
    if (!state->compile(exp, &code)) {
      cerr << "`compile` is not enabled" << endl;
      return false;
    }
    state->funcall(writess, 1, &code);
    cout << endl;

    state->runBinary(code);
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  return true;
}

static bool repl(State* state, std::istream& istrm, bool tty, bool bCompile) {
  if (tty)
    cout << "type ':q' to quit" << endl;
  Svalue q = state->intern(":q");
  Svalue writess = state->referGlobal(state->intern("write/ss"));
  Reader reader(state, istrm);
  for (;;) {
    if (tty)
      cout << "> " << std::flush;
    Svalue s;
    ReadError err = reader.read(&s);
    if (err == END_OF_FILE || s.eq(q))
      break;

    if (err != READ_SUCCESS)
      return false;
    Svalue code;
    if (!state->compile(s, &code)) {
      cerr << "`compile` is not enabled" << endl;
      return false;
    }
    Svalue result = state->runBinary(code);
    if (bCompile) {
      state->funcall(writess, 1, &code);
      cout << endl;
    } else if (tty) {
      result.output(state, cout, true);
      cout << endl;
    }
  }
  if (tty)
    cout << "bye" << endl;
  return true;
}

static Svalue runMain(State* state) {
  Svalue main = state->referGlobal(state->intern("main"));
  if (!state->isTrue(main))
    return main;
  return state->funcall(main, 0, NULL);
}

int main(int argc, char* argv[]) {
  MyAllocator myAllocator;
  State* state = State::create(&myAllocator);

  bool bOutMemResult = false;
  bool bBinary = false;
  bool bCompile = false;
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
      bCompile = true;
      break;
    case 'l':  // Library.
      if (++ii >= argc) {
        cerr << "'-l' takes parameter" << endl;
        exit(1);
      }
      state->runFromFile(argv[ii]);
      break;
    case 'L':  // Binary library.
      if (++ii >= argc) {
        cerr << "'-L' takes parameter" << endl;
        exit(1);
      }
      state->runBinaryFromFile(argv[ii]);
      break;
    default:
      cerr << "Unknown option: " << arg << endl;
      exit(1);
    }
  }

  if (ii >= argc) {
    if (bBinary) {
      if (!runBinary(state, cin))
        exit(1);
    } else {
      if (!repl(state, cin, isatty(0), bCompile))
        exit(1);
    }
  } else {
    for (int i = ii; i < argc; ++i) {
      if (bCompile) {
        if (!compileFile(state, argv[i]))
          exit(1);
      } else if (bBinary) {
        if (!state->runBinaryFromFile(argv[i]))
          exit(1);
      } else {
        if (!state->runFromFile(argv[i]))
          exit(1);
      }
    }
  }
  if (!bCompile)
    runMain(state);

  state->release();

  if (bOutMemResult) {
    printf("#new: %d, #delete: %d\n", nnew, ndelete);
    printf("#alloc: %d, #free: %d\n", myAllocator.nalloc, myAllocator.nfree);
  }
  return 0;
}
