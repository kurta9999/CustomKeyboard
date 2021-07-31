#include "pch.h"

inline std::string extract_string(std::string& str, size_t start, size_t start_end, size_t len)
{
    return str.substr(start + len, start_end - start - len);
}

void Settings::ParseMacroKeys(size_t id, const std::string& key_code, std::string& str, std::unique_ptr<MacroContainer>& c)
{
    enum ConfigTypes : uint8_t
    {
        BIND_NAME, KEY_SEQ, KEY_TYPE, DELAY, MOUSE_MOVE, MOUSE_CLICK, MAX
    };

    constexpr size_t MAX_ITEMS = MAX;
    constexpr const char* start_str_arr[MAX_ITEMS] = { "BIND_NAME[", "KEY_SEQ[", "KEY_TYPE[", "DELAY[", "MOUSE_MOVE[", "MOUSE_CLICK[" };
    constexpr const char start_str_arr_lens[MAX_ITEMS] = { std::char_traits<char>::length(start_str_arr[0]),
        std::char_traits<char>::length(start_str_arr[1]), std::char_traits<char>::length(start_str_arr[2]), std::char_traits<char>::length(start_str_arr[3]), 
        std::char_traits<char>::length(start_str_arr[4]), std::char_traits<char>::length(start_str_arr[5]) };

    constexpr const char* seq_separator = "+";

    size_t pos = 1;
    while(pos < str.length() - 1)
    {
        size_t first_end = str.find("]", pos + 1);
        size_t first_pos[MAX_ITEMS];
        for(int i = 0; i != MAX_ITEMS; ++i)
        {
            first_pos[i] = str.substr(0, first_end).find(start_str_arr[i], pos - 1);
        }

        uint8_t input_type = 0xFF;
        uint8_t not_empty_cnt = 0;
        for(int i = 0; i != MAX_ITEMS; ++i)
        {
            if(first_pos[i] != std::string::npos)
            {
                input_type = i;
                not_empty_cnt++;
            }
        }

        if(not_empty_cnt > 1 || input_type == 0xFF)
        {
            LOGMSG(error, "Error with config file macro formatting");
            return;
        }

        switch(input_type)
        {
            case ConfigTypes::BIND_NAME:
            {
                pos = first_end;
                
                c->bind_name[key_code] = extract_string(str, first_pos[BIND_NAME], first_end, start_str_arr_lens[BIND_NAME]);
                DBG("Bind name: %s\n", c->bind_name[key_code].c_str());
                break;
            }
            case ConfigTypes::KEY_SEQ:
            {
                pos = first_end;
                std::vector<uint16_t> keys;
                std::string sequence = extract_string(str, first_pos[KEY_SEQ], first_end, start_str_arr_lens[KEY_SEQ]);

                boost::char_separator<char> sep(seq_separator);
                boost::tokenizer< boost::char_separator<char> > tok(sequence, sep);
                for(boost::tokenizer< boost::char_separator<char> >::iterator beg = tok.begin(); beg != tok.end(); ++beg)
                {
                    DBG("Token: %s\n", beg->c_str());
                    std::string key_code = *beg;
                    uint16_t key = CustomMacro::Get()->GetKeyScanCode(key_code);
                    if(key == 0xFFFF)
                    {
                        LOGMSG(error, "Invalid key found in settings.ini: {}", key_code);
                    }
                    keys.push_back(key);
                }

                c->key_vec[key_code].push_back(std::make_unique<KeyCombination>(std::move(keys)));
                break;
            }
            case ConfigTypes::KEY_TYPE:
            {
                pos = first_end;
                std::string sequence = extract_string(str, first_pos[KEY_TYPE], first_end, start_str_arr_lens[KEY_TYPE]);

                DBG("Text Token: %s\n", sequence.c_str());
                c->key_vec[key_code].push_back(std::make_unique<KeyText>(std::move(sequence)));
                break;
            }
            case ConfigTypes::DELAY:
            {
                pos = first_end;
                std::string sequence = extract_string(str, first_pos[DELAY], first_end, start_str_arr_lens[DELAY]);

                size_t separator_pos = sequence.find("-");
                if(separator_pos != std::string::npos)
                {
                    uint32_t delay_start = static_cast<uint32_t>(std::stoi(sequence));
                    uint32_t delay_end = static_cast<uint32_t>(std::stoi(&sequence[separator_pos + 1]));
                    DBG("Random Delay range: %d, %d\n", delay_start, delay_end);

                    c->key_vec[key_code].push_back(std::make_unique<KeyDelay>(delay_start, delay_end));
                }
                else
                {
                    uint32_t delay = static_cast<uint32_t>(std::stoi(sequence));
                    DBG("Delay: %d\n", delay);
                    c->key_vec[key_code].push_back(std::make_unique<KeyDelay>(delay));
                }
                break;
            }
            case ConfigTypes::MOUSE_MOVE:
            {
                pos = first_end;
                std::string sequence = extract_string(str, first_pos[MOUSE_MOVE], first_end, start_str_arr_lens[MOUSE_MOVE]);

                size_t separator_pos = sequence.find(",");
                if(separator_pos != std::string::npos)
                {
                    POINT pos;
                    pos.x = static_cast<long>(std::stoi(sequence));
                    pos.y = static_cast<uint32_t>(std::stoi(&sequence[separator_pos + 1]));
                    DBG("Mouse Movement: %d, %d\n", pos.x, pos.y);

                    c->key_vec[key_code].push_back(std::make_unique<MouseMovement>((LPPOINT*)&pos));
                }
                break;
            }
            case ConfigTypes::MOUSE_CLICK:
            {
                pos = first_end;
                std::string sequence = extract_string(str, first_pos[MOUSE_CLICK], first_end, start_str_arr_lens[MOUSE_CLICK]);

                uint16_t mouse_button = 0xFFFF;
                if(sequence == "L" || sequence == "LEFT")
                    mouse_button = MOUSEEVENTF_LEFTDOWN;
                if(sequence == "R" || sequence == "RIGHT")
                    mouse_button = MOUSEEVENTF_RIGHTDOWN;
                if(sequence == "M" || sequence == "MIDDLE")
                    mouse_button = MOUSEEVENTF_MIDDLEDOWN;
                if(mouse_button != 0xFFFF)
                    c->key_vec[key_code].push_back(std::make_unique<MouseClick>(mouse_button));
                else
                    LOGMSG(error, "Invalid mouse button name format!");
                break;
            }
            default:
            {
                LOGMSG(error, "Invalid sequence/text format in line: {}", str.c_str());
                break;
            }
        }
    }
}

