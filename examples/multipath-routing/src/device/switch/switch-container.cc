#include "ns3/log.h"
#include "ns3/abort.h"

#include "sdn-switch.h"
#include "ppfs-switch.h"
#include "switch-container.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchContainer");

void
SwitchContainer::AddSwitch (id_t switchId, SwitchType switchType)
{
  std::pair<switchContainer_t::iterator, bool> ret;
  if (switchType == SwitchType::SdnSwitch)
    {
      ret =
          m_switchContainer.emplace (switchId, std::make_unique<SdnSwitch> (SdnSwitch (switchId)));
    }
  else if (switchType == SwitchType::PpfsSwitch)
    {
      ret = m_switchContainer.emplace (switchId,
                                       std::make_unique<PpfsSwitch> (PpfsSwitch (switchId)));
    }
  else
    {
      NS_ABORT_MSG ("Unknown switch type given");
    }
  NS_LOG_INFO ("Switch: " << switchId << " created");
  NS_ABORT_MSG_IF (ret.second == false,
                   "Trying to insert a duplicate switch with id: " << switchId);
}

SwitchBase *
SwitchContainer::GetSwitch (id_t switchId)
{
  return m_switchContainer.at (switchId).get ();
}
