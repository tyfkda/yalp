//=============================================================================
/// Error code
//=============================================================================

#ifndef _ERROR_CODE_HH_
#define _ERROR_CODE_HH_

namespace yalp {

enum ErrorCode {
  SUCCESS,

  // Read
  END_OF_FILE,
  NO_CLOSE_PAREN,
  EXTRA_CLOSE_PAREN,
  NO_CLOSE_STRING,
  DOT_AT_BASE,  // `.' is appeared at base text.
  ILLEGAL_CHAR,

  COMPILE_ERROR,

  // Run
  FILE_NOT_FOUND,
  RUNTIME_ERROR,
};

}  // namespace yalp

#endif
