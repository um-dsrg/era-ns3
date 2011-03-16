/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo  <nbaldo@cttc.es>
 */


#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/packet.h>

#include "lte-amc.h"
#include "ideal-control-messages.h"
#include "lte-enb-net-device.h"
#include "lte-ue-net-device.h"

#include <ns3/lte-enb-mac.h>
#include <ns3/lte-mac-tag.h>
#include <ns3/lte-ue-phy.h>


NS_LOG_COMPONENT_DEFINE ("LteEnbMac");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (LteEnbMac);

bool
operator< (const flowId_t& lhs, const flowId_t& rhs)
{
  if (lhs.m_rnti == rhs.m_rnti)
    {
      return (lhs.m_lcId < rhs.m_lcId);
    }
  else
    {
      return (lhs.m_rnti < rhs.m_rnti);
    }
}



// //////////////////////////////////////
// member SAP forwarders
// //////////////////////////////////////


class EnbMacMemberLteEnbCmacSapProvider : public LteEnbCmacSapProvider
{
public:
  EnbMacMemberLteEnbCmacSapProvider (LteEnbMac* mac);

  // inherited from LteEnbCmacSapProvider
  virtual void ConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth);
  virtual void AddUe (uint16_t rnti);
  virtual void AddLc (LcInfo lcinfo, LteMacSapUser* msu);
  virtual void ReconfigureLc (LcInfo lcinfo);
  virtual void ReleaseLc (uint16_t rnti, uint8_t lcid);

private:
  LteEnbMac* m_mac;
};


EnbMacMemberLteEnbCmacSapProvider::EnbMacMemberLteEnbCmacSapProvider (LteEnbMac* mac)
  : m_mac (mac)
{
}

void
EnbMacMemberLteEnbCmacSapProvider::ConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  m_mac->DoConfigureMac (ulBandwidth, dlBandwidth);
}

void
EnbMacMemberLteEnbCmacSapProvider::AddUe (uint16_t rnti)
{
  m_mac->DoAddUe (rnti);
}

void
EnbMacMemberLteEnbCmacSapProvider::AddLc (LcInfo lcinfo, LteMacSapUser* msu)
{
  m_mac->DoAddLc (lcinfo, msu);
}

void
EnbMacMemberLteEnbCmacSapProvider::ReconfigureLc (LcInfo lcinfo)
{
  m_mac->DoReconfigureLc (lcinfo);
}

void
EnbMacMemberLteEnbCmacSapProvider::ReleaseLc (uint16_t rnti, uint8_t lcid)
{
  m_mac->DoReleaseLc (rnti, lcid);
}



class EnbMacMemberLteMacSapProvider : public LteMacSapProvider
{
public:
  EnbMacMemberLteMacSapProvider (LteEnbMac* mac);

  // inherited from LteMacSapProvider
  virtual void TransmitPdu (TransmitPduParameters params);
  virtual void ReportBufferStatus (ReportBufferStatusParameters params);

private:
  LteEnbMac* m_mac;
};


EnbMacMemberLteMacSapProvider::EnbMacMemberLteMacSapProvider (LteEnbMac* mac)
  : m_mac (mac)
{
}

void
EnbMacMemberLteMacSapProvider::TransmitPdu (TransmitPduParameters params)
{
  m_mac->DoTransmitPdu (params);
}


void
EnbMacMemberLteMacSapProvider::ReportBufferStatus (ReportBufferStatusParameters params)
{
  m_mac->DoReportBufferStatus (params);
}




class EnbMacMemberFfMacSchedSapUser : public FfMacSchedSapUser
{
public:
  EnbMacMemberFfMacSchedSapUser (LteEnbMac* mac);


  virtual void SchedDlConfigInd (const struct SchedDlConfigIndParameters& params);
  virtual void SchedUlConfigInd (const struct SchedUlConfigIndParameters& params);
private:
  LteEnbMac* m_mac;
};


EnbMacMemberFfMacSchedSapUser::EnbMacMemberFfMacSchedSapUser (LteEnbMac* mac)
  : m_mac (mac)
{
}


void
EnbMacMemberFfMacSchedSapUser::SchedDlConfigInd (const struct SchedDlConfigIndParameters& params)
{
  m_mac->DoSchedDlConfigInd (params);
}



void
EnbMacMemberFfMacSchedSapUser::SchedUlConfigInd (const struct SchedUlConfigIndParameters& params)
{
  m_mac->DoSchedUlConfigInd (params);
}



class EnbMacMemberFfMacCschedSapUser : public FfMacCschedSapUser
{
public:
  EnbMacMemberFfMacCschedSapUser (LteEnbMac* mac);

