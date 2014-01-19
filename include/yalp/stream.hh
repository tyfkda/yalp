//=============================================================================
/// Stream
//=============================================================================

#ifndef _STREAM_HH_
#define _STREAM_HH_

#include "yalp/config.hh"
#include "yalp/object.hh"
#include <stdio.h>  // for FILE

namespace yalp {

// Base class for stream.
class Stream {
public:
  virtual bool close();
  virtual int get() = 0;
  virtual void ungetc(int c) = 0;
  virtual bool write(char c);
  virtual bool write(const char* s);
  virtual bool write(const char* s, size_t len) = 0;

protected:
  Stream();
  virtual ~Stream();
};

// File stream class.
class FileStream : public Stream {
public:
  explicit FileStream(const char* filename, const char* mode);
  explicit FileStream(FILE* fp, bool ownership = false);
  ~FileStream();

  inline bool isOpened() const  { return fp_ != NULL; }

  virtual bool close() override;
  virtual int get() override;
  virtual void ungetc(int c) override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  FILE* fp_;
  bool hasFileOwnership_;
};

// String input stream class.
class StrStream : public Stream {
public:
  explicit StrStream(const char* string);
  ~StrStream();

  virtual bool close() override;
  virtual int get() override;
  virtual void ungetc(int c) override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  const char* p_;
  int ungetc_;
};

// String output stream class.
class StrOStream : public Stream {
public:
  explicit StrOStream(Allocator* allocator);
  ~StrOStream();

  inline const char* getString() const  { return buffer_; }
  inline int getLength() const  { return len_; }

  virtual bool close() override;
  virtual int get() override;
  virtual void ungetc(int c) override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  Allocator* allocator_;
  char* buffer_;
  size_t bufferSize_;
  int len_;
};

}  // namespace yalp

#endif
