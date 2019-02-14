#ifndef random_generator_manager_h
#define random_generator_manager_h

#include <cstdint>

#include "ns3/double.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/random-variable-stream.h"

class RandomGeneratorManager {
    static uint32_t m_seed;
    static uint32_t m_run;
public:
    static uint32_t GetSeed ();
    static uint32_t GetRun ();
    static void SetSeed (uint32_t seed);
    static void SetRun (uint32_t run);
    
    static ns3::Ptr<ns3::UniformRandomVariable> CreateUniformRandomVariable (double min, double max);
    
private:
    RandomGeneratorManager (); // Private constructor as this is a static class and cannot be instantiated
};
#endif /* random_generator_manager_h */
