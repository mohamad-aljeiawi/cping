#include "debug/frame_time.h"
#include <thread>

FrameTimeController::FrameTimeController(int fps)
    : target_fps(fps) {}

void FrameTimeController::start_frame()
{
    frame_start_time = clock::now();
}

void FrameTimeController::end_frame()
{
    const auto frame_duration = std::chrono::milliseconds(1000 / target_fps);
    auto frame_process_time = clock::now() - frame_start_time;

    if (frame_process_time < frame_duration)
    {
        std::this_thread::sleep_for(frame_duration - frame_process_time);
    }
}
