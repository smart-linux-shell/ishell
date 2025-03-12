#ifndef SESSION_TRACKER_HPP
#define SESSION_TRACKER_HPP

#include <fstream>
#include <string>
#include <../nlohmann/json.hpp>

class SessionTracker {
public:
    static SessionTracker& get();

    void startSession();
    void endSession();

    void logEvent(const std::string& event_type, const std::string& data);

private:
    SessionTracker();
    ~SessionTracker();

    SessionTracker(const SessionTracker&) = delete;
    SessionTracker& operator=(const SessionTracker&) = delete;

    std::string generateSessionId();
    void openLogFile();

    nlohmann::json sessionData;
    std::ofstream logFile;

    std::string sessionId;
    std::string username;
};

#endif
