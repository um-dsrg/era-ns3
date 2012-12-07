/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Lluis Parcerisa <lparcerisa@cttc.cat>
 */

#include <ns3/fatal-error.h>
#include <ns3/log.h>
#include <ns3/nstime.h>
#include <ns3/node-list.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include "lte-rrc-protocol-real.h"
#include "lte-ue-rrc.h"
#include "lte-enb-rrc.h"
#include "lte-enb-net-device.h"
#include "lte-ue-net-device.h"

NS_LOG_COMPONENT_DEFINE ("LteRrcProtocolReal");


namespace ns3 {


const Time RRC_REAL_MSG_DELAY = MilliSeconds (0); 

NS_OBJECT_ENSURE_REGISTERED (LteUeRrcProtocolReal);

LteUeRrcProtocolReal::LteUeRrcProtocolReal ()
  :  m_ueRrcSapProvider (0),
     m_enbRrcSapProvider (0)
{
  m_ueRrcSapUser = new MemberLteUeRrcSapUser<LteUeRrcProtocolReal> (this);
}

LteUeRrcProtocolReal::~LteUeRrcProtocolReal ()
{
}

void
LteUeRrcProtocolReal::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_ueRrcSapUser;
  delete m_completeSetupParameters.srb0SapUser;
  delete m_completeSetupParameters.srb1SapUser;
  m_rrc = 0;
}

TypeId
LteUeRrcProtocolReal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteUeRrcProtocolReal")
    .SetParent<Object> ()
    .AddConstructor<LteUeRrcProtocolReal> ()
    ;
  return tid;
}

void 
LteUeRrcProtocolReal::SetLteUeRrcSapProvider (LteUeRrcSapProvider* p)
{
  m_ueRrcSapProvider = p;
}

LteUeRrcSapUser* 
LteUeRrcProtocolReal::GetLteUeRrcSapUser ()
{
  return m_ueRrcSapUser;
}

void 
LteUeRrcProtocolReal::SetUeRrc (Ptr<LteUeRrc> rrc)
{
  m_rrc = rrc;
}

void 
LteUeRrcProtocolReal::DoSetup (LteUeRrcSapUser::SetupParameters params)
{
  NS_LOG_FUNCTION (this);
  // Trick: we use this as a trigger to initialize the RNTI and cellID,
  // and to make sure we are talking to the appropriate eNB (e.g.,
  // after handover). We don't care about SRB0/SRB1 since we use real
  // RRC messages.
  DoReestablish ();

  m_setupParameters.srb0SapProvider = params.srb0SapProvider;
  m_setupParameters.srb1SapProvider = params.srb1SapProvider;

  LteRlcSapUser* srb0SapUser = new LteRlcSpecificLteRlcSapUser<LteUeRrcProtocolReal> (this);
  LtePdcpSapUser* srb1SapUser = new LtePdcpSpecificLtePdcpSapUser<LteUeRrcProtocolReal> (this);
  
  m_completeSetupParameters.srb0SapUser=srb0SapUser;
  m_completeSetupParameters.srb1SapUser=srb1SapUser;
  
  m_ueRrcSapProvider->CompleteSetup(m_completeSetupParameters);
}

void 
LteUeRrcProtocolReal::DoReestablish ()
{
  NS_LOG_FUNCTION (this);
  // // initialize the RNTI and get the EnbLteRrcSapProvider for the
  // // eNB we are currently attached to
  // m_rnti = m_rrc->GetRnti ();
  // SetEnbRrcSapProvider ();
  

  // if (m_havePendingRrcConnectionRequest == true)
  //   {      
  //     Simulator::Schedule (RRC_REAL_MSG_DELAY, 
  //                          &LteEnbRrcSapProvider::RecvRrcConnectionRequest,
  //                          m_enbRrcSapProvider,
  //                          m_rnti, 
  //                          m_pendingRrcConnectionRequest);
  //   }
}

