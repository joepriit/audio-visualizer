#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>

#include "audio_recorder.h"
#include "audio_sink.h"
#include "SDL.h"

// Visualizer class
// This class owns the SDL window and performs visual updates based on system audio
class Visualizer : public AudioSink {
public:

    Visualizer();
    ~Visualizer();

    // The status of the initialization process
    // return: bool - whether the initialization was successful
    bool init_successful() const;

    // Update the visuals based on the most recent packet
    // return: bool - whether the update was successful
    bool update();

    // Copy a packet of data from the audio recorder
    // Must override the method from AudioSink
    // param: data - pointer to data values
    // param: channels - the number of audio channels
    // param: frames - the number of frames of data
    void copy_data(float* data, int channels, int frames) override;

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Event current_event;
    int window_width = 1000;
    int window_height = 600;
    bool mFullScreen = false;
    bool mMinimized = false;


    AudioRecorder recorder;
    std::thread recording_thread;
    std::atomic_bool exit_recording_thread_flag = false;
    std::mutex packet_buffer_mutex;
    typedef std::vector<float> packet;
    packet packet_buffer; // Must use mutex to access

    // Handle SDL window events
    // param: e - the SDL window event to handle
    void handle_event(const SDL_Event& e);

    // Draw a horizontal soundwave with the most recent packet data
    // param: start - the leftmost starting pixel of the wave
    // param: length - the length in pixels of the wave
    // param: pixel_amplitude - the maximum amplitude of the wave
    // param: color - the color of the wave
    void draw_wave(const SDL_Point& start, int length, int pixel_amplitude, const SDL_Color& color);
};