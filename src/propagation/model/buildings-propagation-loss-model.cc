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
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 * 
 */

#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include <math.h>
#include "buildings-propagation-loss-model.h"
#include "ns3/buildings-mobility-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BuildingsPropagationLossModel");
NS_OBJECT_ENSURE_REGISTERED (BuildingsPropagationLossModel);

TypeId
BuildingsPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BuildingsPropagationLossModel")

    .SetParent<PropagationLossModel> ()

    .AddConstructor<BuildingsPropagationLossModel> ()

    .AddAttribute ("Lambda",
                   "The wavelength  (default is 2.3 GHz at 300 000 km/s).",
                   DoubleValue (300000000.0 / 2.3e9),
                   MakeDoubleAccessor (&BuildingsPropagationLossModel::m_lambda),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("Frequency",
                   "The Frequency  (default is 2.3 GHz).",
                   DoubleValue (2.3e9),
                   MakeDoubleAccessor (&BuildingsPropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinDistance",
                   "The distance under which the propagation model refuses to give results (m) ",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&BuildingsPropagationLossModel::SetMinDistance, &BuildingsPropagationLossModel::GetMinDistance),
                   MakeDoubleChecker<double> ());
  return tid;
}

BuildingsPropagationLossModel::BuildingsPropagationLossModel () :
  C (0),
  m_environment (Urban),
  m_citySize (Large)
{

}

void
BuildingsPropagationLossModel::SetLambda (double frequency, double speed)
{
  m_lambda = speed / frequency;
  m_frequency = frequency;
}

void
BuildingsPropagationLossModel::SetLambda (double lambda)
{
  m_lambda = lambda;
  m_frequency = 300000000 / lambda;
}

double
BuildingsPropagationLossModel::GetLambda (void) const
{
  return m_lambda;
}

void
BuildingsPropagationLossModel::SetMinDistance (double minDistance)
{
  m_minDistance = minDistance;
}
double
BuildingsPropagationLossModel::GetMinDistance (void) const
{
  return m_minDistance;
}

void
BuildingsPropagationLossModel::SetEnvironment (Environment env)
{
  m_environment = env;
}
BuildingsPropagationLossModel::Environment
BuildingsPropagationLossModel::GetEnvironment (void) const
{
  return m_environment;
}


double
BuildingsPropagationLossModel::OkumuraHata (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  // Hp: a is the rooftop antenna (from GetLoss logic)
  double loss = 0.0;
  if (m_frequency<=1500)
    {
      // standard Okumura Hata (from wikipedia)
      double log_f = log10 (m_frequency);
      double log_aHeight = 13.82 * log10 (a->GetPosition ().z);
      double log_bHeight = 0.0;
      if (m_citySize == Large)
        {
          if (m_frequency<200)
            {
              log_bHeight = 8.29 * pow (log10 (1.54 * b->GetPosition ().z), 2) -  1.1;
            }
          else
            {
              log_bHeight = 3.2 * pow (log10 (11.75 * b->GetPosition ().z), 2) - 4.97;
            }
        }
      else
        {
          log_bHeight = 0.8 + (1.1*log10(m_frequency) - 0.7)*b->GetPosition ().z - 1.56*log10(m_frequency);
        }
      
      loss = 69.55 + (26.16 * log_f) - log_aHeight + (((44.9 - (6.55 * log_f) ))*log10 (a->GetDistanceFrom (b))) - log_bHeight;
      
      if (m_environment == SubUrban)
        {
          loss += 2 * (pow(log10 (m_frequency / 28), 2)) - 5.4;
        }
      else if (m_environment == OpenAreas)
        {
          loss += -4.70*pow(log10(m_frequency),2) + 18.33*log10(m_frequency) - 40.94;
        }
          
    }
  else if (m_frequency <= 2170) // max 3GPP freq EUTRA band #1
    {
      // COST 231 Okumura model
      double log_f = log10 (m_frequency);
      double log_aHeight = 13.83 * log10 (a->GetPosition ().z);
      double log_bHeight = 0.0;
      if (m_citySize == Large)
        {
          log_bHeight = 3.2 * pow ((log10(11.75 * b->GetPosition ().z)),2);
        }
      else
        {
          log_bHeight = 1.1*log10(m_frequency) - 0.7*b->GetPosition ().z - (1.56*log10(m_frequency) - 0.8);
        }
      
      loss = 46.3 + (33.9 * log_f) - log_aHeight + (((44.9 - (6.55 * log_f) ))*log10 (a->GetDistanceFrom (b))) - log_bHeight;
    }
  else if (m_frequency <= 2690) // max 3GPP freq EUTRA band #1
    {
      // Empirical model from
      // "Path Loss Models for Suburban Scenario at 2.3GHz, 2.6GHz and 3.5GHz"
      // Sun Kun, Wang Ping, Li Yingze
      // Antennas, Propagation and EM Theory, 2008. ISAPE 2008. 8th International Symposium on 
      loss = 36 + 26*log10(a->GetDistanceFrom (b));
    }
      
  return (loss);
}


