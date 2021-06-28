#include "audio_recorder.h"

#include <comdef.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <Mmdeviceapi.h>

#include <chrono>
#include <iostream>
#include <thread>

// Define IIDs for initialization
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);


AudioRecorder::AudioRecorder() {
    m_hr = S_OK;
    m_audio_client = nullptr;

    // Initialize audio device endpoint
    CoInitialize(nullptr);
    m_hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, nullptr,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&m_device_enumerator);

    if (m_hr == S_OK && m_device_enumerator) {
        m_hr = m_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_audio_device_endpoint);
    }

    // init audio client
    WAVEFORMATEX* pwfx = nullptr;
    REFERENCE_TIME hns_requested_duration = 100000000;

    if (m_hr == S_OK && m_audio_device_endpoint) {
        m_hr = m_audio_device_endpoint->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&m_audio_client);
    }

    if (m_hr == S_OK && m_audio_client) {
        m_hr = m_audio_client->GetMixFormat(&pwfx);

        if (m_hr == S_OK && pwfx) {
            if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
                || reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx)->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
                std::cout << "Error: the audio format is not supported" << std::endl;
                m_hr = S_FALSE;
            }
        }

        if (m_hr == S_OK) {
            m_hr = m_audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hns_requested_duration, 10, pwfx, nullptr);
        }

        if (m_hr == S_OK && pwfx) {
            m_num_channels = pwfx->nChannels;
        }

        if (m_hr == S_OK) {
            m_hr = m_audio_client->GetService(IID_IAudioCaptureClient, (void**)&m_capture_client);
        }

        if (m_hr == S_OK && m_capture_client) {
            m_hr = m_audio_client->Start();  // Start recording.
        }
    }

    if (m_hr != S_OK) {
        std::cout << "Error: During AudioRecorder intialization - " << _com_error(m_hr).ErrorMessage() << std::endl;

    }
};

// safely release and nullify pointers (in destructor)
template<class T> inline void safe_release(T*& p_object) {
    if (p_object) {
        p_object->Release();
        p_object = nullptr;
    }
}

AudioRecorder::~AudioRecorder() {
    if (m_audio_client) {
        m_audio_client->Stop();  // Stop recording.
    }

    safe_release(m_device_enumerator);
    safe_release(m_audio_device_endpoint);
    safe_release(m_audio_client);
    safe_release(m_capture_client);
}

bool AudioRecorder::init_successful() const {
    return (m_hr == S_OK) && m_audio_client && m_capture_client;
}

void AudioRecorder::record(AudioSink* audio_sink, std::atomic_bool& exit_flag) const {
    m_hr = S_OK;
    UINT32 packet_length = 0;
    BYTE* data = nullptr;
    UINT32 num_frames_available;
    DWORD flags;

    while (!exit_flag) {
        m_hr = m_capture_client->GetNextPacketSize(&packet_length);
        while (packet_length != 0 && !exit_flag) // while there are available packets
        {
            // Get the available data in the shared buffer.
            data = nullptr;
            m_hr = m_capture_client->GetBuffer(
                &data,
                &num_frames_available,
                &flags, nullptr, nullptr);

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                data = nullptr;  // data pointer is null for silence
            }
            audio_sink->copy_data((float*)data, m_num_channels, num_frames_available);

            m_hr = m_capture_client->ReleaseBuffer(num_frames_available);
            m_hr = m_capture_client->GetNextPacketSize(&packet_length);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
}