void 
LteUeRrcProtocolReal::DoSendRrcConnectionRequest (LteRrcSap::RrcConnectionRequest msg)
{
  // initialize the RNTI and get the EnbLteRrcSapProvider for the
  // eNB we are currently attached to
  m_rnti = m_rrc->GetRnti ();
  SetEnbRrcSapProvider ();
  
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionRequestHeader rrcConnectionRequestHeader;
  rrcConnectionRequestHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionRequestHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = m_rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupParameters.srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
  
  /*  Simulator::Schedule (RRC_REAL_MSG_DELAY, 
                       &LteEnbRrcSapProvider::RecvRrcConnectionRequest,
                       m_enbRrcSapProvider,
                       m_rnti, 
                       msg);*/
}

void 
LteUeRrcProtocolReal::DoSendRrcConnectionSetupCompleted (LteRrcSap::RrcConnectionSetupCompleted msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;
  rrcConnectionSetupCompleteHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionSetupCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  if(m_setupParameters.srb1SapProvider)
  {
    m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
  }
}

void 
LteUeRrcProtocolReal::DoSendRrcConnectionReconfigurationCompleted (LteRrcSap::RrcConnectionReconfigurationCompleted msg)
{
  // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
  // eNB we are currently attached to
  m_rnti = m_rrc->GetRnti ();
  SetEnbRrcSapProvider ();
    
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
  rrcConnectionReconfigurationCompleteHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionReconfigurationCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void 
LteUeRrcProtocolReal::DoSendRrcConnectionReestablishmentRequest (LteRrcSap::RrcConnectionReestablishmentRequest msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
  rrcConnectionReestablishmentRequestHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionReestablishmentRequestHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = m_rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupParameters.srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void 
LteUeRrcProtocolReal::DoSendRrcConnectionReestablishmentComplete (LteRrcSap::RrcConnectionReestablishmentComplete msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
  rrcConnectionReestablishmentCompleteHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionReestablishmentCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}


void 
LteUeRrcProtocolReal::SetEnbRrcSapProvider ()
{
  uint16_t cellId = m_rrc->GetCellId ();  

  // walk list of all nodes to get the peer eNB
  Ptr<LteEnbNetDevice> enbDev;
  NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); 
       (i != listEnd) && (!found); 
       ++i)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      for (int j = 0; 
           (j < nDevs) && (!found);
           j++)
        {
          enbDev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbDev == 0)
            {
              continue;
            }
          else
            {
              if (enbDev->GetCellId () == cellId)
                {
                  found = true;          
                  break;
                }
            }
        }
    }
  NS_ASSERT_MSG (found, " Unable to find eNB with CellId =" << cellId);
  m_enbRrcSapProvider = enbDev->GetRrc ()->GetLteEnbRrcSapProvider ();  
  Ptr<LteEnbRrcProtocolReal> enbRrcProtocolReal = enbDev->GetRrc ()->GetObject<LteEnbRrcProtocolReal> ();
  enbRrcProtocolReal->SetUeRrcSapProvider (m_rnti, m_ueRrcSapProvider);
}

void
LteUeRrcProtocolReal::DoReceivePdcpPdu (Ptr<Packet> p)
{
  // Get type of message received
  RrcDlCcchMessage rrcDlCcchMessage;
  p->PeekHeader(rrcDlCcchMessage);
  
  // Declare possible headers to receive
  RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
  RrcConnectionSetupHeader rrcConnectionSetupHeader;

  // Declare possible messages
  LteRrcSap::RrcConnectionReestablishment rrcConnectionReestablishmentMsg;
  LteRrcSap::RrcConnectionSetup rrcConnectionSetupMsg;        

  // Deserialize packet and call member recv function with appropiate structure
  switch( rrcDlCcchMessage.GetMessageType() )
  {
    case 0:
      // RrcConnectionReestablishment
      p->RemoveHeader (rrcConnectionReestablishmentHeader);
      rrcConnectionReestablishmentMsg = rrcConnectionReestablishmentHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionReestablishment (rrcConnectionReestablishmentMsg);
      break;
    case 1:
      // RrcConnectionReestablishmentReject
      // ...
      break;
    case 2:
      // RrcConnectionReject
      // ...
      break;
    case 3:
      // RrcConnectionSetup
      p->RemoveHeader (rrcConnectionSetupHeader);
      rrcConnectionSetupMsg = rrcConnectionSetupHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionSetup (rrcConnectionSetupMsg);
      break;
  }
}

