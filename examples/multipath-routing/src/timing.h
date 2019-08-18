#ifndef timing_h
#define timing_h

#include <chrono>
#include <vector>

class Timing
{
public:
  explicit Timing (double simulationStopTime);
  void StartTiming ();
  void OutputTotalDuration ();

private:
  void LogTime ();

  double m_simulationStopTime;
  std::vector<std::chrono::high_resolution_clock::time_point> m_timings;
};

#endif /* timing_h */
