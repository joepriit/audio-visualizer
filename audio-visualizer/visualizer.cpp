#pragma once
#include "visualizer.h"

#include <iostream>
#include <thread>
#include <vector>

#include "SDL.h"

Visualizer::Visualizer() : recorder() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }

    window = SDL_CreateWindow("Visualizer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    if (renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
    }

    if (recorder.init_successful()) {
        recording_thread = std::thread(&AudioRecorder::record, &recorder, (AudioSink*)this, std::ref(exit_recording_thread_flag));
    }
}

Visualizer::~Visualizer()
{
    // Stop recording thread before implicitly destroying AudioRecorder
    if (recording_thread.joinable()) {
        exit_recording_thread_flag = true;
        recording_thread.join();
    }

    // Destroy SDL window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Visualizer::init_successful() const {
    return (renderer && recording_thread.joinable());
}


void Visualizer::handle_event(const SDL_Event& e) {
    //Window event occured
    if (e.type == SDL_WINDOWEVENT) {
        switch (e.window.event) {
            //Get new dimensions and repaint on window size change
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            window_width = e.window.data1;
            window_height = e.window.data2;
            SDL_RenderPresent(renderer);
            break;

            //Repaint on exposure
        case SDL_WINDOWEVENT_EXPOSED:
            SDL_RenderPresent(renderer);
            break;

        case SDL_WINDOWEVENT_MINIMIZED:
            mMinimized = true;
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            mMinimized = false;
            break;

        case SDL_WINDOWEVENT_RESTORED:
            mMinimized = false;
            break;
        }

    }
    //Enter exit full screen on return key
    else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
        if (mFullScreen) {
            SDL_SetWindowFullscreen(window, SDL_FALSE);
            mFullScreen = false;
        }
        else {
            SDL_SetWindowFullscreen(window, SDL_TRUE);
            mFullScreen = true;
            mMinimized = false;
        }
    }
}

void Visualizer::copy_data(float* data, int channels, int frames) {
    std::lock_guard<std::mutex>read_guard(packet_buffer_mutex);

    if (data) {
        packet_buffer = packet(data, data + channels * frames);
    }
    else {
        // use an empty vector if there is no data (silence)
        packet_buffer = packet();
    }
}


bool Visualizer::update() {
    //Handle events on queue
    while (SDL_PollEvent(&current_event) != 0) {
        if (current_event.type == SDL_QUIT) {
            return false;
        }
        handle_event(current_event);
    }

    // Do not render if minimized
    if (mMinimized) {
        return true;
    }

    // Render visualizer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    draw_wave(SDL_Point{ 0, window_height / 2 }, window_width, window_height, SDL_Color{ 255,255,255,255 });
    SDL_RenderPresent(renderer);

    return true;
}

void Visualizer::draw_wave(const SDL_Point& start, int length, int pixel_amplitude, const SDL_Color& color) {
    std::vector<SDL_Point> points;

    std::unique_lock<std::mutex>read_guard(packet_buffer_mutex);
    if (!packet_buffer.empty()) {
        // use smallest possible step so soundwave fills window
        int step = (int)ceil((double)length / (double)packet_buffer.size());
        auto amplitude = packet_buffer.begin();
        for (int i = 0; i < length; i += step) {
            points.push_back(SDL_Point{ start.x + i, (int)(start.y + (*amplitude) * pixel_amplitude) });
            ++amplitude;
        }
    }
    else { // silence
        points.push_back(SDL_Point{ start.x, start.y });
        points.push_back(SDL_Point{ start.x + length - 1, start.y });

    }
    read_guard.unlock();

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLines(renderer, &points[0], (int)points.size());
}