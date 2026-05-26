#pragma once

#include <string>
#include <algorithm>

/// Trim from the start (in place)
/// https://stackoverflow.com/questions/216823/how-can-i-trim-a-stdstring
static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    {
                                        return !std::isspace(ch);
                                    }));
}

/// Trim from the end (in place)
/// https://stackoverflow.com/questions/216823/how-can-i-trim-a-stdstring
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         {
                             return !std::isspace(ch);
                         }).base(), s.end());
}

/// Trim from both ends (in place)
static inline void trim(std::string& s)
{
    rtrim(s);
    ltrim(s);
}

/// Convert string to lower case
static inline void tolower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](char c)
    {
        return tolower(c);
    });
}

static inline bool contains(const std::string& s, const std::string& substring)
{
    return s.find(substring) != std::string::npos;
}

static std::vector<std::string> split(std::string s, const std::string& delimiter)
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}