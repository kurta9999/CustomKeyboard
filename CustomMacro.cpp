#include "utils/AsyncSerial.h"

#include "CustomMacro.h"
#include "Logger.h"
#include "PrintScreenSaver.h"

#include <string>
#include <bitset>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>

using namespace std::chrono_literals;
using crc16_modbus_t = boost::crc_optimal<16, 0x8005, 0xFFFF, 0, true, true>;

void CustomMacro::UartDataReceived(const char* data, unsigned int len)
{
    KeyData_t* k = (KeyData_t*)data;
    crc16_modbus_t calc_result;
    calc_result.process_bytes((void*)data, len - 2);
    uint16_t crc = calc_result.checksum();
    if(!strncmp(data, "reset", 5) && crc == 0x6bd8) /* in case of suddenly reset of STM32 */
    {
        pressed_keys.clear();
    }
   
    if(k->crc == crc)
    {
        static const char all_released[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        if(!memcmp(&data[1], all_released, sizeof(all_released)))
        {
            pressed_keys.clear();
        }
        else
        {
            if(!pressed_keys.empty() && pressed_keys[pressed_keys.length() - 1] != '+')
                pressed_keys += '+';

            if(k->lshift)
                pressed_keys += "LSHIFT";
            else if(k->lalt)
                pressed_keys += "LALT";
            else if(k->lgui)
                pressed_keys += "LGUI";
            else if(k->lctrl)
                pressed_keys += "LALT"; 
            if(k->rshift)
                pressed_keys += "RSHIFT";
            else if(k->ralt)
                pressed_keys += "RALT";
            else if(k->rgui)
                pressed_keys += "RGUI";
            else if(k->rctrl)
                pressed_keys += "RALT";
            else  /* only supporting 1 addon key (for now?), it should be more than enough - possible to bind more than 700 macros - PER APPLICATION! */
            {
                for(auto& i : hid_scan_codes)
                {
                    if(i.second == k->keys[0])
                    {
                        pressed_keys += i.first;
                        break;
                    }
                }
            }

            if(PrintScreenSaver::Get()->screenshot_key == pressed_keys)
                PrintScreenSaver::Get()->SaveScreenshot();

            if(use_per_app_macro)
            {
                HWND foreground = GetForegroundWindow();
                if(foreground)
                {
                    char window_title[256];
                    GetWindowTextA(foreground, window_title, 256);
                    DBG("Focus: %s\n", window_title);
                    for(auto& m : macros)
                    {
                        if(boost::algorithm::contains(window_title, m->name) && m->name.length() > 2)
                        {
                            const auto it = m->key_vec.find(pressed_keys);
                            if(it != m->key_vec.end())
                            {
                                for(const auto& i : it->second)
                                {
                                    i->DoWrite();
                                }
                            }
                            return; /* Exit from loop */
                        }
                    }
                }
            }
            else
            {
                const auto it = macros[0]->key_vec.find(pressed_keys);
                if(it != macros[0]->key_vec.end())
                {
                    for(const auto& i : it->second)
                    {
                        i->DoWrite();
                    }
                }
            }
        }
    }
}

void CustomMacro::Init(void)
{
    std::thread(&CustomMacro::UartReceiveThread, this).detach();
}

void CustomMacro::UartReceiveThread(void)
{
    try
    {
        CallbackAsyncSerial serial("\\\\.\\COM" + std::to_string(com_port), 921600); /* baud rate has no meaning here */
        auto fp = std::bind(&CustomMacro::UartDataReceived, this, std::placeholders::_1, std::placeholders::_2);
        serial.setCallback(fp);

        for (;;)
        {
            if (serial.errorStatus() || serial.isOpen() == false)
            {
                LOGMSG(error, "Serial port unexpectedly closed");
                break;
            }
            std::this_thread::sleep_for(1ms);
        }
        serial.close();
    }
    catch (std::exception& e)
    {
        LOGMSG(error, "Exception {}", e.what());
    }
}