double
BuildingsPropagationLossModel::ItuR1411 (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  
  return (0.0);
}


double
BuildingsPropagationLossModel::ItuR1238 (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{

  return (0.0);
}


double
BuildingsPropagationLossModel::BEWPL (Ptr<MobilityModel> a) const
{

  return (0.0);
}



double
BuildingsPropagationLossModel::GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{

  double distance = a->GetDistanceFrom (b);
  if (distance <= m_minDistance)
    {
      return 0.0;
    }
    
  // get the BuildingsMobilityModel pointers
  Ptr<BuildingsMobilityModel> a1 = DynamicCast<BuildingsMobilityModel> (a);
  Ptr<BuildingsMobilityModel> b1 = DynamicCast<BuildingsMobilityModel> (b);
  
  double loss = 0.0;
  
  if (a1->IsOutdoor ())
    {
      if (b1->IsIndoor ())
        {
          if (distance > 1000)
            {
              if (a1->GetPosition ().z > m_rooftopThreshold)
                {
                  // Over the rooftop tranmission -> Okumura Hata
                  loss = OkumuraHata (a1, b1);
                }
              else
                {
                  // TODO put a limit in distance (i.e., < 2000 for small cells)
                  
                  // short range outdoor communication within street canyon
                  loss = ItuR1411 (a1, b1);
                }
            }
          else
            {
              // short range outdoor communication within street canyon
              loss = ItuR1411 (a1, b1);
            }
        }
      else
        {
          // b indoor
          if (distance > 1000)
            {
              loss = OkumuraHata (a1, b1) + BEWPL(b1);
            }
          else
            {
              loss = ItuR1411 (a1, b1) + BEWPL(b1);
            }
        } // end b1->isIndoor ()
    }
  else
    {
      // a is indoor
      if (b1->IsIndoor ())
        {
          if (a1->GetBuilding () == b1->GetBuilding ())
            {
                // nodes are in same building -> indoor communication ITU-R P.1238
                loss = ItuR1238 (a1, b1);
            }
          else
            {
              loss = ItuR1411 (a1, b1) + BEWPL(a1) + BEWPL(b1);
            }
        }
      else
        {
          // b is outdoor
          if (distance > 1000)
            {
              if (b1->GetPosition ().z > m_rooftopThreshold)
                {
                  loss = OkumuraHata (a1, b1) + BEWPL(a1);
                }
              else
                {
                  loss = ItuR1411 (a1, b1) + BEWPL(a1);
                }
            }
          else
            {
              loss = ItuR1411 (a1, b1) + BEWPL(a1);
            }
        } // end b1->IsIndoor ()
    } // end a1->IsOutdoor ()

  return (loss);

}

double
BuildingsPropagationLossModel::DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  return txPowerDbm + GetLoss (a, b);
}

} // namespace ns3
