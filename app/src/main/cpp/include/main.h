#ifndef MAIN_H
#define MAIN_H

#include <jni.h>
#include <android/log.h>
#include <cmath>
#include <string>
#include <vector>
#include <GLES3/gl3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <cstring>
#include <mutex>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_opengl3.h"


using namespace std;

#include "utils/log.h"
#include "utils/enum.h"
#include "utils/utils.h"
#include "utils/native_bridge.h"
#include "utils/structs.h"
#include "utils/socket_client.h"
#include "utils/socket_server.h"
#include "utils/frame_time.h"
#include "utils/ue4.h"


#endif // MAIN_H