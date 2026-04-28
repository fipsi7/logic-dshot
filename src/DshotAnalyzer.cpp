#include "DshotAnalyzer.h"
#include "DshotAnalyzerSettings.h"
#include "magic_enum.hpp"
#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

#include <stdint.h>
#include <cstdio>

// this typedef taken from src\main\drivers\pwm_output.h in the betaflight github page
enum class dshotCommands_e
{
    DSHOT_CMD_MOTOR_STOP = 0,
    DSHOT_CMD_BEACON1,
    DSHOT_CMD_BEACON2,
    DSHOT_CMD_BEACON3,
    DSHOT_CMD_BEACON4,
    DSHOT_CMD_BEACON5,
    DSHOT_CMD_ESC_INFO, // V2 includes settings
    DSHOT_CMD_SPIN_DIRECTION_1,
    DSHOT_CMD_SPIN_DIRECTION_2,
    DSHOT_CMD_3D_MODE_OFF,
    DSHOT_CMD_3D_MODE_ON,
    DSHOT_CMD_SETTINGS_REQUEST, // Currently not implemented
    DSHOT_CMD_SAVE_SETTINGS,
    DSHOT_CMD_SPIN_DIRECTION_NORMAL = 20,
    DSHOT_CMD_SPIN_DIRECTION_REVERSED = 21,
    DSHOT_CMD_LED0_ON,                       // BLHeli32 only
    DSHOT_CMD_LED1_ON,                       // BLHeli32 only
    DSHOT_CMD_LED2_ON,                       // BLHeli32 only
    DSHOT_CMD_LED3_ON,                       // BLHeli32 only
    DSHOT_CMD_LED0_OFF,                      // BLHeli32 only
    DSHOT_CMD_LED1_OFF,                      // BLHeli32 only
    DSHOT_CMD_LED2_OFF,                      // BLHeli32 only
    DSHOT_CMD_LED3_OFF,                      // BLHeli32 only
    DSHOT_CMD_AUDIO_STREAM_MODE_ON_OFF = 30, // KISS audio Stream mode on/Off
    DSHOT_CMD_SILENT_MODE_ON_OFF = 31,       // KISS silent Mode on/Off
    DSHOT_CMD_SIGNAL_LINE_TELEMETRY_DISABLE = 32,
    DSHOT_CMD_SIGNAL_LINE_CONTINUOUS_ERPM_TELEMETRY = 33,
    DSHOT_CMD_MAX = 47
};

DshotAnalyzer::DshotAnalyzer() : Analyzer2(), mSettings( new DshotAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
}

DshotAnalyzer::~DshotAnalyzer()
{
    KillThread();
}

void DshotAnalyzer::SetupResults()
{
    UseFrameV2();
    // SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the
    // results each time.
    mResults.reset( new DshotAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

double DshotAnalyzer::proportionOfBit( U32 width )
{
    return static_cast<double>( width ) / mSamplesPerBit;
}

void DshotAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );
    mSamplesPerBit = mSampleRateHz / ( mSettings->mDshotRate * 1000 );

    if( mSerial->GetBitState() == BIT_HIGH )
        mSerial->AdvanceToNextEdge();

    uint32_t width = 0;

    for( ;; )
    {
        uint16_t data = 0;
        uint64_t starting_sample = 0;
        int i;
        for( i = sizeof( data ) * 8 - 1; i >= 0; i-- )
        {
            mSerial->AdvanceToNextEdge(); // rising edge of first bit
            uint64_t rising_sample = mSerial->GetSampleNumber();
            if( !starting_sample )
                starting_sample = rising_sample;
            mSerial->AdvanceToNextEdge();
            uint64_t falling_sample = mSerial->GetSampleNumber();

            width = falling_sample - rising_sample;
            bool set = proportionOfBit( width ) > 0.5;
            // check if low pulse is too long / next bit is too far away
            bool error = i > 0 && proportionOfBit( mSerial->GetSampleOfNextEdge() - falling_sample ) > 1.5;
            // check if high pulse is too short
            error |= proportionOfBit( width ) < 0.2;

            if( set )
                data |= 1 << i;

            AnalyzerResults::MarkerType marker;
            if( error )
            {
                marker = AnalyzerResults::ErrorX;
            }
            else if( i == 4 )
            { // telem request
                if( set )
                    marker = AnalyzerResults::Start;
                else
                    marker = AnalyzerResults::Stop;
            }
            else
            { // channel bits or crc
                if( set )
                    marker = AnalyzerResults::One;
                else
                    marker = AnalyzerResults::Zero;
            }
            mResults->AddMarker( rising_sample + width / 2, marker, mSettings->mInputChannel );

            if( error )
                break;
        }

        if( i >= 0 )
        { // message ended early / bit was errored
            mResults->CommitResults();
            continue;
        }

        uint16_t chan = data & 0xffe0;
        uint8_t crc = ( ( chan >> 4 ) & 0xf ) ^ ( ( chan >> 8 ) & 0xf ) ^ ( ( chan >> 12 ) & 0xf );
        bool crcok = ( data & 0xf ) == crc;
        chan >>= 5;

        mSerial->Advance( mSamplesPerBit - width ); // end of low pulse

        if( !crcok )
            mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel );

        // we have a byte to save.
        Frame frame;
        frame.mData1 = chan;
        frame.mFlags = ( crcok ? 0 : DISPLAY_AS_ERROR_FLAG ) | ( ( data & 0x10 ) > 0 ); // error flag | telem request
        frame.mStartingSampleInclusive = starting_sample;
        frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

        mResults->AddFrame( frame );

        // New FrameV2 code.
        FrameV2 frame_v2;
        // you can add any number of key value pairs. Each will get it's own column in the data table.
        frame_v2.AddInteger( "channel", frame.mData1 );
        if( chan >= 0 && chan < 48 )
        {
            auto dshot_cmd = magic_enum::enum_cast<dshotCommands_e>( chan );
            if( dshot_cmd.has_value() )
            {
                frame_v2.AddString( "CMD", std::string{ magic_enum::enum_name( dshot_cmd.value() ) }.c_str() );
            }
            else
            {
                frame_v2.AddString( "CMD", "UNKNOWN" );
            }
        }
        // This actually saves your new FrameV2. In this example, we just copy the same start and end sample number from Frame V1 above.
        // The second parameter is the frame "type". Any string is allowed.
        mResults->AddFrameV2( frame_v2, "DSHOT", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );

        // You should already be calling this to make submitted frames available to the rest of the system. It's still required.
        mResults->CommitResults();

        ReportProgress( frame.mEndingSampleInclusive );
    }
}

bool DshotAnalyzer::NeedsRerun()
{
    return false;
}

U32 DshotAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                           SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 DshotAnalyzer::GetMinimumSampleRateHz()
{
    return mSettings->mDshotRate * 4000;
}

const char* DshotAnalyzer::GetAnalyzerName() const
{
    return "Dshot";
}

const char* GetAnalyzerName()
{
    return "Dshot";
}

Analyzer* CreateAnalyzer()
{
    return new DshotAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}