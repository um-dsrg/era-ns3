#ifndef switch_container_h
#define switch_container_h

#include <map>
#include <memory>

#include "switch-base.h"
#include "../../definitions.h"

class SwitchContainer
{
public:
  void AddSwitch (id_t switchId, SwitchType switchType);
  SwitchBase const *GetSwitch (id_t switchId);

private:
  std::map<id_t, std::unique_ptr<SwitchBase>> m_switchContainer;
};

#endif /* switch_container_h */
