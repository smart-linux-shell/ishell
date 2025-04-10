#ifndef SESSION_TRACKER_HPP
#define SESSION_TRACKER_HPP

#include <string>
#include <sqlite3.h>

class SessionTracker {
public:
    enum class EventType {
        ShellCommand,
        SystemCommand,
        Unknown
    };

    static SessionTracker& get();

    void startSession();
    void endSession();
    
    void logAgentInteraction(const std::string& question, const std::string& response);

    void addNewCommand(EventType command_type);
    void appendCommandText(const std::string& text);
    void setCommandOutput(const std::string& output);
    void setExitCode(int exit_code);

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
    std::string currentCommandText;
    std::string currentCommandOutput;
};

#endif // SESSION_TRACKER_HPP
