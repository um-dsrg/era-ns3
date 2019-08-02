#ifndef switch_container_h
#define switch_container_h

#include <map>
#include <memory>

#include "switch-base.h"
#include "../../definitions.h"

// Forward declaration of the ResultsContainer class to avoid cyclic references
class ResultsContainer;

class SwitchContainer
{
public:
  using switchContainer_t = std::map<id_t, std::unique_ptr<SwitchBase>>;

  explicit SwitchContainer (uint64_t switchBufferSize, const std::string& txBufferRetrievalMethod);

  SwitchBase *GetSwitch (id_t switchId);
  bool SwitchExists (id_t switchId);
  void AddSwitch (id_t switchId, SwitchType switchType, ResultsContainer &resContainer);

  void ReconcileRoutingTables ();
  void EnablePacketReceptionOnSwitches ();
  void EnablePacketTransmissionTrace ();

  inline switchContainer_t::iterator begin ();
  inline switchContainer_t::iterator end ();

  inline switchContainer_t::const_iterator begin () const;
  inline switchContainer_t::const_iterator end () const;

private:
  switchContainer_t m_switchContainer;
  const uint64_t m_switchBufferSize;
  const std::string m_txBufferRetrievalMethod;
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

SwitchContainer::switchContainer_t::const_iterator
SwitchContainer::begin () const
{
  return m_switchContainer.begin ();
}

SwitchContainer::switchContainer_t::const_iterator
SwitchContainer::end () const
{
  return m_switchContainer.end ();
}

#endif /* switch_container_h */
