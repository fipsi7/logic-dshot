#ifndef DSHOTANALYZER_ANALYZER_H
#define DSHOTANALYZER_ANALYZER_H

#include <Analyzer.h>
#include "DshotAnalyzerSettings.h"
#include "DshotAnalyzerResults.h"
#include "DshotAnalyzerSimulationDataGenerator.h"
#include <memory>

class ANALYZER_EXPORT DshotAnalyzer : public Analyzer2
{
  public:
    DshotAnalyzer();
    virtual ~DshotAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

  protected: // vars
    DshotAnalyzerSettings mSettings;
    std::unique_ptr<DshotAnalyzerResults> mResults;
    AnalyzerChannelData* mSerial;

    DshotAnalyzerSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // DSHOTANALYZER_ANALYZER_H
