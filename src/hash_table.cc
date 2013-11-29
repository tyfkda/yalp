#include "hash_table.hh"

namespace yalp {

unsigned int strHash(const char* s) {
  unsigned int v = 0;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
       *p != '\0'; ++p)
    v = v * 17 + 1 + *p;
  return v;
}

}  // namespace yalp
