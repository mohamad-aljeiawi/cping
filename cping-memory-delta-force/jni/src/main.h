#ifndef MAIN_H
#define MAIN_H

#include "debug/logger.h"
#include "types/offset.h"
#include "types/structs.h"
#include "utils/utils.h"
#include "utils/branding.h"

#include "utils/process.h"
#include "utils/memory.h"
#include "utils/ue4.h"
#include "utils/socket_server.h"
#include "utils/socket_client.h"
#include "utils/touch.h"

#include "native/a_native_window_creator.h"
#include "native/imgui_renderer.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <fstream>
#ifdef __ANDROID__
#include <pthread.h>
#include <sys/resource.h>
#endif

constexpr int MAX_ITERS = 10;                  // number of convergence iterations
constexpr float TO_DEGREES = 57.2957795f;      // conversion factor from radians to degrees
constexpr float DEG2RAD = 3.14159265f / 180.f; // conversion factor from degrees to radians
constexpr float G_CM = 980.f;                  // acceleration due to gravity in cm/s² in PUBG

const std::vector<uint8_t> key = {
    0xfb, 0x3d, 0x32, 0xff, 0x97, 0xdf, 0xb9, 0x27, 0x31, 0xe9, 0x8c, 0x92,
    0xb6, 0x9f, 0x5b, 0x4a, 0xeb, 0x91, 0x75, 0xd2, 0x98, 0xa0, 0xf7, 0xa0,
    0x9b, 0xcb, 0x3f, 0x71, 0x60, 0xde, 0xa9, 0xdc};

const std::vector<uint8_t> cipher = {};
#endif // MAIN_H