#include "fs.hpp"
#include <fstream>
#include <iterator>
#include <sstream>

bool File::exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path) &&
           !std::filesystem::is_directory(path);
}

std::string File::readAllText(const std::filesystem::path& path)
{
    std::ifstream fs;
    fs.open(path);

    if (!fs.is_open())
        std::abort();

    std::ostringstream ss;
    ss << fs.rdbuf();
    return ss.str();
}

std::vector<u8> File::readAllBytes(const std::filesystem::path& path)
{
    std::ifstream fs;
    fs.open(path, std::ios::binary);

    if (!fs.is_open())
        std::abort();

    fs.seekg(0, std::ios::end);
    std::streampos file_size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    std::vector<u8> ret(static_cast<size_t>(file_size));
    fs.read(reinterpret_cast<char*>(ret.data()), file_size);

    return ret;
}

void File::writeAllBytes(const std::filesystem::path& path, const void* data,
                         size_t size)
{
    std::ofstream fs;
    fs.open(path, std::ios::binary);

    if (!fs.is_open())
        std::abort();

    fs.write(reinterpret_cast<const char*>(data),
             static_cast<std::streamsize>(size));
}

void Directory::create(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
}

bool Directory::exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}
