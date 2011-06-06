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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/double.h"

#include "ns3/mobility-helper.h"
#include "ns3/lena-helper.h"

#include "ns3/lte-enb-phy.h"
#include "ns3/lte-enb-net-device.h"

#include "ns3/lte-ue-phy.h"
#include "ns3/lte-ue-net-device.h"

#include "ns3/lte-test-interference.h"

#include "lte-test-sinr-chunk-processor.h"

NS_LOG_COMPONENT_DEFINE ("LteInterferenceTest");

using namespace ns3;


void
LteTestDlSchedulingCallback (LteInterferenceTestCase *testcase, std::string path,
                             uint32_t frameNo, uint32_t subframeNo, uint16_t rnti,
                             uint8_t mcsTb1, uint16_t sizeTb1, uint8_t mcsTb2, uint16_t sizeTb2)
{
  testcase->DlScheduling(frameNo, subframeNo, rnti, mcsTb1, sizeTb1, mcsTb2, sizeTb2);
}

void
LteTestUlSchedulingCallback (LteInterferenceTestCase *testcase, std::string path,
                            uint32_t frameNo, uint32_t subframeNo, uint16_t rnti,
                            uint8_t mcs, uint16_t sizeTb)
{
    testcase->UlScheduling(frameNo, subframeNo, rnti, mcs, sizeTb);
}


/**
 * TestSuite
 */

LteInterferenceTestSuite::LteInterferenceTestSuite ()
  : TestSuite ("lte-interference", SYSTEM)
{
  NS_LOG_INFO ("Creating LteInterferenceTestSuite");

  AddTestCase (new LteInterferenceTestCase ("d1=3000, d2=6000",  3000.000000, 6000.000000,  3.844681, 1.714583,  0.761558, 0.389662, 6, 4));  
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=10",  50.000000, 10.000000,  0.040000, 0.040000,  0.010399, 0.010399, 0, 0));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=20",  50.000000, 20.000000,  0.160000, 0.159998,  0.041154, 0.041153, 0, 0));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=50",  50.000000, 50.000000,  0.999997, 0.999907,  0.239828, 0.239808, 2, 2));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=100",  50.000000, 100.000000,  3.999955, 3.998520,  0.785259, 0.785042, 6, 6));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=200",  50.000000, 200.000000,  15.999282, 15.976339,  1.961072, 1.959533, 14, 14));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=500",  50.000000, 500.000000,  99.971953, 99.082845,  4.254003, 4.241793, 22, 22));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=1000",  50.000000, 1000.000000,  399.551632, 385.718468,  6.194952, 6.144825, 28, 28));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=10000",  50.000000, 10000.000000,  35964.181431, 8505.970614,  12.667381, 10.588084, 28, 28));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=100000",  50.000000, 100000.000000,  327284.773828, 10774.181090,  15.853097, 10.928917, 28, 28));
  AddTestCase (new LteInterferenceTestCase ("d1=50, d2=1000000",  50.000000, 1000000.000000,  356132.574152, 10802.988445,  15.974963, 10.932767, 28, 28));
  
}

static LteInterferenceTestSuite lteLinkAdaptationWithInterferenceTestSuite;


/**
 * TestCase
 */

LteInterferenceTestCase::LteInterferenceTestCase (std::string name, double d1, double d2, double dlSinr, double ulSinr, double dlSe, double ulSe, uint16_t dlMcs, uint16_t ulMcs)
  : TestCase (name),
    m_d1 (d1),
    m_d2 (d2),
    m_dlSinrDb (10*log10(dlSinr)),
    m_ulSinrDb (10*log10(ulSinr)),
    m_dlSe (dlSe),
    m_ulSe (ulSe),
    m_dlMcs (dlMcs),
    m_ulMcs (ulMcs)
{
}

LteInterferenceTestCase::~LteInterferenceTestCase ()
{
}

