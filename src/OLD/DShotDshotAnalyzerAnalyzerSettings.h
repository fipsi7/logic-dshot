#ifndef DSHOTANALYZER_ANALYZER_SETTINGS
#define DSHOTANALYZER_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class DshotAnalyzerSettings : public AnalyzerSettings
{
  public:
    DshotAnalyzerSettings();
    virtual ~DshotAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();


    Channel mInputChannel;
    U32 mBitRate;

  protected:
    AnalyzerSettingInterfaceChannel mInputChannelInterface;
    AnalyzerSettingInterfaceInteger mBitRateInterface;
};

#endif // DSHOTANALYZER_ANALYZER_SETTINGS
