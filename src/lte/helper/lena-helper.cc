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
 * Author: Nicola Baldo <nbaldo@cttc.es> (re-wrote from scratch this helper)
 *         Giuseppe Piro <g.piro@poliba.it> (parts of the PHY & channel  creation & configuration copied from the GSoC 2011 code)
 */


#include "lena-helper.h"

#include <ns3/string.h>
#include <ns3/log.h>

#include <ns3/lte-enb-rrc.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-ue-mac.h>
#include <ns3/lte-enb-mac.h>

#include <ns3/lte-enb-phy.h>
#include <ns3/lte-ue-phy.h>
#include <ns3/lte-spectrum-phy.h>
#include <ns3/lte-sinr-chunk-processor.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/friis-spectrum-propagation-loss.h>

#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-net-device.h>

#include <ns3/ff-mac-scheduler.h>

NS_LOG_COMPONENT_DEFINE ("LenaHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LenaHelper);

LenaHelper::LenaHelper (void)
{
  NS_LOG_FUNCTION (this);  
}

void 
LenaHelper::DoStart (void)
{
  NS_LOG_FUNCTION (this);
  m_downlinkChannel = CreateObject<SingleModelSpectrumChannel> ();
  m_uplinkChannel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<SpectrumPropagationLossModel> dlPropagationModel = m_propagationModelFactory.Create<SpectrumPropagationLossModel> ();
  Ptr<SpectrumPropagationLossModel> ulPropagationModel = m_propagationModelFactory.Create<SpectrumPropagationLossModel> ();
  m_downlinkChannel->AddSpectrumPropagationLossModel (dlPropagationModel);
  m_uplinkChannel->AddSpectrumPropagationLossModel (ulPropagationModel);  
  Object::DoStart ();
}

LenaHelper::~LenaHelper (void)
{
  NS_LOG_FUNCTION (this);
}


void
LenaHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downlinkChannel = 0;
  m_uplinkChannel = 0;
  Object::DoDispose ();
}

TypeId LenaHelper::GetTypeId (void)
{
  static TypeId
  tid =
    TypeId ("ns3::LenaHelper")
    .SetParent<Object> ()
    .AddConstructor<LenaHelper> ()
    .AddAttribute ("Scheduler",
                   "The type of scheduler to be used for eNBs",               
                   StringValue ("ns3::RrFfMacScheduler"),
                   MakeStringAccessor (&LenaHelper::SetSchedulerType),                   
                   MakeStringChecker ())
    .AddAttribute ("PropagationModel",
                   "The type of propagation model to be used",               
                   StringValue ("ns3::FriisSpectrumPropagationLossModel"),
                   MakeStringAccessor (&LenaHelper::SetPropagationModelType),                   
                   MakeStringChecker ())
    ;
  return tid;
}

void 
LenaHelper::SetSchedulerType (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_schedulerFactory = ObjectFactory ();
  m_schedulerFactory.SetTypeId (type);
}

void 
LenaHelper::SetSchedulerAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this << n);
  m_schedulerFactory.Set (n, v);
}


void 
LenaHelper::SetPropagationModelType (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_propagationModelFactory = ObjectFactory ();
  m_propagationModelFactory.SetTypeId (type);
}

void 
LenaHelper::SetPropagationModelAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this << n);
  m_propagationModelFactory.Set (n, v);
}


NetDeviceContainer
LenaHelper::InstallEnbDevice (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  Start ();  // will run DoStart () if necessary
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<NetDevice> device = InstallSingleEnbDevice (node);
      devices.Add (device);
    }
  return devices;
}

NetDeviceContainer
LenaHelper::InstallUeDevice (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<NetDevice> device = InstallSingleUeDevice (node);
      devices.Add (device);
    }
  return devices;
}


void
LenaHelper::SetEnbDeviceAttribute (std::string name, const AttributeValue &value)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("not implemented yet");
}

void
LenaHelper::SetUeDeviceAttribute (std::string name, const AttributeValue &value)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("not implemented yet");
}


