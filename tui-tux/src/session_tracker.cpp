#include "session_tracker.hpp"
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

SessionTracker& SessionTracker::get() {
    static SessionTracker instance;
    return instance;
}

SessionTracker::SessionTracker()
    : db(nullptr), sessionDbId(-1), lastCommandId(-1) {
    openLogFile();
}

SessionTracker::~SessionTracker() {
    endSession();
}

void SessionTracker::startSession() {
    if (!db) return;

    const char *insertSession = "INSERT INTO sessions DEFAULT VALUES;";
    if (sqlite3_exec(db, insertSession, nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to insert session: " << sqlite3_errmsg(db) << std::endl;
        sessionDbId = -1;
    } else {
        sessionDbId = sqlite3_last_insert_rowid(db);
    }

    lastCommandId = -1;

    this->logAgentInteraction("Hello!", "Hello! How can I assist you with your Linux shell commands today? How can I assist you today?");
}

void SessionTracker::endSession() {
    if (!db || sessionDbId == -1) return;

    std::string sql = "UPDATE sessions SET end_time = CURRENT_TIMESTAMP WHERE session_id = " + std::to_string(sessionDbId) + ";";
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to update session: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_close(db);
    db = nullptr;
    sessionDbId = -1;
    lastCommandId = -1;
}

void SessionTracker::openLogFile() {
    const std::string dbPath = "./local/session_history.db";
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open SQLite database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return;
    }

    std::string schema;
    try {
        std::ifstream inFile("./schema.sql");
   		if (!inFile) {
        	throw std::runtime_error("Can't open file: ./schema.sql");
    	}
    	std::stringstream buffer;
    	buffer << inFile.rdbuf();
    	schema = buffer.str();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return;
    }

    if (sqlite3_exec(db, schema.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to create schema: " << sqlite3_errmsg(db) << std::endl;
    }
}

//---------------------------------- tracking user interaction ---------------------------------------

void SessionTracker::logAgentInteraction(const std::string& question, const std::string& response) {
    if (!db || sessionDbId == -1) return;

    char *escapedQuestion = sqlite3_mprintf("%q", question.c_str());
    char *escapedResponse = sqlite3_mprintf("%q", response.c_str());

    std::string sql = "INSERT INTO interactions (session_id, agent_question, agent_message) VALUES (" +
                      std::to_string(sessionDbId) + ", '" + escapedQuestion + "', '" + escapedResponse + "');";

    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "DB insert interaction failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_free(escapedQuestion);
    sqlite3_free(escapedResponse);
}

void SessionTracker::addNewCommand(EventType command_type) {
    if (!db) return;

    bool is_shell_command = (command_type == EventType::ShellCommand);

    auto callback = [](void *data, int argc, char **argv, char **) -> int {
        int *pmax = static_cast<int*>(data);
        *pmax = argv[0] ? std::atoi(argv[0]) : 0;
        return 0;
    };

    int last_interaction_id = 0;
    std::string sql = "SELECT MAX(interaction_id) AS max_id FROM interactions;";
    if (sqlite3_exec(db, sql.c_str(), callback, &last_interaction_id, nullptr) != SQLITE_OK) {
        std::cerr << "Error selecting max interaction_id\n";
    }

    sql = "INSERT INTO commands (interaction_id, command_text, shell_command) VALUES (" +
                     (last_interaction_id > 0 ? std::to_string(last_interaction_id) : "NULL") +
                     ", '', " + 
                     (is_shell_command ? "1" : "0") + 
                     ");";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to insert command: " << sqlite3_errmsg(db) << std::endl;
        lastCommandId = -1;
    } else {
        lastCommandId = sqlite3_last_insert_rowid(db);
    }
}

void SessionTracker::appendCommandText(const std::string& text) {
    if (!db || lastCommandId == -1) return;

    char *escapedText = sqlite3_mprintf("%q", text.c_str());

    std::string sql = "UPDATE commands SET command_text = '" + std::string(escapedText) + 
                     "' WHERE command_id = " + std::to_string(lastCommandId) + ";";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to update command text: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_free(escapedText);
}

