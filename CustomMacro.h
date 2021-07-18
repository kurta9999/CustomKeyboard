#pragma once

#include "utils/CSingleton.h"

#include "Settings.h"

#include <inttypes.h>
#include <unordered_map>
#include <map>
#include <string>
#include <variant>
#include <array>

#include "Logger.h"
#include <thread>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

#pragma pack(push, 1)
typedef struct
{
    uint8_t state;
    uint8_t lctrl;
    uint8_t lshift;
    uint8_t lalt;
    uint8_t lgui;
    uint8_t rctrl;
    uint8_t rshift;
    uint8_t ralt;
    uint8_t rgui;
    uint8_t keys[6];
    uint16_t crc;
} KeyData_t;
#pragma pack(pop)

class KeyClass
{
public:
    virtual void DoWrite(void) = 0;

public:
    void PressReleaseKey(uint16_t scancode, bool press = true)
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = scancode;
        input.ki.dwFlags = (press ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_SCANCODE;
        if((scancode & 0xFF00) == 0xE000)
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &input, sizeof(input));
    }
  
    void TypeCharacter(WORD character)
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = character;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        if((character & 0xFF00) == 0xE000)
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &input, sizeof(input));
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(input));
    }

    void PressReleaseMouse(WORD mouse_button)
    {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = mouse_button;
        SendInput(1, &input, sizeof(input));
        input.mi.dwFlags = mouse_button << (uint16_t)1;
        SendInput(1, &input, sizeof(input));
    }
protected:
};

class KeyText final : public KeyClass
{
public:
    KeyText(std::string&& keys)
    {
        seq = std::move(keys);
    }

    void DoWrite() override
    {
        for(size_t i = 0; i < seq.length(); i++)
        {
            TypeCharacter(seq[i] & 0xFF);
        }
    }
private:
    std::string seq; /* virtual key codes to press and release*/
};

class KeyCombination final : public KeyClass
{
public:
    KeyCombination(std::vector<uint16_t>&& keys)
    {
        seq = std::move(keys);
    }
    void DoWrite() override
    {
        for(size_t i = 0; i < seq.size(); i++)
            PressReleaseKey(seq[i]);
        for(size_t i = 0; i < seq.size(); i++)
            PressReleaseKey(seq[i], false);
    }
private:
    std::vector<uint16_t> seq; /* scan codes to press and release*/
};

class KeyDelay final : public KeyClass
{
public:
    KeyDelay(uint32_t delay_) : delay(delay_)
    {
        
    }    
    KeyDelay(uint32_t start_, uint32_t end_) : delay(std::array<uint32_t, 2>{start_, end_})
    {
        //delay = ;
    }

    void DoWrite() override
    {
        if(std::holds_alternative<uint32_t>(delay))
            std::this_thread::sleep_for(std::chrono::milliseconds(std::get<uint32_t>(delay)));
        else
        {
            std::array<uint32_t, 2> d = std::get<std::array<uint32_t, 2>>(delay);
            boost::uniform_int<> dist(d[0], d[1]);
            boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);

            int ret = die();
            std::this_thread::sleep_for(std::chrono::milliseconds(ret));
        }
    }
private:
    std::variant<uint32_t, std::array<uint32_t, 2>> delay;
    boost::mt19937 gen;
};

class MouseMovement final : public KeyClass
{
public:
    MouseMovement(LPPOINT* pos_)
    {
        memcpy(&pos, pos_, sizeof(pos));
    }

    void DoWrite() override
    {
        POINT to_screen;
        HWND hwnd = GetForegroundWindow();
        memcpy(&to_screen, &pos, sizeof(to_screen));
        ClientToScreen(hwnd, &to_screen);
        ShowCursor(FALSE);
        SetCursorPos(to_screen.x, to_screen.y);
        ShowCursor(TRUE);
    }

private:
    POINT pos;
};

class MouseClick final : public KeyClass
{
public:
    MouseClick(uint16_t key_) : key(key_)
    {}

    void DoWrite() override
    {
        PressReleaseMouse(key);
    }
private:
    uint16_t key;
};

/* each given macro per-app get's a macro container */
class MacroContainer
{
public:
    MacroContainer() = default;
    template <class T>
    MacroContainer(std::string& name, T* p)
    {
        //key_vec.push_back(p);
    }
    std::map<std::string, std::vector<std::unique_ptr<KeyClass>>> key_vec;  /* std::string -> it could be better, like storing hash, but I did it for myself - it's OK */
    std::map<std::string, std::string> bind_name;  /* [Key code] = bind name text */
    std::string name;
private:
};

class CustomMacro : public CSingleton < CustomMacro >
{
    friend class CSingleton < CustomMacro >;

public:
    CustomMacro() = default;
    ~CustomMacro()
    {
        if(t)
            TerminateThread(t->native_handle(), 0);
    }
    void Init(void);
    std::vector<std::unique_ptr<MacroContainer>>& GetMacros()
    {
        return macros;
    }

private:
    friend class Settings;
    friend class MinerWatchdog;

    void PressKey(std::string key);
    void UartDataReceived(const char* data, unsigned int len);
    void UartReceiveThread(void);

    std::vector<std::unique_ptr<MacroContainer>> macros;
    uint8_t is_enabled;
    uint16_t com_port = 2005;
    bool use_per_app_macro;
    bool advanced_key_binding;
    std::string pressed_keys;
    std::thread* t = nullptr;

