#ifndef SESSION_TRACKER_HPP
#define SESSION_TRACKER_HPP

#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "../nlohmann/json.hpp"

class SessionTracker {
public:
    static SessionTracker& getInstance();

    void startSession();
    void endSession();

    void logUserRequest(const std::string& query);
    void logAgentResponse(const std::string& response);
    void logUserCommand(const std::string& command);
    void logCommandResult(int exit_status, const std::string& output);

private:
    SessionTracker();
    ~SessionTracker();

    std::string generateSessionId();

    nlohmann::json sessionData;
    std::ofstream logFile;

    bool lastInteractionOpen;
};
#endif //SESSION_TRACKER_HPP
