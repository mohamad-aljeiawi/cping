#ifndef MAIN_H
#define MAIN_H

#include <jni.h>

#include "debug/logger.h"
#include "utils/utils.h"
#include "types/structs.h"


#include "utils/socket_server.h"
#include "utils/socket_client.h"
#include "utils/enum.h"

#include "native/imgui_renderer.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <EGL/egl.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __ANDROID__

#include <pthread.h>
#include <sys/resource.h>

#endif

#endif // MAIN_H