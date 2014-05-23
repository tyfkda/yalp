#include "yalp/object.hh"
#include <iostream>

using namespace std;
using namespace yalp;

#define DUMP(klass) \
  (cout << #klass << " = " << sizeof(klass) << " bytes\n")

int main() {
  DUMP(GcObject);
  DUMP(Object);
  DUMP(Cell);
  DUMP(String);
  DUMP(SFlonum);
  DUMP(Vector);
  DUMP(SHashTable);
  DUMP(Callable);
  DUMP(Closure);
  DUMP(Macro);
  DUMP(NativeFunc);
  DUMP(Continuation);
  DUMP(SStream);
  return 0;
}