void SessionTracker::setCommandOutput(const std::string& output) {
    if (!db || lastCommandId == -1) return;

    char *escapedOutput = sqlite3_mprintf("%q", output.c_str());

    std::string sql = "UPDATE commands SET output = '" + std::string(escapedOutput) + 
                     "', execution_end = CURRENT_TIMESTAMP WHERE command_id = " + 
                     std::to_string(lastCommandId) + ";";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to update command output: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_free(escapedOutput);
}

void SessionTracker::setExitCode(int exit_code) {
    if (!db || lastCommandId == -1) return;

    std::string sql = "UPDATE commands SET exit_code = " + std::to_string(exit_code) + 
                     " WHERE command_id = " + std::to_string(lastCommandId) + ";";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to update command exit code: " << sqlite3_errmsg(db) << std::endl;
    }
}

//----------------------------------------- get methods ------------------------------------------

const std::vector<SessionTracker::Interaction>& SessionTracker::get_history() const {
    history.clear();
    if (!db || sessionDbId <= 0)
        return history;

    std::unordered_map<int, std::size_t> rowOf;
    char* err = nullptr;

    const std::string sqlInts =
        "SELECT interaction_id, timestamp, agent_question, agent_message "
        "FROM interactions "
        "WHERE session_id = " + std::to_string(sessionDbId) + " "
        "ORDER BY interaction_id ASC;";

    auto intsCB = [](void* ctx, int n, char** row, char**) -> int
    {
        using Pair = std::pair<std::vector<Interaction>*, std::unordered_map<int,std::size_t>*>;
        auto* p   = static_cast<Pair*>(ctx);
        auto& vec = *p->first;
        auto& map = *p->second;
        if (n < 4) return 0;

        Interaction inter;
        inter.interaction_id = std::atoi(row[0]);
        inter.timestamp      = row[1] ? row[1] : "";
        inter.question       = row[2] ? row[2] : "";
        inter.answer         = row[3] ? row[3] : "";

        map[inter.interaction_id] = vec.size();
        vec.push_back(std::move(inter));
        return 0;
    };

    std::pair<std::vector<Interaction>*, std::unordered_map<int,std::size_t>*> ctxInts{&history, &rowOf};
    if (sqlite3_exec(db, sqlInts.c_str(), intsCB, &ctxInts, &err) != SQLITE_OK)
    {
        if (err) sqlite3_free(err);
        return history;
    }

    if (history.empty())
        return history;

    // shell commands
    const std::string sqlCmds =
        "SELECT interaction_id, command_text, output, "
        "       execution_start, execution_end, exit_code "
        "FROM commands "
        "WHERE shell_command = 1 "
        "  AND interaction_id IN ("
        "        SELECT interaction_id "
        "        FROM interactions "
        "        WHERE session_id = " + std::to_string(sessionDbId) + ") "
        "ORDER BY interaction_id ASC, command_id ASC;";

    auto cmdsCB = [](void* ctx, int n, char** row, char**) -> int
    {
        using Pair = std::pair<std::vector<Interaction>*, std::unordered_map<int,std::size_t>*>;
        auto* p   = static_cast<Pair*>(ctx);
        auto& vec = *p->first;
        auto& map = *p->second;
        if (n < 6) return 0;

        const int id = std::atoi(row[0]);
        auto pos = map.find(id);
        if (pos == map.end()) return 0;

        ShellCmd cmd;
        cmd.interaction_id   = id;
        cmd.command          = row[1] ? row[1] : "";
        cmd.output           = row[2] ? row[2] : "";
        cmd.execution_start  = row[3] ? row[3] : "";
        cmd.execution_end    = row[4] ? row[4] : "";
        cmd.exit_code        = row[5] ? std::atoi(row[5]) : 0;

        vec[pos->second].shell.push_back(std::move(cmd));
        return 0;
    };

    sqlite3_exec(db, sqlCmds.c_str(), cmdsCB, &ctxInts, &err);
    if (err) sqlite3_free(err);

    return history;
}