//=============================================================================
/// Stream
//=============================================================================

#include "yalp/stream.hh"

namespace yalp {

const int NO_UNGETC = -1;

//=============================================================================
// SStream
SStream::~SStream()  { close(); }
void SStream::destruct(Allocator*)  { close(); }

bool SStream::close()  { return true; }

Type SStream::getType() const  { return TT_STREAM; }

void SStream::output(State*, std::ostream& o, bool) const {
  o << "#<stream:" << this << ">";
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

}  // namespace yalp
