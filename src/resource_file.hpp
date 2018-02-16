#ifndef __RESOURCE_FILE_HPP__
#define __RESOURCE_FILE_HPP__

#include <unordered_map>

struct ResourceFile
{
    ResourceFile(const std::string &filepath);
    std::string filepath;
    std::unordered_map<std::string, std::string> entries;
    bool is_file_correct = true;

};

#endif // __RESOURCE_FILE_HPP__
