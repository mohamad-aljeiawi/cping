#ifndef FRAME_TIME_H
#define FRAME_TIME_H

#include <chrono>

class FrameTimeController
{
private:
    int target_fps;
    using clock = std::chrono::steady_clock;
    std::chrono::time_point<clock> frame_start_time;

public:
    explicit FrameTimeController(int fps = 60);
    void start_frame();
    void end_frame();
};

#endif // FRAME_TIME_H