  virtual void CschedCellConfigCnf (const struct CschedCellConfigCnfParameters& params);
  virtual void CschedUeConfigCnf (const struct CschedUeConfigCnfParameters& params);
  virtual void CschedLcConfigCnf (const struct CschedLcConfigCnfParameters& params);
  virtual void CschedLcReleaseCnf (const struct CschedLcReleaseCnfParameters& params);
  virtual void CschedUeReleaseCnf (const struct CschedUeReleaseCnfParameters& params);
  virtual void CschedUeConfigUpdateInd (const struct CschedUeConfigUpdateIndParameters& params);
  virtual void CschedCellConfigUpdateInd (const struct CschedCellConfigUpdateIndParameters& params);

private:
  LteEnbMac* m_mac;
};


EnbMacMemberFfMacCschedSapUser::EnbMacMemberFfMacCschedSapUser (LteEnbMac* mac)
  : m_mac (mac)
{
}

void
EnbMacMemberFfMacCschedSapUser::CschedCellConfigCnf (const struct CschedCellConfigCnfParameters& params)
{
  m_mac->DoCschedCellConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeConfigCnf (const struct CschedUeConfigCnfParameters& params)
{
  m_mac->DoCschedUeConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedLcConfigCnf (const struct CschedLcConfigCnfParameters& params)
{
  m_mac->DoCschedLcConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedLcReleaseCnf (const struct CschedLcReleaseCnfParameters& params)
{
  m_mac->DoCschedLcReleaseCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeReleaseCnf (const struct CschedUeReleaseCnfParameters& params)
{
  m_mac->DoCschedUeReleaseCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeConfigUpdateInd (const struct CschedUeConfigUpdateIndParameters& params)
{
  m_mac->DoCschedUeConfigUpdateInd (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedCellConfigUpdateInd (const struct CschedCellConfigUpdateIndParameters& params)
{
  m_mac->DoCschedCellConfigUpdateInd (params);
}



// ---------- PHY-SAP


class EnbMacMemberLteEnbPhySapUser : public LteEnbPhySapUser
{
public:
  EnbMacMemberLteEnbPhySapUser (LteEnbMac* mac);

  // inherited from LteEnbPhySapUser
  virtual void ReceivePhyPdu (Ptr<Packet> p);
  virtual void SubframeIndication (uint32_t frameNo, uint32_t subframeNo);
  virtual void ReceiveIdealControlMessage (Ptr<IdealControlMessage> msg);

private:
  LteEnbMac* m_mac;
};

EnbMacMemberLteEnbPhySapUser::EnbMacMemberLteEnbPhySapUser (LteEnbMac* mac) : m_mac (mac)
{
}


void
EnbMacMemberLteEnbPhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
  m_mac->DoReceivePhyPdu (p);
}

void
EnbMacMemberLteEnbPhySapUser::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  m_mac->DoSubframeIndication (frameNo, subframeNo);
}

void
EnbMacMemberLteEnbPhySapUser::ReceiveIdealControlMessage (Ptr<IdealControlMessage> msg)
{
  m_mac->DoReceiveIdealControlMessage (msg);
}


// //////////////////////////////////////
// generic LteEnbMac methods
// //////////////////////////////////////


TypeId
LteEnbMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteEnbMac")
    .SetParent<Object> ()
    .AddConstructor<LteEnbMac> ();
  return tid;
}


LteEnbMac::LteEnbMac ()
{
  m_macSapProvider = new EnbMacMemberLteMacSapProvider (this);
  m_cmacSapProvider = new EnbMacMemberLteEnbCmacSapProvider (this);

  m_schedSapUser = new EnbMacMemberFfMacSchedSapUser (this);
  m_cschedSapUser = new EnbMacMemberFfMacCschedSapUser (this);

  m_enbPhySapUser = new EnbMacMemberLteEnbPhySapUser (this);
}


LteEnbMac::~LteEnbMac ()
{
}

void
LteEnbMac::DoDispose ()
{
  delete m_macSapProvider;
  delete m_cmacSapProvider;
  delete m_schedSapUser;
  delete m_cschedSapUser;
}


void
LteEnbMac::SetFfMacSchedSapProvider (FfMacSchedSapProvider* s)
{
  m_schedSapProvider = s;
}

FfMacSchedSapUser*
LteEnbMac::GetFfMacSchedSapUser (void)
{
  return m_schedSapUser;
}

void
LteEnbMac::SetFfMacCschedSapProvider (FfMacCschedSapProvider* s)
{
  m_cschedSapProvider = s;
}

FfMacCschedSapUser*
LteEnbMac::GetFfMacCschedSapUser (void)
{
  return m_cschedSapUser;
}



void
LteEnbMac::SetLteMacSapUser (LteMacSapUser* s)
{
  m_macSapUser = s;
}

LteMacSapProvider*
LteEnbMac::GetLteMacSapProvider (void)
{
  return m_macSapProvider;
}

void
LteEnbMac::SetLteEnbCmacSapUser (LteEnbCmacSapUser* s)
{
  m_cmacSapUser = s;
}

LteEnbCmacSapProvider*
LteEnbMac::GetLteEnbCmacSapProvider (void)
{
  return m_cmacSapProvider;
}

void
LteEnbMac::SetLteEnbPhySapProvider (LteEnbPhySapProvider* s)
{
  m_enbPhySapProvider = s;
}


LteEnbPhySapUser*
LteEnbMac::GetLteEnbPhySapUser ()
{
  return m_enbPhySapUser;
}



void
LteEnbMac::DoSubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this);


  // --- DOWNLINK ---
  // Send CQI info to the scheduler
  FfMacSchedSapProvider::SchedDlCqiInfoReqParameters cqiInfoReq;
  cqiInfoReq.m_sfnSf = ((0xFF & frameNo) << 4) | (0xF & subframeNo);

  int cqiNum = m_dlCqiReceived.size ();
  if (cqiNum > MAX_CQI_LIST)
    {
      cqiNum = MAX_CQI_LIST;
    }

  cqiInfoReq.m_cqiList.insert (cqiInfoReq.m_cqiList.begin (), m_dlCqiReceived.begin (), m_dlCqiReceived.end ());
  m_dlCqiReceived.erase (m_dlCqiReceived.begin (), m_dlCqiReceived.end ());
  m_schedSapProvider->SchedDlCqiInfoReq (cqiInfoReq);


  // Get downlink transmission opportunities
  FfMacSchedSapProvider::SchedDlTriggerReqParameters params;  // to be filled
  m_schedSapProvider->SchedDlTriggerReq (params);


  // --- UPLINK ---

  // Send BSR reports to the scheduler
  FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters ulMacReq;
  ulMacReq.m_sfnSf = ((0xFF & frameNo) << 4) | (0xF & subframeNo);
  ulMacReq.m_macCeList.insert (ulMacReq.m_macCeList.begin (), m_ulCeReceived.begin (), m_ulCeReceived.end ());
  m_ulCeReceived.erase (m_ulCeReceived.begin (), m_ulCeReceived.end ());
  m_schedSapProvider->SchedUlMacCtrlInfoReq (ulMacReq);


  // Get uplink transmission opportunities
  FfMacSchedSapProvider::SchedUlTriggerReqParameters ulparams;
  ulparams.m_sfnSf = ((0xFF & frameNo) << 4) | (0xF & subframeNo);
  
  std::map <uint16_t,UlInfoListElement_s>::iterator it;
  for (it = m_ulInfoListElements.begin (); it != m_ulInfoListElements.end (); it++)
    {
      ulparams.m_ulInfoList.push_back ((*it).second);
    }
  m_schedSapProvider->SchedUlTriggerReq (ulparams);
  
  


  // reset UL info
  //std::map <uint16_t,UlInfoListElement_s>::iterator it;
  for (it = m_ulInfoListElements.begin (); it != m_ulInfoListElements.end (); it++)
    {
      for (uint16_t i = 0; i < (*it).second.m_ulReception.size (); i++)
        {
          (*it).second.m_ulReception.at (i) = 0;
        }
      (*it).second.m_receptionStatus = UlInfoListElement_s::Ok;
      (*it).second.m_tpc = 0;
    }
}

void
LteEnbMac::DoReceiveIdealControlMessage  (Ptr<IdealControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  if (msg->GetMessageType () == IdealControlMessage::DL_CQI)
    {
      Ptr<DlCqiIdealControlMessage> dlcqi = DynamicCast<DlCqiIdealControlMessage> (msg);
      ReceiveDlCqiIdealControlMessage (dlcqi);
    }
  else if (msg->GetMessageType () == IdealControlMessage::BSR)
    {
      Ptr<BsrIdealControlMessage> bsr = DynamicCast<BsrIdealControlMessage> (msg);
      ReceiveBsrMessage (bsr->GetBsr ());
    }
  else
    {
      NS_LOG_FUNCTION (this << " IdealControlMessage not recognized");
    }
}

void
LteEnbMac::ReceiveDlCqiIdealControlMessage  (Ptr<DlCqiIdealControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  // NS_LOG_FUNCTION (this << msg->GetSourceDevice () << msg->GetDestinationDevice ());

  CqiListElement_s dlcqi = msg->GetDlCqi ();
  NS_LOG_FUNCTION(this << "Enb Received DCI rnti" << dlcqi.m_rnti);
  m_dlCqiReceived.push_back (dlcqi);

}


void
LteEnbMac::ReceiveBsrMessage  (MacCeListElement_s bsr)
{
  NS_LOG_FUNCTION (this);

  m_ulCeReceived.push_back (bsr);
}



void
LteEnbMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  LteMacTag tag;
  p->RemovePacketTag (tag);
  
  // store info of the packet received
  
  std::map <uint16_t,UlInfoListElement_s>::iterator it;
//   u_int rnti = tag.GetRnti ();
//  u_int lcid = tag.GetLcid ();
  it = m_ulInfoListElements.find (tag.GetRnti ());
  if (it == m_ulInfoListElements.end ())
    {
      // new RNTI
      UlInfoListElement_s ulinfonew;
      ulinfonew.m_rnti = tag.GetRnti ();
      std::vector <uint16_t>::iterator it = ulinfonew.m_ulReception.begin ();
      ulinfonew.m_ulReception.insert (it + (tag.GetLcid () - 1), p->GetSize ());
      ulinfonew.m_receptionStatus = UlInfoListElement_s::Ok;
      ulinfonew.m_tpc = 0; // Tx power control not implemented at this stage
      m_ulInfoListElements.insert (std::pair<uint16_t, UlInfoListElement_s > (tag.GetRnti (), ulinfonew));
      
    }
  else
    {
      (*it).second.m_ulReception.at (tag.GetLcid () - 1) += p->GetSize ();
      (*it).second.m_receptionStatus = UlInfoListElement_s::Ok;
    }
      
  

  // forward the packet to the correspondent RLC
  flowId_t flow;
  flow.m_rnti = tag.GetRnti ();
  flow.m_lcId = tag.GetLcid ();
  std::map <flowId_t, LteMacSapUser* >::iterator it2;
  it2 = m_rlcAttached.find (flow);
  NS_ASSERT_MSG (it2 != m_rlcAttached.end (), "UE not attached rnti=" << flow.m_rnti << " lcid=" << (uint32_t) flow.m_lcId);
  (*it2).second->ReceivePdu (p);

}




// ////////////////////////////////////////////
// CMAC SAP
// ////////////////////////////////////////////

void
LteEnbMac::DoConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << " ulBandwidth=" << ulBandwidth << " dlBandwidth=" << dlBandwidth);
  FfMacCschedSapProvider::CschedCellConfigReqParameters params;
  // Configure the subset of parameters used by FfMacScheduler
  params.m_ulBandwidth = ulBandwidth;
  params.m_dlBandwidth = dlBandwidth;
  // ...more parameters can be configured
  m_cschedSapProvider->CschedCellConfigReq (params);
}


