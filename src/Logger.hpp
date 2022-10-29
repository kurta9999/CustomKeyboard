#pragma once

#include "utils/CSingleton.hpp"

#include <stdarg.h>

#include <wx/wx.h>
#include "gui/MainFrame.hpp"
#include "CustomKeyboard.hpp"
#include <ctime>
#include <fstream>
#include <filesystem>

#include "ILogHelper.hpp"

#ifndef _WIN32
#include <fmt/format.h>
#include <fmt/chrono.h>
template<class> inline constexpr bool always_false_v = false;
#else

#endif

DECLARE_APP(MyApp);

#ifdef _DEBUG /* this is only for debugging, it remains oldschool */
#define DBG(str, ...) \
    {\
        char __debug_format_str[64]; \
        snprintf(__debug_format_str, sizeof(__debug_format_str), str, __VA_ARGS__); \
        OutputDebugStringA(__debug_format_str); \
    }

#define DBGW(str, ...) \
    {\
        wchar_t __debug_format_str[128]; \
        wsprintfW(__debug_format_str, str, __VA_ARGS__); \
        OutputDebugStringW(__debug_format_str); \
    }
#else
#define DBG(str, ...)
#define DBGW(str, ...)
#endif

enum LogLevel
{
    Verbose,
    Normal,
    Notification,
    Warning,
    Error,
    Critical
};

#define WIDEN(quote) WIDEN2(quote)
#define WIDEN2(quote) L##quote

