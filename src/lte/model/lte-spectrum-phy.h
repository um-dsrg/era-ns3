/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Giuseppe Piro  <g.piro@poliba.it>
 * Modified by: Marco Miozzo <mmiozzo@cttc.es> (introduce physical error model)
 */

#ifndef LTE_SPECTRUM_PHY_H
#define LTE_SPECTRUM_PHY_H

#include <ns3/event-id.h>
#include <ns3/spectrum-value.h>
#include <ns3/mobility-model.h>
#include <ns3/packet.h>
#include <ns3/nstime.h>
#include <ns3/net-device.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-interference.h>
#include <ns3/data-rate.h>
#include <ns3/generic-phy.h>
#include <ns3/packet-burst.h>
#include <ns3/lte-interference.h>
#include <ns3/random-variable.h>
#include <map>

namespace ns3 {


struct tbInfo_t
{
  uint16_t size;
  uint8_t mcs;
  std::vector<int> rbBitmap;
  bool corrupt;
};

typedef std::map<uint16_t, tbInfo_t> expectedTbs_t;

class LteNetDevice;

/**
 * \ingroup lte
 *
 * The LteSpectrumPhy models the physical layer of LTE
 */
class LteSpectrumPhy : public SpectrumPhy
{

public:
  LteSpectrumPhy ();
  virtual ~LteSpectrumPhy ();

  /**
   *  PHY states
   */
  enum State
  {
    IDLE, TX, RX
  };

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  // inherited from SpectrumPhy
  void SetChannel (Ptr<SpectrumChannel> c);
  void SetMobility (Ptr<MobilityModel> m);
  void SetDevice (Ptr<NetDevice> d);
  Ptr<MobilityModel> GetMobility ();
  Ptr<NetDevice> GetDevice ();
  Ptr<const SpectrumModel> GetRxSpectrumModel () const;
  void StartRx (Ptr<SpectrumSignalParameters> params);

  /**
   * set the Power Spectral Density of outgoing signals in W/Hz.
   *
   * @param txPsd
   */
  void SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd);

  /**
   * \brief set the noise power spectral density
   * @param noisePsd the Noise Power Spectral Density in power units
   * (Watt, Pascal...) per Hz.
   */
  void SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd);
 
  /**
   * Start a transmission
   *
   *
   * @param pb the burst of packets to be transmitted
   *
   * @return true if an error occurred and the transmission was not
   * started, false otherwise.
   */
  bool StartTx (Ptr<PacketBurst> pb);


  /**
   * set the callback for the end of a TX, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyTxEndCallback (GenericPhyTxEndCallback c);

  /**
   * set the callback for the end of a RX in error, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyRxEndErrorCallback (GenericPhyRxEndErrorCallback c);

  /**
   * set the callback for the successful end of a RX, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyRxEndOkCallback (GenericPhyRxEndOkCallback c);

  /**
   * \brief Set the state of the phy layer
   * \param newState the state
   */
  void SetState (State newState);

  /** 
   * 
   * 
   * \param cellId the Cell Identifier
   */
  void SetCellId (uint16_t cellId);


  /** 
   * 
   * 
   * \param p the new LteSinrChunkProcessor to be added to the processing chain
   */
  void AddSinrChunkProcessor (Ptr<LteSinrChunkProcessor> p);
  
  /** 
  * 
  * 
  * \param rnti the rnti of the source of the TB
  * \param size the size of the TB
  * \param mcs the MCS of the TB
  * \param map the map of RB(s) used
  */
  void AddExpectedTb (uint16_t  rnti, uint16_t size, uint8_t mcs, std::vector<int> map);
  
  /** 
  * 
  * 
  * \param sinr vector of sinr perceived per each RB
  */
  void UpdateSinrPerceived (const SpectrumValue& sinr);

private:
  void ChangeState (State newState);
  void EndTx ();
  void EndRx ();

  Ptr<MobilityModel> m_mobility;

  Ptr<NetDevice> m_device;

  Ptr<SpectrumChannel> m_channel;

  Ptr<SpectrumValue> m_txPsd;
  Ptr<PacketBurst> m_txPacketBurst;
  std::list<Ptr<PacketBurst> > m_rxPacketBurstList;

  State m_state;
  Time m_firstRxStart;
  Time m_firstRxDuration;

  TracedCallback<Ptr<const PacketBurst> > m_phyTxStartTrace;
  TracedCallback<Ptr<const PacketBurst> > m_phyTxEndTrace;
  TracedCallback<Ptr<const PacketBurst> > m_phyRxStartTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxEndOkTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxEndErrorTrace;

  GenericPhyTxEndCallback        m_genericPhyTxEndCallback;
  GenericPhyRxEndErrorCallback   m_genericPhyRxEndErrorCallback;
  GenericPhyRxEndOkCallback      m_genericPhyRxEndOkCallback;

  Ptr<LteInterference> m_interference;

  uint16_t m_cellId;
  
  expectedTbs_t m_expectedTbs;
  SpectrumValue m_sinrPerceived;
  
  UniformVariable m_random;
};






}

#endif /* LTE_SPECTRUM_PHY_H */
