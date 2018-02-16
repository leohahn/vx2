#ifndef __RESOURCE_FILE_HPP__
#define __RESOURCE_FILE_HPP__

#include <unordered_map>
#include <vector>
#include <memory>
#include "lt_core.hpp"

#define RESOURCE_TOKENS \
        RT(TokenType_Invalid = 0, "Invalid"),                                  \
        RT(TokenType_Identifier, "Identifier"),                         \
        RT(TokenType_OpenBracket, "Open Bracket ([)"),                                 \
        RT(TokenType_CloseBracket, "Close Bracket (])"),                                \
        RT(TokenType_Comma, "Comma (,)"),                                       \
        RT(TokenType_Equals, "Equals (=)"),                                      \
        RT(TokenType_Semicolon, "Semicolon (;)"),

lt_internal const char *ResourceFileTokenNames[] =
{
#define RT(e, s) s
    RESOURCE_TOKENS
#undef RT
};

struct ResourceFile
{
    enum TokenType
    {
#define RT(e, s) e
        RESOURCE_TOKENS
#undef RT
    };

    enum ValType
    {
        ValType_String,
        ValType_Array,
    };

    struct Val
    {
        ValType type;
        Val(ValType type) : type(type) {}
        virtual ~Val() = default;
    };

    struct StringVal : public Val
    {
        StringVal(const std::string &str)
            : Val(ValType_String)
            , str(str)
        {}

        std::string str;
    };

    struct ArrayVal : public Val
    {
        ArrayVal()
            : Val(ValType_Array)
        {}

        std::vector<std::unique_ptr<Val>> vals;
    };

    struct Token
    {
        TokenType type;
        std::string str;

        Token(TokenType type, const std::string &str)
            : type(type)
            , str(str)
        {}

        Token(TokenType type)
            : type(type)
            , str()
        {}
    };

    ResourceFile(const std::string &filepath);

    inline bool
    has(const std::string &key)
    {
        return m_entries.find(key) != m_entries.end();
    }

    template<typename T> inline const T *
    cast_get(const std::string &key)
    {
        // NOTE: maybe use operator[]?
        auto ptr = dynamic_cast<T*>(m_entries.at(key).get());
        LT_Assert(ptr != nullptr);
        return ptr;
    }


    std::string filepath;
    bool is_file_correct = true;

private:
    std::unordered_map<std::string, std::unique_ptr<Val>> m_entries;

    void parse();
    std::vector<Token> tokenize();
};

#endif // __RESOURCE_FILE_HPP__
