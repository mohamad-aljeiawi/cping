#ifndef LOG_H
#define LOG_H

#define LOG_TAG "LOG_TAG"
#define TEST_TAG "TEST_TAG_SOCKET"
#define TEST_EXP "TEST_EXP"

#define LOGI(tag, ...) __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__)
#define LOGW(tag, ...) __android_log_print(ANDROID_LOG_WARN, tag, __VA_ARGS__)
#define LOGD(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#define LOGE(tag, ...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)

#endif // LOG_H
