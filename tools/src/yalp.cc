#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <unistd.h>  // for isatty()

#ifdef LEAK_CHECK
#include <map>
#include <string>
#include <utility>
#endif

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

static int nalloc, nfree;
#ifdef LEAK_CHECK
static map<void*, pair<string, int>> allocated;
static void* myAllocFunc(void* p, size_t size, const char* filename, int line)
#else
static void* myAllocFunc(void* p, size_t size)
#endif
{
  if (size <= 0) {
    free(p);
    ++nfree;
#ifdef LEAK_CHECK
    //allocated.erase(allocated.find(p));
#endif
    return NULL;
  }

  if (p == NULL) {
    ++nalloc;
    void* q = malloc(size);
#ifdef LEAK_CHECK
    allocated[q] = make_pair(filename ? filename : "unknown", line);
#endif
    return q;
  }
  void* q = realloc(p, size);
#ifdef LEAK_CHECK
  allocated.erase(allocated.find(p));
  allocated[q] = make_pair(filename ? filename : "unknown", line);
#endif
  return q;
};

#ifdef LEAK_CHECK
static void dumpLeakedMemory() {
  map<string, int> left;
  for (auto p : allocated)
    left[p.second.first + "(" + to_string(p.second.second) + ")"] += 1;
  cout << "Left: " << allocated.size() << endl;
  for (auto p : left)
    cout << "  " << p.first << ": #" << p.second << endl;
}
#endif

static bool runBinary(State* state, Stream* stream) {
  Reader reader(state, stream);
  Svalue bin;
  ErrorCode err;
  while ((err = reader.read(&bin)) == SUCCESS) {
    if (!state->runBinary(bin, NULL))
      return false;
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  return true;
}

static bool compileFile(State* state, const char* filename, bool bNoRun) {
  FileStream stream(filename, "r");
  if (!stream.isOpened()) {
    std::cerr << "File not found: " << filename << std::endl;
    return false;
  }

  Svalue writess = state->referGlobal(state->intern("write/ss"));
  Reader reader(state, &stream);
  Svalue exp;
  ErrorCode err;
  while ((err = reader.read(&exp)) == SUCCESS) {
    Svalue code;
    if (!state->compile(exp, &code) || state->isFalse(code)) {
      state->resetError();
      return false;
    }
    state->funcall(writess, 1, &code, NULL);
    cout << endl;

    if (!bNoRun && !state->runBinary(code, NULL))
      return false;
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  return true;
}

static bool repl(State* state, Stream* stream, bool tty, bool bCompile, bool bNoRun) {
  if (tty)
    cout << "type ':q' to quit" << endl;
  Svalue q = state->intern(":q");
  Svalue writess = state->referGlobal(state->intern("write/ss"));
  FileStream out(stdout);
  Reader reader(state, stream);
  for (;;) {
    if (tty)
      cout << "> " << std::flush;
    Svalue s;
    ErrorCode err = reader.read(&s);
    if (err == END_OF_FILE || s.eq(q))
      break;

    if (err != SUCCESS) {
      cerr << "Read error: " << err << endl;
      if (tty)
        continue;
      return false;
    }
    Svalue code;
    if (!state->compile(s, &code) || state->isFalse(code)) {
      state->resetError();
      if (tty)
        continue;
      else
        return false;
    }

    if (bCompile) {
      state->funcall(writess, 1, &code, NULL);
      cout << endl;
    }

    Svalue result;
    if (!bNoRun && !state->runBinary(code, &result)) {
      if (!tty)
        return false;
      state->resetError();
      continue;
    }
    if (!bCompile && tty) {
      cout << "=> ";
      result.output(state, &out, true);
      cout << endl;
    }
  }
  if (tty)
    cout << "bye" << endl;
  return true;
}

static bool runMain(State* state) {
  Svalue main = state->referGlobal(state->intern("main"));
  if (!state->isTrue(main))
    return true;
  return state->funcall(main, 0, NULL, NULL);
}

static void exitIfError(ErrorCode err) {
  const char* msg = NULL;
  switch (err) {
  case SUCCESS:
    return;
  case END_OF_FILE:  msg = "End of file"; break;
  case NO_CLOSE_PAREN:  msg = "No close paren"; break;
  case EXTRA_CLOSE_PAREN:  msg = "Extra close paren"; break;
  case DOT_AT_BASE:  msg = "Dot at base"; break;
  case ILLEGAL_CHAR:  msg = "Illegal char"; break;
  case COMPILE_ERROR:  msg = "Compile error"; break;
  case FILE_NOT_FOUND:  msg = "File not found"; break;
  case RUNTIME_ERROR:  return;  // Error message is already printed when runtime error.
  default:
    assert(!"Not handled");
    msg = "Unknown error";
    break;
  }
  std::cout << msg << std::endl;
  exit(1);
}

int main(int argc, char* argv[]) {
  State* state = State::create(&myAllocFunc);

  bool bDebug = false;
  bool bBinary = false;
  bool bCompile = false;
  bool bNoRun = false;
  int ii;
  for (ii = 1; ii < argc; ++ii) {
    char* arg = argv[ii];
    if (arg[0] != '-')
      break;
    switch (arg[1]) {
    case 'd':
      bDebug = true;
      break;
    case 'b':
      bBinary = true;
      break;
    case 'c':
      bCompile = true;
      break;
    case 'C':  // Compile, and not run the code.
      bCompile = true;
      bNoRun = true;
      break;
    case 'l':  // Library.
      if (++ii >= argc) {
        cerr << "'-l' takes parameter" << endl;
        exit(1);
      }
      exitIfError(state->runFromFile(argv[ii]));
      break;
    case 'L':  // Binary library.
      if (++ii >= argc) {
        cerr << "'-L' takes parameter" << endl;
        exit(1);
      }
      exitIfError(state->runBinaryFromFile(argv[ii]));
      break;
    default:
      cerr << "Unknown option: " << arg << endl;
      exit(1);
    }
  }

  if (ii >= argc) {
    FileStream stream(stdin);
    if (bBinary) {
      if (!runBinary(state, &stream))
        exit(1);
    } else {
      if (!repl(state, &stream, isatty(0) != 0, bCompile, bNoRun))
        exit(1);
    }
  } else {
    for (int i = ii; i < argc; ++i) {
      if (bCompile) {
        if (!compileFile(state, argv[i], bNoRun))
          exit(1);
      } else if (bBinary) {
        exitIfError(state->runBinaryFromFile(argv[i]));
      } else {
        exitIfError(state->runFromFile(argv[i]));
      }
    }
  }
  if (!bCompile)
    if (!runMain(state))
      exit(1);

  if (bDebug)
    state->reportDebugInfo();
  state->release();

  if (bDebug) {
    cout << "Memory allocation:" << endl;
    cout << "  #new: " << nnew << ", #delete: " << ndelete << endl;
    cout << "  #alloc: " << nalloc << ", #free: " << nfree << endl;
#ifdef LEAK_CHECK
    dumpLeakedMemory();
#endif
  }
  return 0;
}
