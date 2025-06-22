#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <dirent.h>

namespace Utils
{
    bool is_number(const std::string &str);
    bool is_contains(const std::string &str, const std::string &sub);
    std::string read_file_content(const std::string &path);
    pid_t find_pid_by_package_name(const std::string &package_name);
    uintptr_t find_ue4_base(pid_t pid);
    bool is_package_running(const std::string &package_name);
    bool is_printable_ascii(const char *str);
}

#endif // UTILS_H
