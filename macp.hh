//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#ifndef _MACP_HH_
#define _MACP_HH_

namespace macp {

// S-value: bit embedded type value.
typedef int Svalue;

// State class.
class State {
public:
  static State* create();
  virtual ~State();

  Svalue readString(const char* str);

private:
  State();
};

}  // namespace macp

#endif
