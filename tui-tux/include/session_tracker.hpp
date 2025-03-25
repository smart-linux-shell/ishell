#ifndef SESSION_TRACKER_HPP
#define SESSION_TRACKER_HPP

#include <string>
#include <sqlite3.h>

class SessionTracker {
public:
    enum class EventType {
        AgentResponse,
        UserQuestion,
        ShellCommand,
        SystemCommand,
        SystemMessage,
        Unknown
    };

    static SessionTracker& get();

    void startSession();
    void endSession();

    void logEvent(EventType event_type, const std::string& data);
    void logAgentInteraction(const std::string& question, const std::string& response);
    void finalizeCommand(int exit_code, const std::string& output);

private:
    SessionTracker();
    ~SessionTracker();

    SessionTracker(const SessionTracker&) = delete;
    SessionTracker& operator=(const SessionTracker&) = delete;

    void openLogFile();

    sqlite3 *db;
    int sessionDbId;
    int lastInteractionId;
    int lastCommandId;
};

#endif // SESSION_TRACKER_HPP