void
LteUeRrcProtocolReal::DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params)
{
  // Get type of message received
  RrcDlDcchMessage rrcDlDcchMessage;
  params.pdcpSdu->PeekHeader(rrcDlDcchMessage);
  
  // Declare possible headers to receive
  RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
  

  // Deserialize packet and call member recv function with appropiate structure
  switch( rrcDlDcchMessage.GetMessageType() )
  {
    case 4:
      params.pdcpSdu->RemoveHeader (rrcConnectionReconfigurationHeader);
      LteRrcSap::RrcConnectionReconfiguration rrcConnectionReconfigurationMsg;
      rrcConnectionReconfigurationMsg = rrcConnectionReconfigurationHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionReconfiguration (rrcConnectionReconfigurationMsg);
      break;
  }
}

NS_OBJECT_ENSURE_REGISTERED (LteEnbRrcProtocolReal);

LteEnbRrcProtocolReal::LteEnbRrcProtocolReal ()
  :  m_enbRrcSapProvider (0)
{
  NS_LOG_FUNCTION (this);
  m_enbRrcSapUser = new MemberLteEnbRrcSapUser<LteEnbRrcProtocolReal> (this);
}

LteEnbRrcProtocolReal::~LteEnbRrcProtocolReal ()
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbRrcProtocolReal::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_enbRrcSapUser;  
}

TypeId
LteEnbRrcProtocolReal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteEnbRrcProtocolReal")
    .SetParent<Object> ()
    .AddConstructor<LteEnbRrcProtocolReal> ()
    ;
  return tid;
}

void 
LteEnbRrcProtocolReal::SetLteEnbRrcSapProvider (LteEnbRrcSapProvider* p)
{
  m_enbRrcSapProvider = p;
}

LteEnbRrcSapUser* 
LteEnbRrcProtocolReal::GetLteEnbRrcSapUser ()
{
  return m_enbRrcSapUser;
}

void 
LteEnbRrcProtocolReal::SetCellId (uint16_t cellId)
{
  m_cellId = cellId;
}

LteUeRrcSapProvider* 
LteEnbRrcProtocolReal::GetUeRrcSapProvider (uint16_t rnti)
{
  std::map<uint16_t, LteUeRrcSapProvider*>::const_iterator it;
  it = m_enbRrcSapProviderMap.find (rnti);
  NS_ASSERT_MSG (it != m_enbRrcSapProviderMap.end (), "could not find RNTI = " << rnti);
  return it->second;
}

void 
LteEnbRrcProtocolReal::SetUeRrcSapProvider (uint16_t rnti, LteUeRrcSapProvider* p)
{
  std::map<uint16_t, LteUeRrcSapProvider*>::iterator it;
  it = m_enbRrcSapProviderMap.find (rnti);
  NS_ASSERT_MSG (it != m_enbRrcSapProviderMap.end (), "could not find RNTI = " << rnti);
  it->second = p;
}

