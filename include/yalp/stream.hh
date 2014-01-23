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
  int get();
  void ungetc(int c);
  virtual bool write(char c);
  virtual bool write(const char* s);
  virtual bool write(const char* s, size_t len) = 0;

  inline int getLineNumber() const  { return lineNo_; }

protected:
  Stream();
  virtual ~Stream();

  virtual int doGet() = 0;

  int lineNo_;
  int ungetc_;
};

// File stream class.
class FileStream : public Stream {
public:
  explicit FileStream(const char* filename, const char* mode);
  explicit FileStream(FILE* fp, bool ownership = false);
  ~FileStream();

  inline bool isOpened() const  { return fp_ != NULL; }

  virtual bool close() override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  virtual int doGet() override;

  FILE* fp_;
  bool hasFileOwnership_;
};

// String input stream class.
class StrStream : public Stream {
public:
  explicit StrStream(const char* string);
  ~StrStream();

  virtual bool close() override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  virtual int doGet() override;

  const char* p_;
};

// String output stream class.
class StrOStream : public Stream {
public:
  explicit StrOStream(Allocator* allocator);
  ~StrOStream();

  inline const char* getString() const  { return buffer_; }
  inline int getLength() const  { return len_; }

  virtual bool close() override;
  using Stream::write;
  virtual bool write(const char* s, size_t len) override;

private:
  virtual int doGet() override;

  Allocator* allocator_;
  char* buffer_;
  size_t bufferSize_;
  int len_;
};

}  // namespace yalp

#endif
