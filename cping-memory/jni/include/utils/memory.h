#ifndef MEMORY_H
#define MEMORY_H

#include <type_traits>
#include <string>
#include <vector>
#include <memory>
#include "utils/process.h"

namespace Memory
{
    template <typename T>
    T Read(uintptr_t address, pid_t pid)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        T value{};
        if (Process::Read((void *)address, &value, sizeof(T), pid))
        {
            return value;
        }
        return T{};
    }

    template <typename Type, size_t N>
    bool ReadArray(uintptr_t address, Type (&out)[N], pid_t pid)
    {
        static_assert(std::is_trivially_copyable<Type>::value, "Type must be trivially copyable");
        const size_t bytes_to_read = sizeof(Type) * N;
        return Process::Read(reinterpret_cast<void *>(address), reinterpret_cast<void *>(out), bytes_to_read, pid);
    }

    template <typename T>
    bool Write(uintptr_t address, const T &value, pid_t pid)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        return Process::Write((void *)address, (void *)&value, sizeof(T), pid);
    }

    std::string ReadFName(uintptr_t entry_ptr, pid_t pid); // التصريح فقط

} // namespace Memory

#endif // MEMORY_H
