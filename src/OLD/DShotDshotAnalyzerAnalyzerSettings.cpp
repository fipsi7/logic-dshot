#include "DshotAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


DshotAnalyzerSettings::DshotAnalyzerSettings()
    : mInputChannel( UNDEFINED_CHANNEL ), mBitRate( 9600 ), mInputChannelInterface(), mBitRateInterface()
{
    mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard DshotAnalyzer" );
    mInputChannelInterface.SetChannel( mInputChannel );

    mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)", "Specify the bit rate in bits per second." );
    mBitRateInterface.SetMax( 6000000 );
    mBitRateInterface.SetMin( 1 );
    mBitRateInterface.SetInteger( mBitRate );

    AddInterface( &mInputChannelInterface );
    AddInterface( &mBitRateInterface );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mInputChannel, "Serial", false );
}

DshotAnalyzerSettings::~DshotAnalyzerSettings()
{
}

bool DshotAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface.GetChannel();
    mBitRate = mBitRateInterface.GetInteger();

    ClearChannels();
    AddChannel( mInputChannel, "DshotAnalyzer", true );

    return true;
}

void DshotAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface.SetChannel( mInputChannel );
    mBitRateInterface.SetInteger( mBitRate );
}

void DshotAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mInputChannel;
    text_archive >> mBitRate;

    ClearChannels();
    AddChannel( mInputChannel, "DshotAnalyzer", true );

    UpdateInterfacesFromSettings();
}

const char* DshotAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannel;
    text_archive << mBitRate;

    return SetReturnString( text_archive.GetString() );
}
