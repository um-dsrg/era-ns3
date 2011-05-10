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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_RLC_H
#define LTE_RLC_H

#include <ns3/simple-ref-count.h>
#include <ns3/packet.h>
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nstime.h"

#include "ns3/object.h"


namespace ns3 {


class LteMacSapProvider;
class LteMacSapUser;

/**
 * This abstract base class defines the API to interact with the Radio Link Control
 * (LTE_RLC) in LTE, see 3GPP TS 36.322
 *
 */
class LteRlc : public Object // SimpleRefCount<LteRlc>
{
  friend class LteRlcSpecificLteMacSapUser;
public:
  LteRlc ();
  virtual ~LteRlc ();
  static TypeId GetTypeId (void);

  /**
   *
   *
   * \param rnti
   */
  void SetRnti (uint8_t rnti);

  /**
   *
   *
   * \param lcId
   */
  void SetLcId (uint8_t lcId);

  /**
   *
   *
   * \param s the MAC SAP Providerto be used by this LTE_RLC
   */
  void SetLteMacSapProvider (LteMacSapProvider * s);

  /**
   *
   *
   * \param s the MAC SAP User interface offered to the MAC by this LTE_RLC
   */
  LteMacSapUser* GetLteMacSapUser ();




protected:
  // methods forwarded by LteMacSapUser
  virtual void DoNotifyTxOpportunity (uint32_t bytes) = 0;
  virtual void DoNotifyHarqDeliveryFailure () = 0;
  virtual void DoReceivePdu (Ptr<Packet> p) = 0;

  LteMacSapUser* m_macSapUser;
  LteMacSapProvider* m_macSapProvider;

  uint16_t m_rnti;
  uint8_t m_lcid;

  /**
   * Used to inform of a PDU delivery to the MAC SAP provider
   */
  TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu;
  /**
   * Used to inform of a PDU reception from the MAC SAP user
   */
  TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu;

};



/**
 * LTE_RLC Saturation Mode (SM): simulation-specific mode used for
 * experiments that do not need to consider the layers above the LTE_RLC.
 * The LTE_RLC SM, unlike the standard LTE_RLC modes, it does not provide
 * data delivery services to upper layers; rather, it just generates a
 * new LTE_RLC PDU whenever the MAC notifies a transmission opportunity.
 *
 */
class LteRlcSm : public LteRlc
{
public:
  LteRlcSm ();
  virtual ~LteRlcSm ();
  static TypeId GetTypeId (void);

  virtual void DoNotifyTxOpportunity (uint32_t bytes);
  virtual void DoNotifyHarqDeliveryFailure ();
  virtual void DoReceivePdu (Ptr<Packet> p);

  void Start ();
  
  long int bytesrxed;

};




// /**
//  * Implements LTE_RLC Transparent Mode (TM), see  3GPP TS 36.322
//  *
//  */
// class LteRlcTm : public LteRlc
// {
// public:
//   virtual ~LteRlcTm ();

// };


// /**
//  * Implements LTE_RLC Unacknowledged Mode (UM), see  3GPP TS 36.322
//  *
//  */
// class LteRlcUm : public LteRlc
// {
// public:
//   virtual ~LteRlcUm ();

// };

// /**
//  * Implements LTE_RLC Acknowledged Mode (AM), see  3GPP TS 36.322
//  *
//  */

// class LteRlcAm : public LteRlc
// {
// public:
//   virtual ~LteRlcAm ();
// };





} // namespace ns3

#endif // LTE_RLC_H
