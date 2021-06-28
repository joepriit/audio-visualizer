#pragma once

// Abstract base class to copy data from an AudioRecorder
class AudioSink
{
public:
    // Copy a packet of data from the audio recorder
    // param: data - pointer to data values
    // param: channels - the number of audio channels
    // param: frames - the number of frames of data
    virtual void copy_data(float* data, int channels, int frames) = 0;
};