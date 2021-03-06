//=============================================================================
/// Stream
//=============================================================================

#include "build_env.hh"
#include "yalp/stream.hh"
#include "allocator.hh"

#include <string.h>  // for strlen

namespace yalp {

const int NO_UNGETC = -1;

//=============================================================================
// Stream
Stream::Stream() : lineNo_(1), ungetc_(NO_UNGETC)  {}
Stream::~Stream()  { close(); }
bool Stream::close()  { return true; }

int Stream::get() {
  if (ungetc_ != NO_UNGETC) {
    int c = ungetc_;
    ungetc_ = NO_UNGETC;
    return c;
  }

  int c = doGet();
  if (c == '\n')
    ++lineNo_;
  return c;
}

void Stream::ungetc(int c) {
  ungetc_ = c;
}

bool Stream::write(char c)  { return write(&c, sizeof(c)); }
bool Stream::write(const char* s)  {return write(s, strlen(s)); }

//=============================================================================
FileStream::FileStream(const char* filename, const char* mode)
  : Stream(), hasFileOwnership_(true) {
  fp_ = fopen(filename, mode);
}

FileStream::FileStream(FILE* fp, bool ownership)
  : Stream(), fp_(fp), hasFileOwnership_(ownership) {
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

int FileStream::doGet() {
  return fgetc(fp_);
}

bool FileStream::write(const char* s, size_t len) {
  return fwrite(s, 1, len, fp_) == len;
}

//=============================================================================
StrStream::StrStream(const char* string)
  : Stream(), string_(string), p_(string) {
}

bool StrStream::close() {
  return true;
}

int StrStream::doGet() {
  int c = *p_++;
  if (c == '\0') {
    --p_;
    c = EOF;
  }
  return c;
}

bool StrStream::write(const char*, size_t) {
  return false;
}

//=============================================================================
static const size_t BUFFER_BLOCK_SIZE = 256;

StrOStream::StrOStream(Allocator* allocator)
  : Stream(), allocator_(allocator)
  , buffer_(NULL), bufferSize_(0), len_(0)  {}
StrOStream::~StrOStream() {
  if (buffer_ != NULL)
    allocator_->free(buffer_);
}

bool StrOStream::close()  { return true; }

int StrOStream::doGet()  { return EOF; }
bool StrOStream::write(const char* s, size_t len) {
  size_t newLen = len_ + len;
  if (newLen >= bufferSize_) {
    size_t newSize = newLen + (BUFFER_BLOCK_SIZE - 1);
    newSize -= newSize % BUFFER_BLOCK_SIZE;
    buffer_ = static_cast<char*>(allocator_->realloc(buffer_, newSize));
    bufferSize_ = newSize;
  }
  memcpy(&buffer_[len_], s, len);
  len_ += len;
  buffer_[len_] = '\0';
  return true;
}

}  // namespace yalp
