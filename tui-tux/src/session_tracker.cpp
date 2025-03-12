#include "session_tracker.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

SessionTracker& SessionTracker::get() {
    static SessionTracker instance;
    return instance;
}

SessionTracker::SessionTracker() {
    sessionId = generateSessionId();

    struct passwd *pw = getpwuid(getuid());
    username = pw ? pw->pw_name : "unknown";

    openLogFile();
}

SessionTracker::~SessionTracker() {
    endSession();
}

std::string SessionTracker::generateSessionId() {
    std::ostringstream oss;
    auto t = std::time(nullptr);
    oss << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S");
    return oss.str();
}

void SessionTracker::openLogFile() {
    std::string logDir = "./local";
    mkdir(logDir.c_str(), 0755);

    std::string filename = logDir + "/session_" + username + "_" + sessionId + ".jsonl";
    logFile.open(filename, std::ios::out | std::ios::app);
}

void SessionTracker::startSession() {

    nlohmann::json event = {
        {"timestamp", sessionId},
        {"user", username},
        {"event", "session_start"}
    };
    logFile << event.dump() << std::endl;
}

void SessionTracker::logEvent(const std::string& event_type, const std::string& data) {

    auto t = std::time(nullptr);
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");

    nlohmann::json event = {
        {"timestamp", ts.str()},
        {"user", username},
        {"event", event_type},
        {"data", data}
    };
    logFile << event.dump() << std::endl;
}

void SessionTracker::endSession() {

    auto t = std::time(nullptr);
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");

    nlohmann::json event = {
        {"timestamp", ts.str()},
        {"user", username},
        {"event", "session_end"}
    };
    logFile << event.dump() << std::endl;

    logFile.close();
}