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

// lt_global_variable const std::vector<std::string> entry_keys = {
//     // texture entries
//     "texture_type",
//     "texture_format",
//     "pixel_format",
//     "face_x_pos",
//     "face_x_neg",
//     "face_y_pos",
//     "face_y_neg",
//     "face_z_pos",
//     "face_z_neg",
//     // shader entries
//     "shader_source"
// };

ResourceFile::ResourceFile(const std::string &filepath)
    : filepath(filepath)
{
    parse();
}

lt_internal inline u8 *
eat_whitespaces(u8 *it, const u8 *end_it)
{
    u8 *new_it = it;
    while (std::isspace(*new_it) && it <= end_it)
    {
        new_it++;
    }
    return new_it;
}

lt_internal inline u8 *
eat_until(char c, u8 *it, const u8 *end_it)
{
    u8 *new_it = it;
    while (*new_it != c && it <= end_it)
    {
        new_it++;
    }
    return new_it;
}


template<typename T>
lt_internal inline u8 *
eat_until(const T &chars, u8 *it, const u8 *end_it)
{
    u8 *new_it = it;
    while (std::find(std::begin(chars), std::end(chars), *new_it) == std::end(chars) &&
           it <= end_it)
    {
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

    const u8 *end_it = (u8*)contents->data + contents->size - 1;
    u8 *it = (u8*)contents->data;

    while (it <= end_it)
    {
        it = eat_whitespaces(it, end_it);

        if (*it == '#')
        {
            it = eat_until('\n', it, end_it);
        }
        else if (std::isalpha(*it))
        {
            std::array<char, 6> chars{{' ', '=', ';', ',', ']', '\n'}};
            u8 *last_it = eat_until(chars, it, end_it);

            Token tk(TokenType_Identifier, std::string(it, last_it));
            tokens.push_back(tk);

            it = last_it;
        }
        else if (*it == '=')
        {
            Token tk(TokenType_Equals);
            tokens.push_back(tk);
            it++;
        }
        else if (*it == '[')
        {
            Token tk(TokenType_OpenBracket);
            tokens.push_back(tk);
            it++;
        }
        else if (*it == ']')
        {
            Token tk(TokenType_CloseBracket);
            tokens.push_back(tk);
            it++;
        }
        else if (*it == ',')
        {
            Token tk(TokenType_Comma);
            tokens.push_back(tk);
            it++;
        }
        else if (*it == ';')
        {
            Token tk(TokenType_Semicolon);
            tokens.push_back(tk);
            it++;
        }
    }

    file_free_contents(contents);
    return tokens;
}

void
ResourceFile::parse()
{
    std::vector<Token> tokens = tokenize();

    if (tokens.size() < 4)
    {
        logger.error("For file ", filepath);
        logger.error("Rule must have at least 4 tokens.");
        return;
    }

    // logger.log("For file ", filepath);

    // for (auto &token : tokens)
    //     logger.log(ResourceFileTokenNames[token.type]);

    auto loops_left = [](i32 t, const std::vector<Token> &tokens) {
        return tokens.size() - 1 - t;
    };

    for (u32 t = 0; t < tokens.size();)
    {
        if (tokens[t].type == TokenType_Identifier &&
            tokens[t+1].type == TokenType_Equals)
        {
            auto key = tokens[t].str;
            if (has(key))
            {
                logger.error("File already has the key ", key);
                return;
            }

            if (tokens[t+2].type == TokenType_Identifier && tokens[t+3].type == TokenType_Semicolon)
            {
                auto val = tokens[t+2].str;

                logger.log("adding entry ", key);
                logger.log("value ", val);

                auto string_val = std::make_unique<StringVal>(val);
                m_entries.insert(std::make_pair(key, std::move(string_val)));

                t += 4; // two tokens were recognized
            }
            else if (tokens[t+2].type == TokenType_OpenBracket)
            {
                t += 3;

                auto array_val = std::make_unique<ArrayVal>();

                while (t < tokens.size())
                {
                    if (tokens[t].type == TokenType_Identifier)
                    {
                        auto new_val = std::make_unique<StringVal>(tokens[t].str);
                        array_val->vals.push_back(std::move(new_val));

                        if (loops_left(t, tokens) >= 2 &&
                            tokens[t+1].type == TokenType_CloseBracket &&
                            tokens[t+2].type == TokenType_Semicolon)
                        {
                            t += 3;
                            break;
                        }
                        else if (loops_left(t, tokens) >= 1 && tokens[t+1].type == TokenType_Comma)
                        {
                            t += 2;
                        }
                        else
                        {
                            logger.error("Invalid array.");
                            return;
                        }
                    }
                    else
                    {
                        logger.error("Invalid array.");
                        return;
                    }
                }

                logger.log("adding array entry ", key);
                m_entries.insert(std::make_pair(key, std::move(array_val)));

                t += 4; // two tokens were recognized
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
            return;
        }
    }
}