void
LteEnbMac::DoAddUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " rnti=" << rnti);
  FfMacCschedSapProvider::CschedUeConfigReqParameters params;
  params.m_rnti = rnti;
  m_cschedSapProvider->CschedUeConfigReq (params);
}


void
LteEnbMac::DoAddLc (LteEnbCmacSapProvider::LcInfo lcinfo, LteMacSapUser* msu)
{
  NS_LOG_FUNCTION (this);
  std::map <flowId_t, LteMacSapUser* >::iterator it;

  flowId_t flow;
  flow.m_rnti = lcinfo.rnti;
  flow.m_lcId = lcinfo.lcId;

  it = m_rlcAttached.find (flow);
  if (it == m_rlcAttached.end ())
    {
      m_rlcAttached.insert (std::pair<flowId_t, LteMacSapUser* > (flow, msu));
    }
  else
    {
      NS_LOG_ERROR ("LC already exists");
    }


  struct FfMacCschedSapProvider::CschedLcConfigReqParameters params;
  params.m_rnti = lcinfo.rnti;
  params.m_reconfigureFlag = false;

  struct LogicalChannelConfigListElement_s lccle;
  lccle.m_logicalChannelIdentity = lcinfo.lcId;
  lccle.m_logicalChannelGroup = lcinfo.lcGroup;
  lccle.m_direction = LogicalChannelConfigListElement_s::DIR_BOTH;
  lccle.m_qosBearerType = lcinfo.isGbr ? LogicalChannelConfigListElement_s::QBT_GBR : LogicalChannelConfigListElement_s::QBT_NON_GBR;
  lccle.m_qci = lcinfo.qci;
  lccle.m_eRabMaximulBitrateUl = lcinfo.mbrUl;
  lccle.m_eRabMaximulBitrateDl = lcinfo.mbrDl;
  lccle.m_eRabGuaranteedBitrateUl = lcinfo.gbrUl;
  lccle.m_eRabGuaranteedBitrateDl = lcinfo.gbrDl;
  params.m_logicalChannelConfigList.push_back (lccle);

  m_cschedSapProvider->CschedLcConfigReq (params);
}

