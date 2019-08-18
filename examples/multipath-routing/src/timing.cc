#include "ns3/simulator.h"

#include "timing.h"

using namespace ns3;

std::string
SecondsToString (double timeInSeconds)
{
  // Get the hours
  uint32_t numHours = (int) timeInSeconds / 3600;
  timeInSeconds -= (numHours * 3600);

  // Get the minutes
  uint32_t numMinutes = (int) timeInSeconds / 60;
  timeInSeconds -= (numMinutes * 60);

  return std::to_string (numHours) + "H:" + std::to_string (numMinutes) +
         "M:" + std::to_string (timeInSeconds) + "S";
}

Timing::Timing (double simulationStopTime) : m_simulationStopTime (simulationStopTime)
{
  Simulator::Schedule (Seconds (0), &Timing::StartTiming, this);
}

void
Timing::StartTiming ()
{
  m_timings.emplace_back (std::chrono::high_resolution_clock::now ());

  std::cout << "Simulator at " << Simulator::Now ().GetSeconds () << "s" << std::endl;
  Simulator::Schedule (Seconds (1), &Timing::LogTime, this);
}

void
Timing::OutputTotalDuration ()
{
  auto currentTime{std::chrono::high_resolution_clock::now ()};
  const auto &start{m_timings.front ()};

  std::chrono::duration<double> totalDuration = currentTime - start;
  std::cout << "Total Simulation Duration: " << SecondsToString (totalDuration.count ())
            << std::endl;
}

void
Timing::LogTime ()
{
  auto currentTime{std::chrono::high_resolution_clock::now ()};

  const auto &previousTiming{m_timings.back ()};
  const auto &start{m_timings.front ()};

  std::chrono::duration<double> totalDuration = currentTime - start;
  std::chrono::duration<double> previousDuration = currentTime - previousTiming;

  auto currentSimulationTime{Simulator::Now ().GetSeconds ()};
  auto remainingTime{m_simulationStopTime - currentSimulationTime};

  auto estimatedCompletionTime{previousDuration.count () * remainingTime};

  std::cout << "Simulator at " << currentSimulationTime
            << "s. Total Duration: " << SecondsToString (totalDuration.count ())
            << "s. Estimated completion time: " << SecondsToString (estimatedCompletionTime) << "s"
            << std::endl;

  Simulator::Schedule (Seconds (1), &Timing::LogTime, this);

  m_timings.push_back (currentTime);
}