void Settings::LoadFile()
{
    if(!std::filesystem::exists("settings.ini"))
    {
        SaveFile(true);
    }

    boost::property_tree::ptree pt;
    try
    {
        boost::property_tree::ini_parser::read_ini("settings.ini", pt);
    }
    catch(boost::property_tree::ptree_error& e)
    {
        LOGMSG(error, "exception: {}", e.what());
    }

    try
    {
        CustomMacro::Get()->use_per_app_macro = static_cast<bool>(std::stoi(pt.get_child("Macro_Config").find("UsePerApplicationMacros")->second.data())) != 0;
        CustomMacro::Get()->advanced_key_binding = static_cast<bool>(std::stoi(pt.get_child("Macro_Config").find("UseAdvancedKeyBinding")->second.data())) != 0;

        CustomMacro::Get()->macros.clear();
        std::unique_ptr<MacroContainer> p = std::make_unique<MacroContainer>();
        macro_section.clear();
        auto& global_child = pt.get_child("Keys_Global");
        for(auto& key : global_child)
        {
            std::string& str = key.second.data();
            ParseMacroKeys(0, key.first, str, p);
        }
        p->name = "Global";
        CustomMacro::Get()->macros.push_back(std::move(p));

        /* load per-application macros */
        size_t counter = 1;
        size_t cnt = 0;
        while((cnt = pt.count("Keys_Macro" + std::to_string(counter))) == 1)
        {
            std::unique_ptr<MacroContainer> p2 = std::make_unique<MacroContainer>();
            auto& ch = pt.get_child("Keys_Macro" + std::to_string(counter));
            for(auto& key : ch)
            {
                if(key.first.data() == std::string("AppName"))
                {
                    p2->name = key.second.data();
                    continue;
                }
                std::string& str = key.second.data();
                ParseMacroKeys(counter, key.first, str, p2);
            }
            counter++;
            CustomMacro::Get()->macros.push_back(std::move(p2));
        }
        CustomMacro::Get()->is_enabled = std::stoi(pt.get_child("COM_Backend").find("Enable")->second.data());
        CustomMacro::Get()->com_port = std::stoi(pt.get_child("COM_Backend").find("COM")->second.data());
        Server::Get()->is_enabled = static_cast<uint16_t>(std::stoi(pt.get_child("TCP_Backend").find("Enable")->second.data()));
        Server::Get()->tcp_port = static_cast<uint16_t>(std::stoi(pt.get_child("TCP_Backend").find("TCP_Port")->second.data()));
        minimize_on_exit = static_cast<bool>(std::stoi(pt.get_child("App").find("MinimizeOnExit")->second.data()) != 0);
        minimize_on_startup = static_cast<bool>(std::stoi(pt.get_child("App").find("MinimizeOnStartup")->second.data()) != 0);
        default_page = static_cast<uint8_t>(std::stoi(pt.get_child("App").find("DefaultPage")->second.data()));
        PrintScreenSaver::Get()->screenshot_key = pt.get_child("Screenshot").find("ScreenshotKey")->second.data();
        PrintScreenSaver::Get()->timestamp_format = pt.get_child("Screenshot").find("ScreenshotDateFormat")->second.data();
        PrintScreenSaver::Get()->screenshot_path = pt.get_child("Screenshot").find("ScreenshotPath")->second.data();
        PathSeparator::Get()->replace_key = pt.get_child("PathSeparator").find("ReplacePathSeparatorKey")->second.data();

        if(!std::filesystem::exists(PrintScreenSaver::Get()->screenshot_path))
            std::filesystem::create_directory(PrintScreenSaver::Get()->screenshot_path);

        DirectoryBackup::Get()->backup_time_format = pt.get_child("BackupSettings").find("BackupFileFormat")->second.data();
        DirectoryBackup::Get()->backup_key = pt.get_child("BackupSettings").find("BackupKey")->second.data();

        /* load backup configs */
        DirectoryBackup::Get()->backups.clear();
        size_t counter_ = 1;
        size_t cnt_ = 0;
        while((cnt_ = pt.count("Backup_" + std::to_string(counter_))) == 1)
        {
            std::string key = "Backup_" + std::to_string(counter_);

            std::filesystem::path from = pt.get_child(key).find("From")->second.data();

            std::vector<std::filesystem::path> to;
            boost::split(to, pt.get_child(key).find("To")->second.data(), boost::is_any_of("|"));

            std::vector<std::string> ignore_list;
            boost::split(ignore_list, pt.get_child(key).find("Ignore")->second.data(), boost::is_any_of("|"));
            int max_backups = std::stoi(pt.get_child(key).find("MaxBackups")->second.data());
            BackupEntry* b = new BackupEntry(std::move(from), std::move(to), std::move(ignore_list), max_backups);

            counter_++;
            DirectoryBackup::Get()->backups.push_back(b);
        }

        if(default_page > 5)
            default_page = 5;

        if(std::stoi(pt.get_child("Miner").find("Enable")->second.data()) != 0)
        {
            MinerWatchdog::Get()->miner_dir = std::move(pt.get_child("Miner").find("MinerDirectory")->second.data());
            MinerWatchdog::Get()->miner_params = std::move(pt.get_child("Miner").find("MinerParameters")->second.data());
            if(!pt.get_child("Miner").find("PreStartupMacro")->second.data().empty())
            {
                std::unique_ptr<MacroContainer> p2 = std::make_unique<MacroContainer>();
                std::string& str = pt.get_child("Miner").find("PreStartupMacro")->second.data();
                ParseMacroKeys(counter, "AFTERBURNER", str, p2);
            }
        }
        else
        {
            MinerWatchdog::Get()->miner_dir.clear();
            MinerWatchdog::Get()->miner_params.clear();
        }

        uint32_t val1 = static_cast<uint32_t>(std::stoi(pt.get_child("Graph").find("Graph1HoursBack")->second.data()));
        Database::Get()->SetGraphHours(0, val1);
        uint32_t val2 = static_cast<uint32_t>(std::stoi(pt.get_child("Graph").find("Graph2HoursBack")->second.data()));
        Database::Get()->SetGraphHours(1, val2);
    }
    catch(boost::property_tree::ptree_error& e)
    {
        LOGMSG(error, "exception: {}", e.what());
    }
}

