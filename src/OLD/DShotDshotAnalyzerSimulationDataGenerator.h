#ifndef DSHOTANALYZER_SIMULATION_DATA_GENERATOR
#define DSHOTANALYZER_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class DshotAnalyzerSettings;

class DshotAnalyzerSimulationDataGenerator
{
  public:
    DshotAnalyzerSimulationDataGenerator();
    ~DshotAnalyzerSimulationDataGenerator();

    void Initialize( U32 simulation_sample_rate, DshotAnalyzerSettings* settings );
    U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

  protected:
    DshotAnalyzerSettings* mSettings;
    U32 mSimulationSampleRateHz;

  protected:
    void CreateSerialByte();
    std::string mSerialText;
    U32 mStringIndex;

    SimulationChannelDescriptor mSerialSimulationData;
};
#endif // DSHOTANALYZER_SIMULATION_DATA_GENERATOR