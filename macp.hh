//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#ifndef _MACP_HH_
#define _MACP_HH_

namespace macp {

class State {
public:
  static State* create();
  virtual ~State();

private:
  State();
};

}  // namespace macp

#endif