Ptr<NetDevice>
LenaHelper::InstallSingleEnbDevice (Ptr<Node> n)
{
  Ptr<LteSpectrumPhy> dlPhy = CreateObject<LteSpectrumPhy> ();
  Ptr<LteSpectrumPhy> ulPhy = CreateObject<LteSpectrumPhy> ();
  
  Ptr<LteEnbPhy> phy = CreateObject<LteEnbPhy> (dlPhy, ulPhy);

  Ptr<LteCqiSinrChunkProcessor> p = Create<LteCqiSinrChunkProcessor> (phy->GetObject<LtePhy> ());
  ulPhy->AddSinrChunkProcessor (p);

  dlPhy->SetChannel (m_downlinkChannel);
  ulPhy->SetChannel (m_uplinkChannel);

  Ptr<MobilityModel> mm = n->GetObject<MobilityModel> ();
  NS_ASSERT_MSG (mm, "MobilityModel needs to be set on node before calling LenaHelper::InstallUeDevice ()");
  dlPhy->SetMobility (mm);
  ulPhy->SetMobility (mm);

  m_uplinkChannel->AddRx (ulPhy);

  Ptr<LteEnbMac> mac = CreateObject<LteEnbMac> ();
  Ptr<FfMacScheduler> sched = m_schedulerFactory.Create<FfMacScheduler> ();  
  Ptr<LteEnbRrc> rrc = CreateObject<LteEnbRrc> ();


  // connect SAPs
  rrc->SetLteEnbCmacSapProvider (mac->GetLteEnbCmacSapProvider ());
  mac->SetLteEnbCmacSapUser (rrc->GetLteEnbCmacSapUser ());
  rrc->SetLteMacSapProvider (mac->GetLteMacSapProvider ());

  mac->SetFfMacSchedSapProvider (sched->GetFfMacSchedSapProvider ());
  mac->SetFfMacCschedSapProvider (sched->GetFfMacCschedSapProvider ());

  sched->SetFfMacSchedSapUser (mac->GetFfMacSchedSapUser ());
  sched->SetFfMacCschedSapUser (mac->GetFfMacCschedSapUser ());

  phy->SetLteEnbPhySapUser (mac->GetLteEnbPhySapUser ());
  mac->SetLteEnbPhySapProvider (phy->GetLteEnbPhySapProvider ());

  Ptr<LteEnbNetDevice> dev = CreateObject<LteEnbNetDevice> (n, phy, mac, sched, rrc);
  phy->SetDevice (dev);
  dlPhy->SetDevice (dev);
  ulPhy->SetDevice (dev);

  n->AddDevice (dev);
  ulPhy->SetPhyMacRxEndOkCallback (MakeCallback (&LteEnbPhy::PhyPduReceived, phy));

  dev->Start ();
  return dev;
}

Ptr<NetDevice>
LenaHelper::InstallSingleUeDevice (Ptr<Node> n)
{
  Ptr<LteSpectrumPhy> dlPhy = CreateObject<LteSpectrumPhy> ();
  Ptr<LteSpectrumPhy> ulPhy = CreateObject<LteSpectrumPhy> ();

  Ptr<LteUePhy> phy = CreateObject<LteUePhy> (dlPhy, ulPhy);

  Ptr<LteCqiSinrChunkProcessor> p = Create<LteCqiSinrChunkProcessor> (phy->GetObject<LtePhy> ());
  dlPhy->AddSinrChunkProcessor (p);

  dlPhy->SetChannel (m_downlinkChannel);
  ulPhy->SetChannel (m_uplinkChannel);

  Ptr<MobilityModel> mm = n->GetObject<MobilityModel> ();
  NS_ASSERT_MSG (mm, "MobilityModel needs to be set on node before calling LenaHelper::InstallUeDevice ()");
  dlPhy->SetMobility (mm);
  ulPhy->SetMobility (mm);

  m_downlinkChannel->AddRx (dlPhy);
  
  Ptr<LteUeMac> mac = CreateObject<LteUeMac> ();
  Ptr<LteUeRrc> rrc = CreateObject<LteUeRrc> ();

  // connect SAPs
  rrc->SetLteUeCmacSapProvider (mac->GetLteUeCmacSapProvider ());
  mac->SetLteUeCmacSapUser (rrc->GetLteUeCmacSapUser ());
  rrc->SetLteMacSapProvider (mac->GetLteMacSapProvider ());
  phy->SetLteUePhySapUser (mac->GetLteUePhySapUser ());
  mac->SetLteUePhySapProvider (phy->GetLteUePhySapProvider ());

  Ptr<LteUeNetDevice> dev = CreateObject<LteUeNetDevice> (n, phy, mac, rrc);
  phy->SetDevice (dev);
  dlPhy->SetDevice (dev);
  ulPhy->SetDevice (dev);

  n->AddDevice (dev);
  dlPhy->SetPhyMacRxEndOkCallback (MakeCallback (&LteUePhy::PhyPduReceived, phy));

  dev->Start ();
  return dev;
}


