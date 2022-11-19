#pragma once

#include "utils/CSingleton.hpp"
#include <string>
#include <thread>
#include <memory>
#include <variant>

#include "ICmdHelper.hpp"

class Command
{
public:
    Command(const std::string& name, const std::string& cmd, uint32_t color, uint32_t bg_color, bool is_bold, float scale) :
        m_name(name), m_cmd(cmd), m_color(color), m_bg_color(bg_color), m_is_bold(is_bold), m_scale(scale)
    {

    }

    void Execute();

    const std::string& GetName() { return m_name; }
    Command& SetName(const std::string& name) { m_name = name; return *this; }    

    const std::string& GetCmd() { return m_cmd; }
    Command& SetCmd(const std::string& cmd) { m_cmd = cmd; return *this; }

    uint32_t GetColor() { return m_color; }
    Command& SetColor(uint32_t color) { m_color = color; return *this; }

    uint32_t GetBackgroundColor() { return m_bg_color; }
    Command& SetBackgroundColor(uint32_t bg_color) { m_bg_color = bg_color; return *this; }

    bool IsBold() { return m_is_bold; }
    Command& SetBold(bool is_bold) { m_is_bold = is_bold; return *this; }

    float GetScale() { return m_scale; }
    Command& SetScale(float scale) { m_scale = scale; return *this; }

private:
    
    // !\ brief Handles hardcoded commands like set current time - ugly, but no time for better solution
    void HandleHarcdodedCommand();

    std::string m_name;
    std::string m_cmd;
    uint32_t m_color;
    uint32_t m_bg_color;
    bool m_is_bold;
    float m_scale;
};

class Separator
{
public:
    Separator(int width_) :
        width(width_)
    {

    }

    int width;
};

using CommandStorage = std::vector<std::vector<CommandTypes>>;

class ICommandLoader
{
public:
    virtual bool Load(const std::filesystem::path& path, CommandStorage& e) = 0;
    virtual bool Save(const std::filesystem::path& path, CommandStorage& e) = 0;
};

class XmlCommandLoader : public ICommandLoader
{
public:
    XmlCommandLoader(ICmdHelper* mediator);
    virtual ~XmlCommandLoader();

    bool Load(const std::filesystem::path& path, CommandStorage& e) override;
    bool Save(const std::filesystem::path& path, CommandStorage& e) override;

private:
    uint8_t m_Cols = 0;
    ICmdHelper* m_Mediator = nullptr;
};

class ICmdExecutor
{
public:
    virtual void Init() = 0;
    virtual void SetMediator(ICmdHelper* mediator) = 0;
    virtual void AddCommand(uint8_t col, Command cmd) = 0;
    virtual bool ReloadCommandsFromFile() = 0;
    virtual bool Save() = 0;
    virtual uint8_t GetColumns() = 0;
};

class CmdExecutor : public ICmdExecutor
{
public:
    CmdExecutor() = default;
    virtual ~CmdExecutor();

    void Init() override;
    void SetMediator(ICmdHelper* mediator) override;
    void AddCommand(uint8_t col, Command cmd) override;
    bool ReloadCommandsFromFile() override;
    bool Save() override;
    uint8_t GetColumns() override;

private:
    bool AddItem(uint8_t col, std::shared_ptr<Command>&& cmd) ;

    uint8_t m_Cols = 2;
    CommandStorage m_Commands;
    ICmdHelper* m_CmdMediator = nullptr;
};