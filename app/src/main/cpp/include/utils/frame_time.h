#ifndef FRAME_TIME_H
#define FRAME_TIME_H

#include <chrono>
#include <thread>

class FrameTimeController {
private:
    int target_fps;
    using clock = std::chrono::steady_clock;
    std::chrono::time_point<clock> frame_start_time;

public:
    explicit FrameTimeController(int fps = 60)
            : target_fps(fps) {}

    void start_frame() {
        frame_start_time = clock::now();
    }

    void end_frame() {
        const auto frame_duration = std::chrono::milliseconds(1000 / target_fps);
        auto frame_process_time = clock::now() - frame_start_time;

        if (frame_process_time < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_process_time);
        }
    }
};

#endif // FRAME_TIME_H
