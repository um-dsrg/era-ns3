#include "random-generator-manager.h"

uint32_t RandomGeneratorManager::m_seed = 1;
uint32_t RandomGeneratorManager::m_run = 1;

uint32_t
RandomGeneratorManager::GetSeed ()
{
  return m_seed;
}

uint32_t
RandomGeneratorManager::GetRun ()
{
  return m_run;
}

void
RandomGeneratorManager::SetSeed (uint32_t seed)
{
  m_seed = seed;
}

void
RandomGeneratorManager::SetRun (uint32_t run)
{
  m_run = run;
}

ns3::Ptr<ns3::UniformRandomVariable>
RandomGeneratorManager::CreateUniformRandomVariable (double min, double max)
{
  ns3::SeedManager::SetSeed (m_seed);
  ns3::SeedManager::SetRun (m_run);

  ns3::Ptr<ns3::UniformRandomVariable> randVar = ns3::CreateObject<ns3::UniformRandomVariable> ();
  randVar->SetAttribute ("Min", ns3::DoubleValue (min));
  randVar->SetAttribute ("Max", ns3::DoubleValue (max));

  m_run++;
  return randVar;
}