#define LOG(level, message, ...) \
    do {\
    {Logger::Get()->Log(level, __FILE__, __LINE__, __FUNCTION__, message, ##__VA_ARGS__);} \
} while(0)

#ifdef _WIN32
#define LOGW(level, message, ...) \
    do {\
    {Logger::Get()->Log(level, WIDEN(__FILE__), __LINE__, WIDEN(__FUNCTION__), message, ##__VA_ARGS__);} \
} while(0)
#else
#define LOGW(str, ...)
#endif

class Logger : public CSingleton < Logger >
{
    friend class CSingleton < Logger >;
public:
    Logger();
    ~Logger() = default;
    void SetLogHelper(ILogHelper* helper);
    bool SearchInLogFile(std::string_view filter, std::string_view log_level);

#ifdef _WIN32  /* std::format version with both std::string & std::wstring support - GCC's std::format and std::chrono::current_zone implementation is still missing - 2022.10.28 */
#define LOG_GUI_FORMAT "{:%Y.%m.%d %H:%M:%S} [{}] {}"
#define LOG_FILE_FORMAT "{:%Y.%m.%d %H:%M:%S} [{}] [{}:{} - {}] {}\n"

    template <class T>
    struct HelperTraits
    {
        static_assert(always_false_v<T>, "Invalid type. Only std::string and std::wstring are accepted!");
    };

    template <>
    struct HelperTraits<std::string>
    {
        static constexpr std::string_view serverities[] = { "Verbose", "Normal", "Notification", "Warning", "Error", "Critical" };
        static constexpr char slash = '/';
        static constexpr char backslash = '\\';
        static constexpr std::string_view gui_str = LOG_GUI_FORMAT;
        static constexpr std::string_view log_str = LOG_FILE_FORMAT;
    };

    template <>
    struct HelperTraits<std::wstring>
    {
        static constexpr std::wstring_view serverities[] = { L"Verbose", L"Normal", L"Notification", L"Warning", L"Error", L"Critical" };
        static constexpr wchar_t slash = L'/';
        static constexpr wchar_t backslash = L'\\';
        static constexpr std::wstring_view gui_str = WIDEN(LOG_GUI_FORMAT);
        static constexpr std::wstring_view log_str = WIDEN(LOG_FILE_FORMAT);
    };

    template<typename T>
    size_t strlen_helper(T input)
    {
        using X = std::decay_t<decltype(input)>;
        if constexpr(std::is_same_v<X, const char*>)
        {
            return strlen(input);
        }
        else if constexpr(std::is_same_v<X, const wchar_t*>)
        {
            return std::wstring(input).length();
        }
        else
        {
            static_assert(always_false_v<X>, "Invalid type. Only const char* and const wchar_t* are accepted!");
        }
    }

    template <typename T> struct get_fmt_mkarg_type;
    template <> struct get_fmt_mkarg_type<const wchar_t*> { using type = std::wformat_context; };
    template <> struct get_fmt_mkarg_type<const wchar_t> { using type = std::wformat_context; };
    template <> struct get_fmt_mkarg_type<wchar_t> { using type = std::wformat_context; };
    template <> struct get_fmt_mkarg_type<const char*> { using type = std::format_context; };
    template <> struct get_fmt_mkarg_type<const char> { using type = std::format_context; };
    template <> struct get_fmt_mkarg_type<char> { using type = std::format_context; };

    template <typename T> struct get_fmt_ret_string_type;
    template <> struct get_fmt_ret_string_type<const wchar_t*> { using type = std::wstring; };
    template <> struct get_fmt_ret_string_type<const wchar_t> { using type = std::wstring; };
    template <> struct get_fmt_ret_string_type<wchar_t> { using type = std::wstring; };
    template <> struct get_fmt_ret_string_type<const char*> { using type = std::string; };
    template <> struct get_fmt_ret_string_type<const char> { using type = std::string; };
    template <> struct get_fmt_ret_string_type<char> { using type = std::string; };

    template<typename F = const char*, typename G = const char*, class T, typename... Args>
    void LogInternal(LogLevel lvl, F file, long line, G function, std::basic_string_view<T> msg, Args &&...args)
    {
        using string_type = get_fmt_ret_string_type<T>::type;
        typename get_fmt_ret_string_type<T>::type formatted_msg = (sizeof...(args) != 0) ? std::vformat(msg, std::make_format_args<typename get_fmt_mkarg_type<T>::type>(args...)) : msg.data();
        const auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        typename get_fmt_ret_string_type<T>::type str = std::vformat(HelperTraits<string_type>::gui_str, std::make_format_args<typename get_fmt_mkarg_type<T>::type>(now, HelperTraits<string_type>::serverities[lvl], formatted_msg));

        F filename = file;  /* get filename from file path - __FILE__ macro gives abosulte path for filename */
        for(int i = strlen_helper(file); i > 0; i--)
        {
            if(file[i] == HelperTraits<string_type>::slash || file[i] == HelperTraits<string_type>::backslash)
            {
                filename = &file[i + 1];
                break;
            }
        }

        uint8_t retry_count = 0;
        while(++retry_count < 5)
        {
            DBG("\n\n");
            //DBG("mutex try lock pre %s\n", function);
            bool ret = m_mutex.try_lock_for(std::chrono::milliseconds(10));
            DBG("mutex try lock post\n");
            if(ret)
            {
                if(lvl >= LogLevel::Verbose && lvl <= LogLevel::Critical)
                {
#ifdef _WIN32
                    const auto now_truncated_to_ms = std::chrono::floor<std::chrono::milliseconds>(now);
                    fLog << std::format(HelperTraits<string_type>::log_str, now_truncated_to_ms, HelperTraits<string_type>::serverities[lvl], filename, line, function, formatted_msg);
#else
                    fLog << fmt::format(HelperTraits<string_type>::log_str, now_truncated_to_ms, HelperTraits<string_type>::serverities[lvl], filename, line, function, formatted_msg);
#endif
                    fLog.flush();
                    DBG("write file\n");
                }
                 
                MyFrame* frame = ((MyFrame*)(wxGetApp().GetTopWindow()));
                if(wxGetApp().is_init_finished && frame && frame->log_panel && frame->log_panel->m_Log && m_helper)
                {
                    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

                    m_helper->AppendLog(str, true);
                    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
                    int64_t dif = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
                    DBG("append %.3f\n", (float)dif / 1000000.0);
                }
                else
                    preinit_entries.push_back(wxString(str));
            }
            else
            {
                std::this_thread::yield();
                DBG("mutex lock fail\n");
            }

            if(ret)
            {
                m_mutex.unlock();
                DBG("mutex unlock, OK\n");
                break;
            }
        }

        DBG("mutex try lock release\n");
    }

    template<typename F = const char*, typename G = const char*, typename... Args>
    void LogM(LogLevel lvl, F file, long line, G function, std::string_view msg, Args &&...args)
    {
        LogInternal(lvl, file, line, function, msg, std::forward<Args>(args)...);
    }

    template<typename F = const char*, typename G = const char*, typename... Args>
    void LogW(LogLevel lvl, F file, long line, G function, std::wstring_view msg, Args &&...args)
    {
        LogInternal(lvl, file, line, function, msg, std::forward<Args>(args)...);
    }

    // !\brief Write given log message to logfile.txt & LogPanel
    // !\param lvl [in] Serverity level
    // !\param file [in] Filename where the call comes from
    // !\param line [in] Line in source file where the call comes from
    // !\param function [in] Function name in source file where the call comes from
    // !\param msg [in] Message to log
    // !\param args [in] va_args arguments for std::format
    template<typename F = const char*, typename G = const char*, class T, typename... Args>
    void Log(LogLevel lvl, F file, long line, G function, T msg, Args &&...args)
    {
        using X = std::decay_t<decltype(msg)>;
        if constexpr(std::is_same_v<X, const char*>)
        {
            LogM(lvl, file, line, function, msg, std::forward<Args>(args)...);
        }
        else if constexpr(std::is_same_v<X, const wchar_t*>)
        {
            LogW(lvl, file, line, function, msg, std::forward<Args>(args)...);
        }
        else
        {
            static_assert(always_false_v<X>, "Invalid type. Only const char* and const wchar_t* are accepted!");
        }
    }
#else
template<typename... Args>
void Log(LogLevel lvl, const char* file, long line, const char* function, const char* msg, Args &&...args)
{
#if 0
    std::string str;
    std::string formatted_msg = (sizeof...(args) != 0) ? fmt::format(msg, std::forward<Args>(args)...) : msg;
    time_t current_time;
    tm* current_tm;
    time(&current_time);
    current_tm = localtime(&current_time);
    str = fmt::format("{:%Y.%m.%d %H:%M:%S} [{}] {}", *current_tm, severity_str[lvl], formatted_msg);
#if DEBUG
    OutputDebugStringA(str.c_str());
    OutputDebugStringA("\n");
#endif
    const char* filename = file;  /* get filename from file path - __FILE__ macro gives abosulte path for filename */
    for(int i = strlen(file); i > 0; i--)
    {
        if(file[i] == '/' || file[i] == '\\')
        {
            filename = &file[i + 1];
            break;
        }
    }
    if(lvl > LogLevel::Normal && lvl <= LogLevel::Critical)
    {
        fLog << fmt::format("{:%Y.%m.%d %H:%M:%S} [{}] [{}:{} - {}] {}\n", *current_tm, severity_str[lvl], filename, line, function, formatted_msg);
        fLog.flush();
    }
    MyFrame* frame = ((MyFrame*)(wxGetApp().GetTopWindow()));
    if(wxGetApp().is_init_finished && frame && frame->log_panel && frame->log_panel->m_Log)
    {
        frame->log_panel->m_Log->Append(wxString(str));
        frame->log_panel->m_Log->ScrollLines(frame->log_panel->m_Log->GetCount());
    }
    else
        preinit_entries.Add(str);
#endif
}

#endif

    // !\brief Append log messages to log panel which were logged before log panel was constructor
    void AppendPreinitedEntries()
    {
        MyFrame* frame = ((MyFrame*)(wxGetApp().GetTopWindow()));
        if(frame && frame->log_panel && m_helper)
        {
            bool ret = m_mutex.try_lock_for(std::chrono::milliseconds(1000));
            if(ret)
            {
                for(auto& i : preinit_entries)
                {
                    if(!i.empty())
                        m_helper->AppendLog(i.ToStdString());
                }
                preinit_entries.clear();
                m_mutex.unlock();
            }
        }
    }

private:
    // !\brief File handle for log 
    std::ofstream fLog;

    // !\brief Preinited log messages
    std::vector<wxString> preinit_entries;

    // !\brief Pointer to LogPanel
    ILogHelper* m_helper = nullptr;

    // !\brief Logger's mutex
    std::timed_mutex m_mutex;
};