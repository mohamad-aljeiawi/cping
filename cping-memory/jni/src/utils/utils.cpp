#include "utils/utils.h"
#include "debug/logger.h"

#include <algorithm>
#include <fstream>
#include <dirent.h>

namespace Utils
{

    bool is_number(const std::string &str)
    {
        return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
    }

    bool is_contains(const std::string &str, const std::string &sub)
    {
        return str.find(sub) != std::string::npos;
    }

    std::string read_file_content(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return "";
        }
        std::string content;
        std::getline(file, content, '\0');
        file.close();
        return content;
    }

    uintptr_t find_ue4_base(pid_t pid)
    {
        if (pid <= 0)
        {
            Logger::e("Invalid PID provided");
            return 0;
        }

        std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
        std::ifstream maps_file(maps_path);

        if (!maps_file.is_open())
        {
            Logger::e("Failed to open maps file for pid %d", pid);
            return 0;
        }

        std::string line;
        while (std::getline(maps_file, line))
        {
            if (line.find("libUE4.so") != std::string::npos)
            {
                uintptr_t base = std::stoull(line.substr(0, line.find("-")), nullptr, 16);
                maps_file.close();
                return base;
            }
        }

        maps_file.close();
        Logger::e("libUE4.so not found in process %d", pid);
        return 0;
    }

    bool is_package_running(const std::string &package_name)
    {
        return find_pid_by_package_name(package_name) > 0;
    }
    
    pid_t find_pid_by_package_name(const std::string &package_name)
    {
        DIR *dir = opendir("/proc");
        if (!dir)
        {
            Logger::e("Failed to open /proc directory");
            return -1;
        }

        pid_t target_pid = -1;
        struct dirent *entry;

        while ((entry = readdir(dir)))
        {
            if (!is_number(entry->d_name))
            {
                continue;
            }

            std::string cmdline_path = "/proc/" + std::string(entry->d_name) + "/cmdline";
            std::string cmdline = read_file_content(cmdline_path);

            if (cmdline == package_name)
            {
                target_pid = std::stoi(entry->d_name);
                break;
            }
        }

        closedir(dir);
        return target_pid;
    }

    bool is_printable_ascii(const char *str)
    {
        if (!str)
            return false;
        while (*str)
        {
            if (*str < 32 || *str > 126)
                return false;
            str++;
        }
        return true;
    }

} // namespace Utils
