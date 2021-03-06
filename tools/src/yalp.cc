#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"

#include <assert.h>
#include <fcntl.h>  // O_RDONLY, O_WRONLY
#include <fstream>
#include <iostream>
#include <string.h>

#ifdef _MSC_VER
#include <io.h>
#define noexcept  // nothing
#define isatty  _isatty
#define dup  _dup
#define dup2  _dup2
#define fileno  _fileno
#define open  _open
#define fdopen  _fdopen
#else
#include <unistd.h>  // for isatty()
#endif

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
  ErrorCode err;
  for (;;) {
    int arena = state->saveArena();
    Value bin;
    err = reader.read(&bin);
    if (err != SUCCESS)
      break;
    if (!state->runBinary(bin, NULL))
      return false;
    state->restoreArena(arena);
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  return true;
}

static void replaceFile(FILE* fp, int fd) {
  fflush(fp);
  dup2(fd, fileno(fp));
}

static FILE* duplicateFile(FILE* fp, const char* mode) {
  return fdopen(dup(fileno(fp)), mode);
}

static int reopenFile(FILE* fp, const char* fileName, const char* mode) {
  int flags = mode[0] == 'w' ? O_WRONLY : O_RDONLY;
  int fd = open(fileName, flags);
  dup2(fd, fileno(fp));
  return fd;
}

static void writess(State* state, Value value, Value outStream) {
  Stream* stream = static_cast<SStream*>(outStream.toObject())->getStream();
  Value fn = state->referGlobal(state->intern("write/ss"));
  if (!fn.eq(Value::NIL)) {
    Value args[] = { value, outStream };
    state->funcall(fn, 2, args, NULL);
  } else {
    value.output(state, stream, true);
  }
  stream->write("\n");
}

static bool compile(State* state, Stream* stream, bool bNoRun, FILE* outFp) {
  int topArena = state->saveArena();
  Value outStream = state->createFileStream(outFp);
  Reader reader(state, stream);
  ErrorCode err;
  for (;;) {
    int arena = state->saveArena();
    Value exp;
    err = reader.read(&exp);
    if (err != SUCCESS)
      break;

    Value code;
    if (!state->compile(exp, &code) || code.isFalse()) {
      state->resetError();
      return false;
    }

    writess(state, code, outStream);
    if (!bNoRun && !state->runBinary(code, NULL))
      return false;
    state->restoreArena(arena);
  }
  if (err != END_OF_FILE) {
    cerr << "Read error: " << err << endl;
    return false;
  }
  state->restoreArena(topArena);
  return true;
}

static bool repl(State* state, Stream* stream, bool tty) {
  if (tty)
    cout << "type ':q' to quit" << endl;
  Value q = state->intern(":q");
  Value outStream = state->createFileStream(stdout);
  Reader reader(state, stream);
  for (;;) {
    int arena = state->saveArena();
    if (tty)
      cout << "> " << std::flush;
    Value exp;
    ErrorCode err = reader.read(&exp);
    if (err == END_OF_FILE || exp.eq(q))
      break;

    if (err != SUCCESS) {
      cerr << "Read error: " << err << endl;
      if (!tty)
        return false;
      continue;
    }
    Value code;
    if (!state->compile(exp, &code) || code.isFalse()) {
      state->resetError();
      if (!tty)
        return false;
      continue;
    }

    Value result;
    if (!state->runBinary(code, &result)) {
      if (!tty)
        return false;
      state->resetError();
      continue;
    }
    if (tty) {
      const char* prompt = "=> ";
      int n = state->getResultNum();
      if (n == 0) {
        cout << ";; No result value\n";
        continue;
      }

      // Calling write/ss function breaks multiple values, so need to preserve them.
      // TODO: Are these values safe from GC?
      Value* results = static_cast<Value*>(alloca(sizeof(Value) * n));
      for (int i = 0; i < n; ++i)
        results[i] = state->getResult(i);
      for (int i = 0; i < n; ++i) {
        Value result = results[i];
        cout << prompt;
        writess(state, result, outStream);
        prompt = "   ";
      }
    }
    state->restoreArena(arena);
  }
  if (tty)
    cout << "bye" << endl;
  return true;
}

static bool runMain(State* state, int argc, char** const argv, Value* pResult) {
  Value main = state->referGlobal("main");
  if (main.isFalse())
    return true;  // If no `main` function, it is ok.

  Value args = Value::NIL;
  for (int i = argc; --i >= 0; )
    args = state->cons(state->string(argv[i]), args);
  return state->funcall(main, 1, &args, pResult);
}

static void exitIfError(ErrorCode err) {
  switch (err) {
  case FILE_NOT_FOUND:
    cerr << "File not found\n";
    break;
  default:
    break;
  }
  if (err != SUCCESS)
    exit(1);
}

int main(int argc, char* argv[]) {
  State* state = State::create(&myAllocFunc);

  FILE* outFp = duplicateFile(stdout, "w");
  int tmpFd = -1;
  bool bLibraryLoaded = false;

  bool bDebug = false;
  bool bBinary = false;
  bool bCompile = false;
  bool bNoRun = false;
  const char* oneLinear = NULL;
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
    case 'e':
      if (++ii >= argc) {
        cerr << "'-e' takes parameter" << endl;
        exit(1);
      }
      oneLinear = argv[ii];
      break;
    case 'C':  // Compile, and not run the code.
      bCompile = true;
      bNoRun = true;
      break;
    case 'l':  // Library.
      if (!bLibraryLoaded) {
        bLibraryLoaded = true;
        state->runBinaryFromString(bootBinaryData);
      }
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
      if (argv[ii][0] != '\0')
        exitIfError(state->runBinaryFromFile(argv[ii]));
      bLibraryLoaded = true;
      break;
    default:
      cerr << "Unknown option: " << arg << endl;
      exit(1);
    }
  }

  if (!bLibraryLoaded)
    state->runBinaryFromString(bootBinaryData);

  if (bCompile && !bDebug)
    tmpFd = reopenFile(stdout, "/dev/null", "w");

  if (oneLinear != NULL) {
    StrStream stream(oneLinear);
    repl(state, &stream, false);
  } else if (ii >= argc) {
    FileStream stream(stdin);
    bool result;
    if (bBinary)        result = runBinary(state, &stream);
    else if (bCompile)  result = compile(state, &stream, bNoRun, outFp);
    else                result = repl(state, &stream, isatty(0) != 0);
    if (!result)
      exit(1);
  } else if (bCompile) {
    for (; ii < argc; ++ii) {
      if (strcmp(argv[ii], "--") == 0) {
        ++ii;
        break;
      }
      FileStream stream(argv[ii], "r");
      if (!stream.isOpened()) {
        std::cerr << "File not found: " << argv[ii] << std::endl;
        exit(1);
      }
      if (!compile(state, &stream, bNoRun, outFp))
        exit(1);
    }
  } else if (bBinary) {
    exitIfError(state->runBinaryFromFile(argv[ii++]));
  } else {
    exitIfError(state->runFromFile(argv[ii++]));
  }
  if (!bCompile)
    if (!runMain(state, argc - ii, &argv[ii], NULL))
      exit(1);

  fclose(outFp);
  if (tmpFd >= 0)
    replaceFile(stdout, tmpFd);

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
