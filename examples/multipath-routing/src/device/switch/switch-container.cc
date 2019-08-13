#include "ns3/log.h"
#include "ns3/abort.h"

#include "sdn-switch.h"
#include "ppfs-switch.h"
#include "switch-container.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchContainer");

SwitchContainer::SwitchContainer (uint64_t switchBufferSize) : m_switchBufferSize (switchBufferSize)
{
}

SwitchBase *
SwitchContainer::GetSwitch (id_t switchId)
{
  return m_switchContainer.at (switchId).get ();
}

bool
SwitchContainer::SwitchExists (id_t switchId)
{
  if (m_switchContainer.find (switchId) == m_switchContainer.end ())
    return false;
  else
    return true;
}

void
SwitchContainer::AddSwitch (id_t switchId, SwitchType switchType, ResultsContainer &resContainer)
{
  NS_LOG_INFO ("Adding switch " << switchId);
  std::pair<switchContainer_t::iterator, bool> ret;
  if (switchType == SwitchType::SdnSwitch)
    {
      ret = m_switchContainer.emplace (
          switchId, std::make_unique<SdnSwitch> (SdnSwitch (switchId, m_switchBufferSize)));
    }
  else if (switchType == SwitchType::PpfsSwitch)
    {
      ret = m_switchContainer.emplace (
          switchId, std::make_unique<PpfsSwitch> (PpfsSwitch (switchId, m_switchBufferSize)));
    }
  else
    {
      NS_ABORT_MSG ("Unknown switch type given");
    }
  NS_ABORT_MSG_IF (ret.second == false,
                   "Trying to insert a duplicate switch with id: " << switchId);
}

void
SwitchContainer::SetupSwitches ()
{
  EnablePacketTransmissionTrace ();
  EnablePacketReceptionOnSwitches ();

  // Reconcile the routing tables
  ReconcileRoutingTables ();
}

void
SwitchContainer::ReconcileRoutingTables ()
{
  NS_LOG_INFO ("Reconciling the routing tables.\n"
               "NOTE This function should only be invoked for PPFS switches");

  for (auto &switchPair : m_switchContainer)
    {
      auto &switchObject{switchPair.second};
      switchObject->ReconcileSplitRatios ();
    }
}

void
SwitchContainer::EnablePacketReceptionOnSwitches ()
{
  NS_LOG_INFO ("Enabling Packet reception on all switches");
  for (auto &switchPair : m_switchContainer)
    {
      auto &switchInstance = switchPair.second;
      switchInstance->SetPacketReception ();
    }
}

void
SwitchContainer::EnablePacketTransmissionTrace ()
{
  NS_LOG_INFO ("Enabling packet transmission complete trace on all switches");

  for (auto &switchPair : m_switchContainer)
    {
      auto &switchInstance = switchPair.second;
      switchInstance->EnablePacketTransmissionCompletionTrace ();
    }
}
