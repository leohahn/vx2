#include "resource_file.hpp"

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <array>

#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "lt_fs.hpp"

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
    // std::ifstream infile(filepath);
    // std::string line;
    // while (std::getline(infile, line))
    // {
    //     std::istringstream iss(line);
    //     std::string key, val;
    //     char eq;

    //     bool success_read = (bool)(iss >> key >> eq >> val);

    //     // Skip comments
    //     if (key.size() > 0 && key[0] == '#')
    //         continue;

    //     if (!success_read && eq == '=')
    //     {
    //         logger.error("Loading file ", filepath, ": incorrect syntax");
    //         is_file_correct = false;
    //         break;
    //     }

    //     if (std::find(entry_keys.begin(), entry_keys.end(), key) == entry_keys.end())
    //     {
    //         logger.error("On file ", filepath);
    //         logger.error("Invalid key ", key);
    //         is_file_correct = false;
    //         break;
    //     }

    //     m_entries.insert(std::make_pair(key, val));
    // }
}

lt_internal inline u8 *
eat_whitespaces(u8 *it, i32 *jumps_to_do)
{
    u8 *new_it = it;
    while (std::isspace(*new_it) && *jumps_to_do > 0)
    {
        (*jumps_to_do)--;
        new_it++;
    }

    return new_it;
}

lt_internal inline u8 *
eat_until(char c, u8 *it, i32 *jumps_to_do)
{
    u8 *new_it = it;
    while (*new_it != c && *jumps_to_do > 0)
    {
        (*jumps_to_do)--;
        new_it++;
    }
    return new_it;
}


template<typename T>
lt_internal inline u8 *
eat_until(const T &chars, u8 *it, i32 *jumps_to_do)
{
    u8 *new_it = it;
    while (std::find(std::begin(chars), std::end(chars), *new_it) == std::end(chars) &&
           *jumps_to_do > 0)
    {
        (*jumps_to_do)--;
        new_it++;
    }

    return new_it;
}

std::vector<ResourceFile::Token>
ResourceFile::tokenize()
{
    std::vector<Token> tokens;

    FileContents *contents = file_read_contents(filepath.c_str());
    if (contents->error != FileError_None)
    {
        logger.error("Failed to open file ", filepath);
        return tokens;
    }

    u8 *it = (u8*)contents->data;
    i32 jumps_to_do = contents->size;

    while (jumps_to_do > 0)
    {
        it = eat_whitespaces(it, &jumps_to_do);

        if (*it == '#')
        {
            it = eat_until('\n', it, &jumps_to_do);
        }
        else if (std::isalpha(*it))
        {
            std::array<char, 3> chars{{' ', '=', ';'}};
            u8 *last_it = eat_until(chars, it, &jumps_to_do);

            Token tk(TokenType_Identifier, std::make_unique<StringVal>(std::string(it, last_it)));
            tokens.push_back(std::move(tk));

            it = last_it;
        }
        else if (*it == '=')
        {
            Token tk(TokenType_Equals);
            tokens.push_back(std::move(tk));
            if (jumps_to_do)
            {
                it++;
                jumps_to_do--;
            }
        }
        else if (*it == ';')
        {
            Token tk(TokenType_Semicolon);
            tokens.push_back(std::move(tk));
            if (jumps_to_do)
            {
                it++;
                jumps_to_do--;
            }
        }
    }

    file_free_contents(contents);
    return tokens;
}

void
ResourceFile::parse()
{
    std::vector<Token> tokens = tokenize();

    i32 tokens_left = tokens.size();
    i32 t = 0;

    if (tokens_left < 4)
    {
        logger.error("For file ", filepath);
        logger.error("Rule must have at least 4 tokens. ", tokens_left);
        return;
    }

    // logger.log("For file ", filepath);

    // for (auto &token : tokens)
    //     logger.log(ResourceFileTokenNames[token.type]);

    while (tokens_left >= 4)
    {
        if (tokens[t].type == TokenType_Identifier &&
            tokens[t+1].type == TokenType_Equals)
        {
            LT_Assert(tokens[t].val->type == ValType_String);
            auto key = dynamic_cast<StringVal*>(tokens[t].val.get());

            if (has(key->str))
            {
                logger.error("File already has the key ", key->str);
                return;
            }

            if (tokens[t+2].type == TokenType_Identifier && tokens[t+3].type == TokenType_Semicolon)
            {
                LT_Assert(tokens[t+2].val->type == ValType_String);
                auto val = dynamic_cast<StringVal*>(tokens[t+2].val.get());

                logger.log("adding entry ", key->str);
                logger.log("value ", val->str);

                m_entries.insert(std::make_pair(key->str, std::move(tokens[t+2].val)));
                tokens_left -= 4;
                t += 4;
            }
            else if (tokens[t+2].type == TokenType_Identifier && tokens[t+3].type == TokenType_OpenBracket)
            {
                // Must recognize array
                LT_Assert(false);
            }
            else
            {
                logger.error("Wrong syntax on value.");
                return;
            }
        }
        else
        {
            logger.error("Wrong syntax on file.");
        }
    }
}