void 
LteEnbRrcProtocolReal::DoSetupUe (uint16_t rnti, LteEnbRrcSapUser::SetupUeParameters params)
{
  NS_LOG_FUNCTION (this << rnti);

  // // walk list of all nodes to get the peer UE RRC SAP Provider
  // Ptr<LteUeRrc> ueRrc;
  // NodeList::Iterator listEnd = NodeList::End ();
  // bool found = false;
  // for (NodeList::Iterator i = NodeList::Begin (); (i != listEnd) && (found == false); i++)
  //   {
  //     Ptr<Node> node = *i;
  //     int nDevs = node->GetNDevices ();
  //     for (int j = 0; j < nDevs; j++)
  //       {
  //         Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
  //         if (!ueDev)
  //           {
  //             continue;
  //           }
  //         else
  //           {
  //             ueRrc = ueDev->GetRrc ();
  //             if ((ueRrc->GetRnti () == rnti) && (ueRrc->GetCellId () == m_cellId))
  //               {                 
  //       	  found = true;
  //       	  break;
  //               }
  //           }
  //       }
  //   }
  // NS_ASSERT_MSG (found , " Unable to find UE with RNTI=" << rnti << " cellId=" << m_cellId);
  // m_enbRrcSapProviderMap[rnti] = ueRrc->GetLteUeRrcSapProvider ();

  // just create empty entry, the UeRrcSapProvider will be set by the
  // ue upon connection request or connection reconfiguration
  // completed 
  m_enbRrcSapProviderMap[rnti] = 0;

  // Store SetupUeParameters
  m_setupUeParametersMap[rnti] = params;
  
  // Create LteRlcSapUser, LtePdcpSapUser
  LteRlcSapUser* srb0SapUser = new LteRlcSpecificLteRlcSapUser<LteEnbRrcProtocolReal> (this);
  LtePdcpSapUser* srb1SapUser = new LtePdcpSpecificLtePdcpSapUser<LteEnbRrcProtocolReal> (this);
  LteEnbRrcSapProvider::CompleteSetupUeParameters completeSetupUeParameters;
  completeSetupUeParameters.srb0SapUser = srb0SapUser;
  completeSetupUeParameters.srb1SapUser = srb1SapUser;
  
  // Store LteRlcSapUser, LtePdcpSapUser
  m_completeSetupUeParametersMap[rnti] = completeSetupUeParameters;
  
  m_enbRrcSapProvider->CompleteSetupUe(rnti,completeSetupUeParameters);
}

void 
LteEnbRrcProtocolReal::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  m_enbRrcSapProviderMap.erase (rnti);
  m_setupUeParametersMap.erase (rnti);
}

void 
LteEnbRrcProtocolReal::DoSendMasterInformationBlock (LteRrcSap::MasterInformationBlock msg)
{
  NS_LOG_FUNCTION (this);
  for (std::map<uint16_t, LteUeRrcSapProvider*>::const_iterator it = m_enbRrcSapProviderMap.begin ();
       it != m_enbRrcSapProviderMap.end ();
       ++it)
    {
      Simulator::Schedule (RRC_REAL_MSG_DELAY, 
                           &LteUeRrcSapProvider::RecvMasterInformationBlock,
                           it->second, 
                           msg);
    }
}

void 
LteEnbRrcProtocolReal::DoSendSystemInformationBlockType1 (LteRrcSap::SystemInformationBlockType1 msg)
{
  NS_LOG_FUNCTION (this << m_cellId);
 // walk list of all nodes to get UEs with this cellId
  Ptr<LteUeRrc> ueRrc;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; ++j)
        {
          Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (ueDev != 0)
            {
              NS_LOG_LOGIC ("considering UE " << ueDev->GetImsi ());
              Ptr<LteUeRrc> ueRrc = ueDev->GetRrc ();              
              if (ueRrc->GetCellId () == m_cellId)
                {       
                  Simulator::Schedule (RRC_REAL_MSG_DELAY, 
                                       &LteUeRrcSapProvider::RecvSystemInformationBlockType1,
                                       ueRrc->GetLteUeRrcSapProvider (), 
                                       msg);          
                }             
            }
        }
    } 
}

void 
LteEnbRrcProtocolReal::DoSendSystemInformation (LteRrcSap::SystemInformation msg)
{
  NS_LOG_FUNCTION (this << m_cellId);
  // walk list of all nodes to get UEs with this cellId
  Ptr<LteUeRrc> ueRrc;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; ++j)
        {
          Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (ueDev != 0)
            {
              Ptr<LteUeRrc> ueRrc = ueDev->GetRrc ();              
              NS_LOG_LOGIC ("considering UE IMSI " << ueDev->GetImsi () << " that has cellId " << ueRrc->GetCellId ());
              if (ueRrc->GetCellId () == m_cellId)
                {       
                  NS_LOG_LOGIC ("sending SI to IMSI " << ueDev->GetImsi ());
                  ueRrc->GetLteUeRrcSapProvider ()->RecvSystemInformation (msg);
                  Simulator::Schedule (RRC_REAL_MSG_DELAY, 
                                       &LteUeRrcSapProvider::RecvSystemInformation,
                                       ueRrc->GetLteUeRrcSapProvider (), 
                                       msg);          
                }             
            }
        }
    } 
}

