#ifndef SESSION_TRACKER_HPP
#define SESSION_TRACKER_HPP

#include <string>
#include <vector>
#include <sqlite3.h>

class SessionTracker {
public:
    enum class EventType {
        ShellCommand,
        SystemCommand,
        Unknown
    };

    struct ShellCmd {
        int interaction_id;
        std::string command;
        std::string output;
        std::string execution_start;
        std::string execution_end;
        int exit_code;
    };

    struct Interaction {
        int interaction_id;
        std::string timestamp;
        std::string question;
        std::string answer;
        std::vector<ShellCmd> shell;
    };

    static SessionTracker& get();

    void startSession();
    void endSession();
    
    void logAgentInteraction(const std::string& question, const std::string& response);

    void addNewCommand(EventType command_type);
    void appendCommandText(const std::string& text);
    void setCommandOutput(const std::string& output);
    void setExitCode(int exit_code);

    const std::vector<Interaction>& get_history() const;

private:
    SessionTracker();
    ~SessionTracker();

    SessionTracker(const SessionTracker&) = delete;
    SessionTracker& operator=(const SessionTracker&) = delete;

    void openLogFile();

    sqlite3 *db;
    int sessionDbId;
    int lastCommandId;

    mutable std::vector<Interaction> history;
};

#endif // SESSION_TRACKER_HPP