void
LteEnbMac::DoReconfigureLc (LteEnbCmacSapProvider::LcInfo lcinfo)
{
  NS_FATAL_ERROR ("not implemented");
}

void
LteEnbMac::DoReleaseLc (uint16_t rnti, uint8_t lcid)
{
  NS_FATAL_ERROR ("not implemented");
}



// ////////////////////////////////////////////
// MAC SAP
// ////////////////////////////////////////////


void
LteEnbMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);
  LteMacTag tag (params.rnti, params.lcid);
  params.pdu->AddPacketTag (tag);
//   Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
//   pb->AddPacket (params.pdu);

  m_enbPhySapProvider->SendMacPdu (params.pdu);
}

void
LteEnbMac::DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params)
{
  NS_LOG_FUNCTION (this);
  FfMacSchedSapProvider::SchedDlRlcBufferReqParameters req;
  req.m_rnti = params.rnti;
  req.m_logicalChannelIdentity = params.lcid;
  req.m_rlcTransmissionQueueSize = params.txQueueSize;
  req.m_rlcTransmissionQueueHolDelay = params.txQueueHolDelay;
  req.m_rlcRetransmissionQueueSize = params.retxQueueSize;
  req.m_rlcRetransmissionHolDelay = params.retxQueueHolDelay;
  req.m_rlcStatusPduSize = params.statusPduSize;
  m_schedSapProvider->SchedDlRlcBufferReq (req);
}



