#ifndef __RESOURCE_FILE_HPP__
#define __RESOURCE_FILE_HPP__

#include <unordered_map>

struct ResourceFile
{
    ResourceFile(const std::string &filepath);
    std::string filepath;
    std::unordered_map<std::string, std::string> entries;
    bool is_file_correct = true;

    inline bool has(const std::string &key)
    {
        return entries.find(key) != entries.end();
    }

    const std::string &
    operator[](const std::string &key)
    {
        // NOTE: maybe use operator[]?
        return entries.at(key);
    }
};

#endif // __RESOURCE_FILE_HPP__
