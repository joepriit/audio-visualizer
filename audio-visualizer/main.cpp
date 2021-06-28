#include <chrono>
#include <thread>

#include "visualizer.h"

constexpr int sleep_time = 20;

int main(int argc, char* argv[])
{
    Visualizer generator;
    bool continue_app = generator.init_successful();

    while (continue_app)
    {
        continue_app = generator.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }

    return 0;
}