void Settings::SaveFile(bool write_default_macros) /* tried boost::ptree ini writer but it doesn't support comments... sticking to plain file functions */
{
    std::ofstream out("settings.ini", std::ofstream::binary);
    out << "# Possible macro keywords: \n";
    out << "# KEY_TYPE[text] = Press & release given keys in sequence to type a text\n";
    out << "# KEY_SEQ[CTRL+C] = Press all given keys after each other and release it when each was pressed - ideal for key shortcats\n";
    out << "# DELAY[time in ms] = Waits for given milliseconds\n";
    out << "# DELAY[min ms - max ms] = Waits randomly between min ms and max ms\n";
    out << "\n";
    out << "[Macro_Config]\n";
    out << "# Use per-application macros. AppName is searched in active window title, so window name must contain AppName\n";
    out << "UsePerApplicationMacros = " << CustomMacro::Get()->use_per_app_macro << "\n";
    out << "\n";
    out << "# If enabled, you can bind multiple key combinations with special keys like RSHIFT + 1, but can't bind SHIFT, CTRL and other special keys alone\n";
    out << "UseAdvancedKeyBinding = " << CustomMacro::Get()->advanced_key_binding << "\n";
    out << "\n";

    if(!write_default_macros)  /* True if settings.ini file doesn't exists - write a few macro lines here as example */
    {
        int cnt = 0;
        std::string key;
        auto& m = CustomMacro::Get()->GetMacros();
        for(auto& i : m)
        {
            if(!cnt)
                out << "[Keys_Global]\n";
            else
            {
                out << fmt::format("\n[Keys_Macro{}]\n", cnt);
                out << fmt::format("AppName = {}\n", i->name);
            }
            cnt++;
            for(auto& x : i->key_vec)
            {
                key = fmt::format("{} = BIND_NAME[{}]", x.first, i->bind_name[x.first]);

                /* TODO: replace this with function pointers & lambdas and move it to it's class - CustomMacro */
                for(auto& k : x.second)
                {
                    KeyClass* p = k.get();
                    CustomMacro::Get()->GenerateReadableTextFromMap(k, true,
                        [this, &key](std::string& macro_str, std::string* macro_typename) mutable  -> void
                        {
                            key += macro_str;
                        });
                }
                out << key << '\n';
                key.clear();
            }
        }
    }
    else
    {
        out << "[Keys_Global]\n";
        out << "NUM_0 = KEY_SEQ[A+B+C]\n";
        out << "NUM_1 = KEY_TYPE[global macro 1]\n";
        out << "\n";
        out << "[Keys_Macro1]\n";
        out << "AppName = Notepad\n";
        out << "NUM_1 = KEY_TYPE[test string from CustomKeyboard.exe] DELAY[100] KEY_TYPE[Closing window...] DELAY[100 - 3000] KEY_SEQ[ALT+F4]\n";
    }

    out << "\n";
    out << "[TCP_Backend]\n";
    out << "Enable = " << Server::Get()->is_enabled << "\n";
    out << "TCP_Port = " << Server::Get()->tcp_port << " # TCP Port for receiving measurements from sensors\n";
    out << "\n";
    out << "[COM_Backend]\n";
    out << "Enable = " << CustomMacro::Get()->is_enabled << "\n";
    out << "COM = " << CustomMacro::Get()->com_port << " # Com port for UART where data received from STM32\n";
    out << "\n";
    out << "[App]\n";
    out << "MinimizeOnExit = " << minimize_on_exit << "\n";
    out << "MinimizeOnStartup = " << minimize_on_startup<< "\n";
    out << "DefaultPage = " << static_cast<uint16_t>(default_page) << "\n";
    out << "\n";
    out << "[Screenshot]\n";
    out << "ScreenshotKey = " << PrintScreenSaver::Get()->screenshot_key << "\n";
    out << "ScreenshotDateFormat = " << PrintScreenSaver::Get()->timestamp_format << "\n";
    out << "ScreenshotPath = " << PrintScreenSaver::Get()->screenshot_path << "\n";
    out << "\n";
    out << "[PathSeparator]\n";
    out << "ReplacePathSeparatorKey = " << PathSeparator::Get()->replace_key << "\n";
    out << "\n";
    out << "[BackupSettings]\n";
    out << "BackupKey = " << DirectoryBackup::Get()->backup_key << "\n";
    out << "BackupFileFormat = " << DirectoryBackup::Get()->backup_time_format << "\n";
    if(!write_default_macros)
    {
        int cnt = 0;
        std::string key;
        for(auto& i : DirectoryBackup::Get()->backups)
        {
            out << fmt::format("\n[Backup_{}]\n", cnt++);
            out << "From = " << i->from << '\n';
            key.clear();
            for(auto& x : i->to)
            {
                key += x.generic_u8string() + '|';
            }
            if(key[key.length() - 1] == '|')
                key.erase(key.length() - 1, key.length());
            out << "To = " << key << '\n';
            key.clear();
            for(auto& x : i->ignore_list)
            {
                key += x + '|';
            }
            if(key[key.length() - 1] == '|')
                key.erase(key.length() - 1, key.length());
            out << "Ignore = " << key << '\n';
            out << "MaxBackups = " << i->max_backups << '\n';
        }
    }
    else
    {
        out << "\n[Backup_1]\n";
        out << "From = C:\\Users\\Ati\\Desktop\\folder_from_backup\n";
        out << "To = C:\\Users\\Ati\\Desktop\\folder_where_to_backup|F:\\Backup\\folder_where_to_backup\n";
        out << "Ignore = git/COMMIT_EDITMSG|.git|.vs|Debug|Release|Screenshots|x64|Graphs/Line Chart|Graphs/Temperature.html|Graphs/Humidity.html|Graphs/CO2.html|Graphs/Lux.html|Graphs/VOC.html|Graphs/CCT.html|Graphs/PM10.html|Graphs/PM25.html\n";
        out << "MaxBackups = 5\n";
    }
    out << "\n";
    out << "[Graph]\n";
    out << "Graph1HoursBack = " << Database::Get()->GetGraphHours(0) << " # One day\n";
    out << "Graph2HoursBack = " << Database::Get()->GetGraphHours(1) << " # One week\n";
    out.close();
}

void Settings::Init()
{
    LoadFile();
}