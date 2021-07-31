#pragma once

#include "utils/CSingleton.h"
#include <string>

class PathSeparator : public CSingleton < PathSeparator >
{
    friend class CSingleton < PathSeparator >;

public:
    PathSeparator() = default;
    void ReplaceClipboard();

    std::string replace_key = "F11";

private:
    void ReplaceString(std::string& str);
};