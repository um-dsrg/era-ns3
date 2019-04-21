#ifndef switch_container_h
#define switch_container_h

#include <map>
#include <memory>

#include "switch-base.h"
#include "../../definitions.h"

class SwitchContainer
{
public:
  using switchContainer_t = std::map<id_t, std::unique_ptr<SwitchBase>>;

  explicit SwitchContainer (uint64_t switchBufferSize);

  SwitchBase *GetSwitch (id_t switchId);
  bool SwitchExists (id_t switchId);
  void AddSwitch (id_t switchId, SwitchType switchType);

  void ReconcileRoutingTables ();
  void EnablePacketReceptionOnSwitches ();
  void EnablePacketTransmissionTrace ();

  inline switchContainer_t::iterator begin ();
  inline switchContainer_t::iterator end ();

private:
  switchContainer_t m_switchContainer;
  const uint64_t m_switchBufferSize;
};

SwitchContainer::switchContainer_t::iterator
SwitchContainer::begin ()
{
  return m_switchContainer.begin ();
}

SwitchContainer::switchContainer_t::iterator
SwitchContainer::end ()
{
  return m_switchContainer.end ();
}

#endif /* switch_container_h */
