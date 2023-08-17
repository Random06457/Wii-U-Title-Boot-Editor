#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "types.hpp"

class File
{
public:
    static std::vector<u8> readAllBytes(const std::filesystem::path& path);
    static void writeAllBytes(const std::filesystem::path& path,
                              const void* data, size_t size);

    static std::string readAllText(const std::filesystem::path& path);

    static bool exists(const std::filesystem::path& path);
};

class Directory
{
public:
    static void create(const std::filesystem::path& path);
    static bool exists(const std::filesystem::path& path);
};
