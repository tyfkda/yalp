//=============================================================================
/// Stream
//=============================================================================

#include "yalp/stream.hh"
#include <string.h>  // for strlen

namespace yalp {

const int NO_UNGETC = -1;

//=============================================================================
// SStream
SStream::~SStream()  { close(); }
void SStream::destruct(Allocator*)  { close(); }

bool SStream::close()  { return true; }

Type SStream::getType() const  { return TT_STREAM; }

void SStream::output(State*, SStream* o, bool) const {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "#<stream:%p>", this);
  o->write(buffer);
}

//=============================================================================
FileStream::FileStream(const char* filename, const char* mode)
  : SStream(), hasFileOwnership_(true) {
  fp_ = fopen(filename, mode);
}

FileStream::FileStream(FILE* fp, bool ownership)
  : SStream(), fp_(fp), hasFileOwnership_(ownership) {
}

FileStream::~FileStream()  { close(); }

bool FileStream::close() {
  if (!hasFileOwnership_ || fp_ == NULL)
    return false;
  fclose(fp_);
  fp_ = NULL;
  hasFileOwnership_ = false;
  return true;
}

int FileStream::get() {
  return fgetc(fp_);
}

void FileStream::putback(int c) {
  ungetc(c, fp_);
}

bool FileStream::write(char c) {
  return fputc(c, fp_) != EOF;
}

bool FileStream::write(const char* s) {
  size_t len = strlen(s);
  return fwrite(s, 1, len, fp_) == len;
}

//=============================================================================
StrStream::StrStream(const char* string)
  : SStream(), string_(string), p_(string), ungetc_(NO_UNGETC) {
}

StrStream::~StrStream() {
}

bool StrStream::close() {
  return true;
}

int StrStream::get() {
  if (ungetc_ != NO_UNGETC) {
    int c = ungetc_;
    ungetc_ = NO_UNGETC;
    return c;
  }
  int c = *p_++;
  if (c == '\0') {
    --p_;
    c = EOF;
  }
  return c;
}

void StrStream::putback(int c) {
  ungetc_ = c;
}

bool StrStream::write(char) {
  return false;
}

bool StrStream::write(const char*) {
  return false;
}

}  // namespace yalp