// ////////////////////////////////////////////
// SCHED SAP
// ////////////////////////////////////////////



void
LteEnbMac::DoSchedDlConfigInd (FfMacSchedSapUser::SchedDlConfigIndParameters ind)
{
  NS_LOG_FUNCTION (this);
  // Create DL PHY PDU
  Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
  std::map <flowId_t, LteMacSapUser* >::iterator it;

  for (unsigned int i = 0; i < ind.m_buildDataList.size (); i++)
    {
      for (unsigned int j = 0; j < ind.m_buildDataList.at (i).m_rlcPduList.size (); j++)
        {
          flowId_t flow;
          flow.m_rnti = ind.m_buildDataList.at (i).m_rnti;
          flow.m_lcId = ind.m_buildDataList.at (i).m_rlcPduList.at (j).at (0).m_logicalChannelIdentity;
          it = m_rlcAttached.find (flow);
          NS_ASSERT_MSG (it != m_rlcAttached.end (), "rnti=" << flow.m_rnti << " lcid=" << (uint32_t) flow.m_lcId);
          (*it).second->NotifyTxOpportunity (ind.m_buildDataList.at (i).m_rlcPduList.at (j).at (0).m_size); // second [] is for TB
          // send the relative DCI
          Ptr<DlDciIdealControlMessage> msg = Create<DlDciIdealControlMessage> ();
          msg->SetDci (ind.m_buildDataList.at (i).m_dci);
          m_enbPhySapProvider->SendIdealControlMessage (msg);
        }
    }

}


void
LteEnbMac::DoSchedUlConfigInd (FfMacSchedSapUser::SchedUlConfigIndParameters ind)
{

  NS_LOG_FUNCTION (this);
  
  for (unsigned int i = 0; i < ind.m_dciList.size (); i++)
    {
      // send the correspondent ul dci
      Ptr<UlDciIdealControlMessage> msg = Create<UlDciIdealControlMessage> ();
      msg->SetDci (ind.m_dciList.at (i));
      m_enbPhySapProvider->SendIdealControlMessage (msg);
    }
  
}




// ////////////////////////////////////////////
// CSCHED SAP
// ////////////////////////////////////////////


void
LteEnbMac::DoCschedCellConfigCnf (FfMacCschedSapUser::CschedCellConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeConfigCnf (FfMacCschedSapUser::CschedUeConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedLcConfigCnf (FfMacCschedSapUser::CschedLcConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
  // Call the CSCHED primitive
  // m_cschedSap->LcConfigCompleted();
}

void
LteEnbMac::DoCschedLcReleaseCnf (FfMacCschedSapUser::CschedLcReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeReleaseCnf (FfMacCschedSapUser::CschedUeReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeConfigUpdateInd (FfMacCschedSapUser::CschedUeConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedCellConfigUpdateInd (FfMacCschedSapUser::CschedCellConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
}


} // namespace ns3