    uint16_t GetKeyScanCode(std::string& str)
    {
        uint16_t ret = 0xFFFF;
        static const std::unordered_map<std::string, int> scan_codes =
        {
            {"LCTRL",       0x1D},
            {"RCTRL",       0xE01D},
            {"LALT",        0x38},
            {"RALT",        0xE038},
            {"LSHIFT",      0x2A},
            {"RSHIFT",      0x36},
            {"BACKSPACE",   0xE0},
            {"TAB",         0x0F},
            {"ENTER",       0x1C},
            {"ESC",         0x01},
            {"SPACE",       0x39},
            {"PAGEUP",      0xE049},
            {"PAGEDOWN",    0xE051},
            {"END",         0xE04F},
            {"HOME",        0xE047},
            {"PRINT",       0x0}, // TODO
            {"INSERT",      0xE052},
            {"DELETE",      0xE053},
            {"NUM_1",       0x4F},
            {"NUM_2",       0x50},
            {"NUM_3",       0x51},
            {"NUM_4",       0x4B},
            {"NUM_5",       0x4C},
            {"NUM_6",       0x4D},
            {"NUM_7",       0x47},
            {"NUM_8",       0x48},
            {"NUM_9",       0x49},
            {"NUM_0",       0x52},
            {"NUM_MUL",     0x37},
            {"NUM_DOT",     0x53},
            {"NUM_PLUS",    0x4E},
            {"NUM_MINUS",   0x4A},
            {"NUM_DIV",     0xE035},
            {"A",           0x1E},
            {"B",           0x30},
            {"C",           0x2E},
            {"D",           0x20},
            {"E",           0x12},
            {"F",           0x21},
            {"G",           0x22},
            {"H",           0x23},
            {"I",           0x17},
            {"J",           0x24},
            {"K",           0x25},
            {"L",           0x26},
            {"M",           0x32},
            {"N",           0x31},
            {"O",           0x18},
            {"P",           0x19},
            {"Q",           0x10},
            {"R",           0x13},
            {"S",           0x1F},
            {"T",           0x14},
            {"U",           0x16},
            {"V",           0x2F},
            {"W",           0x11},
            {"X",           0x2D},
            {"Y",           0x15},
            {"Z",           0x2C},
            {"1",           0x02},
            {"2",           0x03},
            {"F1",          0x3B},
            {"F2",          0x3C},
            {"F3",          0x3D},
            {"F4",          0x3E},
            {"F5",          0x3F},
            {"F6",          0x40},
            {"F7",          0x41},
            {"F8",          0x42},
            {"F9",          0x43},
            {"F10",         0x44},
            {"F11",         0x59},
            {"F12",         0x58},
            {"LEFT",        0xE04B},
            {"RIGHT",       0xE04D},
            {"UP",          0xE048},
            {"DOWN",        0xE050},
            {"AFTERBURNER", 0xFFFE},
        };

        auto it = scan_codes.find(str);
        if(it != scan_codes.end())
            ret = it->second;
        return ret;
    }

    const std::unordered_map<int, std::string> hid_scan_codes =
    {
        {0x04, "A"},
        {0x05, "B"},
        {0x06, "C"},
        {0x07, "D"},
        {0x08, "E"},
        {0x09, "F"},
        {0x0A, "G"},
        {0x0B, "H"},
        {0x0C, "I"},
        {0x0D, "J"},
        {0x0E, "K"},
        {0x0F, "L"},
        {0x10, "M"},
        {0x11, "N"},
        {0x12, "O"},
        {0x13, "P"},
        {0x14, "Q"},
        {0x15, "R"},
        {0x16, "S"},
        {0x17, "T"},
        {0x18, "U"},
        {0x19, "V"},
        {0x1A, "W"},
        {0x1B, "X"},
        {0x1C, "Y"},
        {0x1D, "Z"},
        {0x1E, "1"},
        {0x1F, "2"},
        {0x20, "3"},
        {0x21, "4"},
        {0x22, "5"},
        {0x23, "6"},
        {0x24, "7"},
        {0x25, "8"},
        {0x26, "9"},
        {0x27, "0"},
        {0x3A, "F1"},
        {0x3B, "F2"},
        {0x3C, "F3"},
        {0x3D, "F4"},
        {0x3E, "F5"},
        {0x3F, "F6"},
        {0x40, "F7"},
        {0x41, "F8"},
        {0x42, "F9"},
        {0x43, "F10"},
        {0x44, "F11"},
        {0x45, "F12"},
        {0x46, "PRINT"},
        {0x47, "SCROLL"},
        {0x48, "PUASE"},
        {0x49, "INSERT"},
        {0x4A, "HOME"},
        {0x4B, "PAGEUP"},
        {0x4C, "DELETE"},
        {0x4D, "END"},
        {0x4E, "PAGEDOWN"},
        {0x4F, "RIGHT"},
        {0x50, "LEFT"},
        {0x51, "UP"},
        {0x52, "DOWN"},
        {0x53, "NUM_LOCK"},
        {0x54, "NUM_DIV"},
        {0x55, "NUM_MUL"},
        {0x56, "NUM_MINUS"},
        {0x57, "NUM_PLUS"},
        {0x58, "ENTER"},
        {0x59, "NUM_1"},
        {0x5A, "NUM_2"},
        {0x5B, "NUM_3"},
        {0x5C, "NUM_4"},
        {0x5D, "NUM_5"},
        {0x5E, "NUM_6"},
        {0x5F, "NUM_7"},
        {0x60, "NUM_8"},
        {0x61, "NUM_9"},
        {0x62, "NUM_0"},
        {0x63, "NUM_DEL"},
        {0xF0, "LCTRL"},
        {0xF1, "LSHIFT"},
        {0xF2, "LALT"},
        {0xF3, "LGUI"},
        {0xF4, "RCTRL"},
        {0xF5, "RSHIFT"},
        {0xF6, "RALT"},
        {0xF7, "RGUI"}
    };
};