void 
LteEnbRrcProtocolReal::DoSendRrcConnectionSetup (uint16_t rnti, LteRrcSap::RrcConnectionSetup msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionSetupHeader rrcConnectionSetupHeader;
  rrcConnectionSetupHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionSetupHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  std::map<uint16_t, LteEnbRrcSapUser::SetupUeParameters>::iterator it;
  if(m_setupUeParametersMap.find(rnti) == m_setupUeParametersMap.end() )
  {
    std::cout << "RNTI not found in Enb setup parameters Map!" << std::endl;
  }
  else
  {
    m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
  }
}

void 
LteEnbRrcProtocolReal::DoSendRrcConnectionReconfiguration (uint16_t rnti, LteRrcSap::RrcConnectionReconfiguration msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
  rrcConnectionReconfigurationHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionReconfigurationHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupUeParametersMap[rnti].srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void 
LteEnbRrcProtocolReal::DoSendRrcConnectionReestablishment (uint16_t rnti, LteRrcSap::RrcConnectionReestablishment msg)
{
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
  rrcConnectionReestablishmentHeader.SetMessage (msg);
  
  packet->AddHeader (rrcConnectionReestablishmentHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void 
LteEnbRrcProtocolReal::DoSendRrcConnectionReestablishmentReject (uint16_t rnti, LteRrcSap::RrcConnectionReestablishmentReject msg)
{
  Simulator::Schedule (RRC_REAL_MSG_DELAY, 
		       &LteUeRrcSapProvider::RecvRrcConnectionReestablishmentReject,
		       GetUeRrcSapProvider (rnti), 
		       msg);
}

void 
LteEnbRrcProtocolReal::DoSendRrcConnectionRelease (uint16_t rnti, LteRrcSap::RrcConnectionRelease msg)
{
  Simulator::Schedule (RRC_REAL_MSG_DELAY, 
		       &LteUeRrcSapProvider::RecvRrcConnectionRelease,
		       GetUeRrcSapProvider (rnti), 
		       msg);
}

void
LteEnbRrcProtocolReal::DoReceivePdcpPdu (Ptr<Packet> p)
{
  // Get type of message received
  RrcUlCcchMessage rrcUlCcchMessage;
  p->PeekHeader(rrcUlCcchMessage);
  
  // Declare possible headers to receive
  RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
  RrcConnectionRequestHeader rrcConnectionRequestHeader;

  // Deserialize packet and call member recv function with appropiate structure
  switch( rrcUlCcchMessage.GetMessageType() )
  {
    case 0:
      p->RemoveHeader (rrcConnectionReestablishmentRequestHeader);
      LteRrcSap::RrcConnectionReestablishmentRequest rrcConnectionReestablishmentRequestMsg;
      rrcConnectionReestablishmentRequestMsg = rrcConnectionReestablishmentRequestHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReestablishmentRequest (m_rnti,rrcConnectionReestablishmentRequestMsg);
      break;
    case 1:
      p->RemoveHeader (rrcConnectionRequestHeader);
      LteRrcSap::RrcConnectionRequest rrcConnectionRequestMsg;
      rrcConnectionRequestMsg = rrcConnectionRequestHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionRequest (m_rnti,rrcConnectionRequestMsg);
      break;
  }
}

void
LteEnbRrcProtocolReal::DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params)
{
  // Get type of message received
  RrcUlDcchMessage rrcUlDcchMessage;
  params.pdcpSdu->PeekHeader(rrcUlDcchMessage);
  
  // Declare possible headers to receive
  RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
  RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
  RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;

  // Deserialize packet and call member recv function with appropiate structure
  switch( rrcUlDcchMessage.GetMessageType() )
  {
    case 2:
      params.pdcpSdu->RemoveHeader (rrcConnectionReconfigurationCompleteHeader);
      LteRrcSap::RrcConnectionReconfigurationCompleted rrcConnectionReconfigurationCompleteMsg;
      rrcConnectionReconfigurationCompleteMsg = rrcConnectionReconfigurationCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReconfigurationCompleted (params.rnti,rrcConnectionReconfigurationCompleteMsg);
      break;
    case 3:
      params.pdcpSdu->RemoveHeader (rrcConnectionReestablishmentCompleteHeader);
      LteRrcSap::RrcConnectionReestablishmentComplete rrcConnectionReestablishmentCompleteMsg;
      rrcConnectionReestablishmentCompleteMsg = rrcConnectionReestablishmentCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReestablishmentComplete (params.rnti,rrcConnectionReestablishmentCompleteMsg);
      break;
    case 4:
      params.pdcpSdu->RemoveHeader (rrcConnectionSetupCompleteHeader);
      LteRrcSap::RrcConnectionSetupCompleted rrcConnectionSetupCompletedMsg;
      rrcConnectionSetupCompletedMsg = rrcConnectionSetupCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionSetupCompleted (params.rnti, rrcConnectionSetupCompletedMsg);
      break;
  }

}

/*
 * The purpose of LteEnbRrcProtocolReal is to avoid encoding
 * messages. In order to do so, we need to have some form of encoding for
 * inter-node RRC messages like HandoverPreparationInfo and HandoverCommand. Doing so
 * directly is not practical (these messages includes a lot of
 * information elements, so encoding all of them would defeat the
 * purpose of LteEnbRrcProtocolReal. The workaround is to store the
 * actual message in a global map, so that then we can just encode the
 * key in a header and send that between eNBs over X2.
 * 
 */

std::map<uint32_t, LteRrcSap::HandoverPreparationInfo> g_handoverPreparationInfoMsgMap2;
uint32_t g_handoverPreparationInfoMsgIdCounter2 = 0;

/*
 * This header encodes the map key discussed above. We keep this
 * private since it should not be used outside this file.
 * 
 */
class RealHandoverPreparationInfoHeader : public Header
{
public:
  uint32_t GetMsgId ();
  void SetMsgId (uint32_t id);
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint32_t m_msgId;
};

uint32_t 
RealHandoverPreparationInfoHeader::GetMsgId ()
{
  return m_msgId;
}  

void 
RealHandoverPreparationInfoHeader::SetMsgId (uint32_t id)
{
  m_msgId = id;
}  


TypeId
RealHandoverPreparationInfoHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RealHandoverPreparationInfoHeader")
    .SetParent<Header> ()
    .AddConstructor<RealHandoverPreparationInfoHeader> ()
  ;
  return tid;
}

TypeId
RealHandoverPreparationInfoHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void RealHandoverPreparationInfoHeader::Print (std::ostream &os)  const
{
  os << " msgId=" << m_msgId;
}

uint32_t RealHandoverPreparationInfoHeader::GetSerializedSize (void) const
{
  return 4;
}

void RealHandoverPreparationInfoHeader::Serialize (Buffer::Iterator start) const
{  
  start.WriteU32 (m_msgId);
}

uint32_t RealHandoverPreparationInfoHeader::Deserialize (Buffer::Iterator start)
{
  m_msgId = start.ReadU32 ();
  return GetSerializedSize ();
}



Ptr<Packet> 
LteEnbRrcProtocolReal::DoEncodeHandoverPreparationInformation (LteRrcSap::HandoverPreparationInfo msg)
{
  uint32_t msgId = ++g_handoverPreparationInfoMsgIdCounter2;
  NS_ASSERT_MSG (g_handoverPreparationInfoMsgMap2.find (msgId) == g_handoverPreparationInfoMsgMap2.end (), "msgId " << msgId << " already in use");
  NS_LOG_INFO (" encoding msgId = " << msgId);
  g_handoverPreparationInfoMsgMap2.insert (std::pair<uint32_t, LteRrcSap::HandoverPreparationInfo> (msgId, msg));
  RealHandoverPreparationInfoHeader h;
  h.SetMsgId (msgId);
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (h);
  return p;
}

LteRrcSap::HandoverPreparationInfo 
LteEnbRrcProtocolReal::DoDecodeHandoverPreparationInformation (Ptr<Packet> p)
{
  RealHandoverPreparationInfoHeader h;
  p->RemoveHeader (h);
  uint32_t msgId = h.GetMsgId ();
  NS_LOG_INFO (" decoding msgId = " << msgId);
  std::map<uint32_t, LteRrcSap::HandoverPreparationInfo>::iterator it = g_handoverPreparationInfoMsgMap2.find (msgId);
  NS_ASSERT_MSG (it != g_handoverPreparationInfoMsgMap2.end (), "msgId " << msgId << " not found");
  LteRrcSap::HandoverPreparationInfo msg = it->second;
  g_handoverPreparationInfoMsgMap2.erase (it);
  return msg;
}



std::map<uint32_t, LteRrcSap::RrcConnectionReconfiguration> g_handoverCommandMsgMap2;
uint32_t g_handoverCommandMsgIdCounter2 = 0;

/*
 * This header encodes the map key discussed above. We keep this
 * private since it should not be used outside this file.
 * 
 */
class RealHandoverCommandHeader : public Header
{
public:
  uint32_t GetMsgId ();
  void SetMsgId (uint32_t id);
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint32_t m_msgId;
};

uint32_t 
RealHandoverCommandHeader::GetMsgId ()
{
  return m_msgId;
}  

void 
RealHandoverCommandHeader::SetMsgId (uint32_t id)
{
  m_msgId = id;
}  


TypeId
RealHandoverCommandHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RealHandoverCommandHeader")
    .SetParent<Header> ()
    .AddConstructor<RealHandoverCommandHeader> ()
  ;
  return tid;
}