void
LteInterferenceTestCase::DoRun (void)
{
  /**
    * Simulation Topology
    */

  Ptr<LenaHelper> lena = CreateObject<LenaHelper> ();
//   lena->EnableLogComponents ();
  lena->EnableMacTraces ();
  lena->EnableRlcTraces ();
  lena->SetAttribute ("PropagationModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes1;
  NodeContainer ueNodes2;
  enbNodes.Create (2);
  ueNodes1.Create (1);
  ueNodes2.Create (1);
  NodeContainer allNodes = NodeContainer ( enbNodes, ueNodes1, ueNodes2);

  // the topology is the following:
  //         d2  
  //  UE1-----------eNB2
  //   |             |
  // d1|             |d1
  //   |     d2      |
  //  eNB1----------UE2
  //
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));   // eNB1
  positionAlloc->Add (Vector (m_d2, m_d1, 0.0)); // eNB2
  positionAlloc->Add (Vector (0.0, m_d1, 0.0));  // UE1
  positionAlloc->Add (Vector (m_d2, 0.0, 0.0));  // UE2
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (allNodes);

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs1;
  NetDeviceContainer ueDevs2;
  lena->SetSchedulerType ("ns3::RrFfMacScheduler");
  enbDevs = lena->InstallEnbDevice (enbNodes);
  ueDevs1 = lena->InstallUeDevice (ueNodes1);
  ueDevs2 = lena->InstallUeDevice (ueNodes2);

  lena->Attach (ueDevs1, enbDevs.Get (0));
  lena->Attach (ueDevs2, enbDevs.Get (1));

  // Activate an EPS bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lena->ActivateEpsBearer (ueDevs1, bearer);
  lena->ActivateEpsBearer (ueDevs2, bearer);


  // Use testing chunk processor in the PHY layer
  // It will be used to test that the SNR is as intended
  // we plug in two instances, one for DL and one for UL

  Ptr<LtePhy> uePhy = ueDevs1.Get (0)->GetObject<LteUeNetDevice> ()->GetPhy ()->GetObject<LtePhy> ();
  Ptr<LteTestSinrChunkProcessor> testDlSinr = Create<LteTestSinrChunkProcessor> (uePhy);
  uePhy->GetDownlinkSpectrumPhy ()->AddSinrChunkProcessor (testDlSinr);

  Ptr<LtePhy> enbPhy = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetPhy ()->GetObject<LtePhy> ();
  Ptr<LteTestSinrChunkProcessor> testUlSinr = Create<LteTestSinrChunkProcessor> (enbPhy);
  enbPhy->GetUplinkSpectrumPhy ()->AddSinrChunkProcessor (testUlSinr);

  Config::Connect ("/NodeList/0/DeviceList/0/LteEnbMac/DlScheduling",
                    MakeBoundCallback(&LteTestDlSchedulingCallback, this));

  Config::Connect ("/NodeList/0/DeviceList/0/LteEnbMac/UlScheduling",
                    MakeBoundCallback(&LteTestUlSchedulingCallback, this));


  Simulator::Stop (Seconds (0.005));
  Simulator::Run ();

  double dlSinrDb = 10.0 * log10 (testDlSinr->GetSinr ()[0]);
  NS_TEST_ASSERT_MSG_EQ_TOL (dlSinrDb, m_dlSinrDb, 0.01, "Wrong SINR in DL!");

  double ulSinrDb = 10.0 * log10 (testUlSinr->GetSinr ()[0]);
  NS_TEST_ASSERT_MSG_EQ_TOL (ulSinrDb, m_ulSinrDb, 0.01, "Wrong SINR in UL!");

  Simulator::Destroy ();
}


void
LteInterferenceTestCase::DlScheduling (uint32_t frameNo, uint32_t subframeNo, uint16_t rnti,
                                         uint8_t mcsTb1, uint16_t sizeTb1, uint8_t mcsTb2, uint16_t sizeTb2)
{
  /**
   * Note:
   *    For first 4 subframeNo in the first frameNo, the MCS cannot be properly evaluated,
   *    because CQI feedback is still not available at the eNB.
   */
  if ( (frameNo > 1) || (subframeNo > 4) )
    {
      NS_TEST_ASSERT_MSG_EQ ((uint16_t)mcsTb1, m_dlMcs, "Wrong DL MCS ");
    }
}

void
LteInterferenceTestCase::UlScheduling (uint32_t frameNo, uint32_t subframeNo, uint16_t rnti,
                                         uint8_t mcs, uint16_t sizeTb)
{
  /**
   * Note:
   *    For first 4 subframeNo in the first frameNo, the MCS cannot be properly evaluated,
   *    because CQI feedback is still not available at the eNB.
   */
  if ( (frameNo > 1) || (subframeNo > 4) )
    {
      NS_TEST_ASSERT_MSG_EQ ((uint16_t)mcs, m_ulMcs, "Wrong UL MCS");
    }
}
