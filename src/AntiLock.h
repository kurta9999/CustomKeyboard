#pragma once

#include "utils/CSingleton.h"

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
private:
    // \brief Check if current session active (user is logged in)
    // \return Is session active?
    bool IsSessionActive();

    // \brief Simulate user activity
    void SimulateUserActivity();

    // \brief Mouse step
    bool step_forward = false;
};