TypeId
RealHandoverCommandHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void RealHandoverCommandHeader::Print (std::ostream &os)  const
{
  os << " msgId=" << m_msgId;
}

uint32_t RealHandoverCommandHeader::GetSerializedSize (void) const
{
  return 4;
}

void RealHandoverCommandHeader::Serialize (Buffer::Iterator start) const
{  
  start.WriteU32 (m_msgId);
}

uint32_t RealHandoverCommandHeader::Deserialize (Buffer::Iterator start)
{
  m_msgId = start.ReadU32 ();
  return GetSerializedSize ();
}



Ptr<Packet> 
LteEnbRrcProtocolReal::DoEncodeHandoverCommand (LteRrcSap::RrcConnectionReconfiguration msg)
{
  uint32_t msgId = ++g_handoverCommandMsgIdCounter2;
  NS_ASSERT_MSG (g_handoverCommandMsgMap2.find (msgId) == g_handoverCommandMsgMap2.end (), "msgId " << msgId << " already in use");
  NS_LOG_INFO (" encoding msgId = " << msgId);
  g_handoverCommandMsgMap2.insert (std::pair<uint32_t, LteRrcSap::RrcConnectionReconfiguration> (msgId, msg));
  RealHandoverCommandHeader h;
  h.SetMsgId (msgId);
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (h);
  return p;
}

LteRrcSap::RrcConnectionReconfiguration
LteEnbRrcProtocolReal::DoDecodeHandoverCommand (Ptr<Packet> p)
{
  RealHandoverCommandHeader h;
  p->RemoveHeader (h);
  uint32_t msgId = h.GetMsgId ();
  NS_LOG_INFO (" decoding msgId = " << msgId);
  std::map<uint32_t, LteRrcSap::RrcConnectionReconfiguration>::iterator it = g_handoverCommandMsgMap2.find (msgId);
  NS_ASSERT_MSG (it != g_handoverCommandMsgMap2.end (), "msgId " << msgId << " not found");
  LteRrcSap::RrcConnectionReconfiguration msg = it->second;
  g_handoverCommandMsgMap2.erase (it);
  return msg;
}





} // namespace ns3
