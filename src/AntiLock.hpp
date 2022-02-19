#pragma once

#include "utils/CSingleton.hpp"

class AntiLock : public CSingleton < AntiLock >
{
    friend class CSingleton < AntiLock >;

public:
    AntiLock() = default;
    ~AntiLock() = default;

    // \brief Process for antilock
    void Process();

    // \brief Is enabled?
    bool is_enabled = false;

    // \brief Antilock timeout [s]
    uint32_t timeout = 300;

    // \brief Is screen saver enabled
    bool is_screensaver = false;
private:
    // \brief Check if current session active (user is logged in)
    // \return Is session active?
    bool IsSessionActive();

    // \brief Simulate user activity
    void SimulateUserActivity();    
    
    // \brief Simulate user activity
    void StartScreenSaver();

    // \brief Mouse step
    bool step_forward = false;
};