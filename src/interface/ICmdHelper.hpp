#pragma once

#include <string>
#include <memory>
#include <variant>

class Command;
class Separator;

using CommandTypes = std::variant<std::shared_ptr<Command>, Separator>;

class ICmdHelper
{
public:
    //virtual ~ICmdHelperr();
    virtual void OnPreReload(uint8_t cols) = 0;
    virtual void OnCommandLoaded(uint8_t col, CommandTypes cmd) = 0;
    virtual void OnPostReload(uint8_t cols) = 0;

protected:
};