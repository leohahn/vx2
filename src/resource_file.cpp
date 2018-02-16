#include "resource_file.hpp"

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "lt_core.hpp"
#include "lt_utils.hpp"

lt_global_variable lt::Logger logger("resource_file");

lt_global_variable const std::vector<std::string> entry_keys = {
    // texture entries
    "texture_type",
    "texture_format",
    "pixel_format",
    "face_x_pos",
    "face_x_neg",
    "face_y_pos",
    "face_y_neg",
    "face_z_pos",
    "face_z_neg",
    // shader entries
    "shader_source"
};

ResourceFile::ResourceFile(const std::string &filepath)
    : filepath(filepath)
{
    std::ifstream infile(filepath);
    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string key, val;
        char eq;

        bool success_read = (bool)(iss >> key >> eq >> val);

        // Skip comments
        if (key.size() > 0 && key[0] == '#')
            continue;

        if (!success_read && eq == '=')
        {
            logger.error("Loading file ", filepath, ": incorrect syntax");
            is_file_correct = false;
            break;
        }

        if (std::find(entry_keys.begin(), entry_keys.end(), key) == entry_keys.end())
        {
            logger.error("On file ", filepath);
            logger.error("Invalid key ", key);
            is_file_correct = false;
            break;
        }

        entries.insert(std::make_pair(key, val));
    }
}
