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
 * Author: Marco Miozzo <mmiozzo@cttc.es>
 */


#include <ns3/trace-fading-loss-model.h>
#include <ns3/mobility-model.h>
#include <ns3/spectrum-value.h>
#include <ns3/log.h>
#include <ns3/string.h>
#include <ns3/double.h>
#include <fstream>
#include <ns3/simulator.h>

NS_LOG_COMPONENT_DEFINE ("TraceFadingLossModel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TraceFadingLossModel);
  


TraceFadingLossModel::TraceFadingLossModel ()
  : m_traceLength (10.0),
    m_samplesNum (10000),
    m_windowSize (0.5),
    m_rbNum (100)
{
  NS_LOG_FUNCTION (this);
  SetNext (NULL);
}


TraceFadingLossModel::~TraceFadingLossModel ()
{
  m_fadingTrace.clear ();
}


TypeId
TraceFadingLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TraceFadingLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .AddConstructor<TraceFadingLossModel> ()
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from. By default, uses /JakesTraces/fading_trace_EPA_3kmph.fad",
                   StringValue (""),
                   MakeStringAccessor (&TraceFadingLossModel::m_traceFile),
                   MakeStringChecker ())
    .AddAttribute ("TraceLength",
                  "The total length of the fading trace (default value 10 s.)",
                  TimeValue (Seconds (10.0)),
                  MakeTimeAccessor (&TraceFadingLossModel::m_traceLength),
                  MakeTimeChecker ())
    .AddAttribute ("SamplesNum",
                  "The number of samples the trace is made of (default 10000)",
                  DoubleValue (10000),
                  MakeDoubleAccessor (&TraceFadingLossModel::m_samplesNum),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("WindowSize",
                  "The size of the window for the fading trace (default value 0.5 s.)",
                  TimeValue (Seconds (0.5)),
                  MakeTimeAccessor (&TraceFadingLossModel::m_windowSize),
                  MakeTimeChecker ())
  ;
  return tid;
}


void
TraceFadingLossModel::LoadTrace ()
{
  NS_LOG_FUNCTION (this << "Loading Fading Trace " << m_traceFile);
  std::ifstream ifTraceFile;
  ifTraceFile.open (m_traceFile.c_str (), std::ifstream::in);
  m_fadingTrace.clear ();
  if (!ifTraceFile.good ())
    {
      NS_ASSERT_MSG(ifTraceFile.good (), " Fading trace file not found");
    }

  NS_LOG_INFO (this << " length " << m_traceLength.GetSeconds ());
  NS_LOG_INFO (this << " RB " << m_rbNum << " samples " << m_samplesNum);
  for (uint32_t i = 0; i < m_rbNum; i++)
    {
      FadingTraceSample rbTimeFadingTrace;
      for (uint32_t j = 0; j < m_samplesNum; j++)
        {
          double sample;
          ifTraceFile >> sample;
          rbTimeFadingTrace.push_back (sample);
        }
      m_fadingTrace.push_back (rbTimeFadingTrace);
    }
  ifTraceFile.close ();
  m_timeGranularity = m_traceLength.GetMilliSeconds () / m_samplesNum;
}


Ptr<SpectrumValue>
TraceFadingLossModel::DoCalcRxPowerSpectralDensity (
  Ptr<const SpectrumValue> txPsd,
  Ptr<const MobilityModel> a,
  Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this << *txPsd << a << b);
  
  //Ptr<FadingTraceManager> c = GetFadingChannelRealization (a,b);
  std::map <ChannelRealizationId_t, int >::iterator itOff;
  ChannelRealizationId_t mobilityPair = std::make_pair (a,b);
  itOff = m_windowOffsetsMap.find (mobilityPair);
  if (Simulator::Now ().GetSeconds () >= m_lastWindowUpdate.GetSeconds () + m_windowSize.GetSeconds ())
    {
      NS_LOG_INFO ("Fading Window Updated");
      std::map <ChannelRealizationId_t, UniformVariable* >::iterator itVar;

     itVar = m_startVariableMap.find (mobilityPair);
     (*itOff).second = (*itVar).second->GetValue ();
      //SetLastUpdate ();
      m_lastWindowUpdate = Simulator::Now ();
    }

  
  Ptr<SpectrumValue> rxPsd = Copy<SpectrumValue> (txPsd);
  Values::iterator vit = rxPsd->ValuesBegin ();
  
  //Vector aSpeedVector = a->GetVelocity ();
  //Vector bSpeedVector = b->GetVelocity ();
  
  //double speed = sqrt (pow (aSpeedVector.x-bSpeedVector.x,2) +  pow (aSpeedVector.y-bSpeedVector.y,2));

  NS_LOG_FUNCTION (this << *rxPsd);

  int now_ms = static_cast<int> (Simulator::Now ().GetMilliSeconds () * m_timeGranularity);
  int lastUpdate_ms = static_cast<int> (m_lastWindowUpdate.GetMilliSeconds () * m_timeGranularity);
  int index = (*itOff).second + now_ms - lastUpdate_ms;
  int subChannel = 0;
  while (vit != rxPsd->ValuesEnd ())
    {
      NS_ASSERT (subChannel < 100);
      if (*vit != 0.)
        {
          double fading = m_fadingTrace.at (subChannel).at (index);
          //NS_LOG_INFO (this << " offset " << (*itOff).second << " fading " << fading);
          double power = *vit; // in Watt/Hz
          power = 10 * log10 (180000 * power); // in dB

          NS_LOG_FUNCTION (this << subChannel << *vit  << power << fading);

          *vit = pow (10., ((power + fading) / 10)) / 180000; // in Watt

          NS_LOG_FUNCTION (this << subChannel << *vit);

        }

      ++vit;
      ++subChannel;

    }

  NS_LOG_FUNCTION (this << *rxPsd);
  return rxPsd;
}


void
TraceFadingLossModel::CreateFadingChannelRealization (Ptr<const MobilityModel> enbMobility, Ptr<const MobilityModel> ueMobility)
{
  NS_LOG_FUNCTION (this << enbMobility << ueMobility);
  
  if (m_fadingTrace.empty ())
    {
      TraceFadingLossModel::LoadTrace ();
      m_lastWindowUpdate = Simulator::Now ();
    }
   

  NS_LOG_FUNCTION (this <<
                   "insert new channel realization, m_windowOffsetMap.size () = "
                        << m_windowOffsetsMap.size ());
  UniformVariable* startV = new UniformVariable (1, (m_traceLength.GetSeconds () - m_windowSize.GetSeconds ()) * 1000.0);
  ChannelRealizationId_t mobilityPair = std::make_pair (enbMobility,ueMobility);
  m_startVariableMap.insert (std::pair<ChannelRealizationId_t,UniformVariable* > (mobilityPair, startV));
  m_windowOffsetsMap.insert (std::pair<ChannelRealizationId_t,int> (mobilityPair, 0));  

}


} // namespace ns3
