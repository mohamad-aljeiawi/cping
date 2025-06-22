#include "utils/memory.h"
#include <cstring>

namespace Memory
{

    std::string ReadFName(uintptr_t entry_ptr, pid_t pid)
    {
        const size_t max_limit = 4096;
        size_t buff_size = 1024;
        std::unique_ptr<char[]> buffer(new char[buff_size]);
        memset(buffer.get(), 0, buff_size);

        size_t read_size = 0;
        uintptr_t base_ptr = entry_ptr + 4 + sizeof(uintptr_t); // skip FNameEntry header

        while (read_size < max_limit)
        {
            size_t chunk_size = buff_size - read_size;
            if (chunk_size == 0)
            {
                size_t new_size = buff_size * 2;
                if (new_size > max_limit)
                    return "none";

                std::unique_ptr<char[]> new_buffer(new char[new_size]);
                memcpy(new_buffer.get(), buffer.get(), buff_size);
                buffer.swap(new_buffer);
                buff_size = new_size;
                chunk_size = buff_size - read_size;
            }

            if (!Process::Read((void *)(base_ptr + read_size), buffer.get() + read_size, chunk_size, pid))
            {
                return "none";
            }

            for (size_t i = read_size; i < read_size + chunk_size; ++i)
            {
                if (buffer[i] == '\0')
                {
                    return std::string(buffer.get());
                }
            }

            read_size += chunk_size;
        }

        return "none";
    }

} // namespace Memory