void
LenaHelper::Attach (NetDeviceContainer ueDevices, Ptr<NetDevice> enbDevice)
{
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      Attach (*i, enbDevice);
    }
}

void
LenaHelper::Attach (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice)
{
  // setup RRC connection
  Ptr<LteEnbRrc> enbRrc = enbDevice->GetObject<LteEnbNetDevice> ()->GetRrc ();
  uint16_t rnti = enbRrc->AddUe ();
  Ptr<LteUeRrc> ueRrc = ueDevice->GetObject<LteUeNetDevice> ()->GetRrc ();
  ueRrc->ConfigureUe (rnti);

  // attach UE to eNB
  ueDevice->GetObject<LteUeNetDevice> ()->SetTargetEnb (enbDevice->GetObject<LteEnbNetDevice> ());


  // connect at the PHY layer
  Ptr<LteEnbPhy> enbPhy = enbDevice->GetObject<LteEnbNetDevice> ()->GetPhy ();
  Ptr<LteUePhy> uePhy = ueDevice->GetObject<LteUeNetDevice> ()->GetPhy ();
  enbPhy->AddUePhy (rnti, uePhy);
 
  // WILD HACK - should be done through PHY SAP, probably passing by RRC
  uePhy->SetRnti (rnti);
  uePhy->DoSetBandwidth (enbDevice->GetObject<LteEnbNetDevice> ()->GetUlBandwidth (), 
                         enbDevice->GetObject<LteEnbNetDevice> ()->GetDlBandwidth ());
}


void
LenaHelper::ActivateEpsBearer (NetDeviceContainer ueDevices, EpsBearer bearer)
{
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      ActivateEpsBearer (*i, bearer);
    }
}


void
LenaHelper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, EpsBearer bearer)
{
  Ptr<LteEnbNetDevice> enbDevice = ueDevice->GetObject<LteUeNetDevice> ()->GetTargetEnb ();
  Ptr<LteEnbRrc> enbRrc = enbDevice->GetObject<LteEnbNetDevice> ()->GetRrc ();
  Ptr<LteUeRrc> ueRrc = ueDevice->GetObject<LteUeNetDevice> ()->GetRrc ();
  uint16_t rnti = ueRrc->GetRnti ();
  uint8_t lcid = enbRrc->SetupRadioBearer (rnti, bearer);
  ueRrc->SetupRadioBearer (rnti, bearer, lcid);
}


void
LenaHelper::EnableLogComponents (void)
{
  LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlc", LOG_LEVEL_ALL);
  LogComponentEnable ("RrFfMacScheduler", LOG_LEVEL_ALL);
  LogComponentEnable ("PfFfMacScheduler", LOG_LEVEL_ALL);

  LogComponentEnable ("LtePhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUePhy", LOG_LEVEL_ALL);

  LogComponentEnable ("LteSpectrumPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteInterference", LOG_LEVEL_ALL);
  LogComponentEnable ("LteSinrChunkProcessor", LOG_LEVEL_ALL);
 
  LogComponentEnable ("LtePropagationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("LossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("ShadowingLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("PenetrationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("MultipathLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("PathLossModel", LOG_LEVEL_ALL);

  LogComponentEnable ("LteNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbNetDevice", LOG_LEVEL_ALL);

  LogComponentEnable ("RlcStatsCalculator", LOG_LEVEL_ALL);
}



} // namespace ns3
