#include "session_tracker.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

SessionTracker& SessionTracker::get() {
    static SessionTracker instance;
    return instance;
}

SessionTracker::SessionTracker()
    : db(nullptr), sessionDbId(-1), lastInteractionId(-1), lastCommandId(-1) {
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

    lastInteractionId = -1;
    lastCommandId = -1;
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
    lastInteractionId = -1;
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

void SessionTracker::logEvent(EventType event_type, const std::string& data) {
    if (!db) return;

    char *escapedData = sqlite3_mprintf("%q", data.c_str());
    std::string sql;

    switch (event_type) {
        case EventType::ShellCommand:
            sql = "INSERT INTO commands (interaction_id, command_text, shell_command) VALUES (" +
                  (lastInteractionId > 0 ? std::to_string(lastInteractionId) : "NULL") +
                  ", '" + escapedData + "', 1);";
            if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK)
                lastCommandId = sqlite3_last_insert_rowid(db);
            break;

        case EventType::SystemCommand:
            sql = "INSERT INTO commands (interaction_id, command_text, shell_command) VALUES (" +
                  (lastInteractionId > 0 ? std::to_string(lastInteractionId) : "NULL") +
                  ", '" + escapedData + "', 0);";
            if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK)
                lastCommandId = sqlite3_last_insert_rowid(db);
            break;

        case EventType::Unknown:
            break;
    }

    sqlite3_free(escapedData);
}


void SessionTracker::logAgentInteraction(const std::string& question, const std::string& response) {
    if (!db || sessionDbId == -1) return;

    char *escapedQuestion = sqlite3_mprintf("%q", question.c_str());
    char *escapedResponse = sqlite3_mprintf("%q", response.c_str());

    std::string sql = "INSERT INTO interactions (session_id, agent_question, agent_message) VALUES (" +
                      std::to_string(sessionDbId) + ", '" + escapedQuestion + "', '" + escapedResponse + "');";

    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "DB insert interaction failed: " << sqlite3_errmsg(db) << std::endl;
        lastInteractionId = -1;
    } else {
        lastInteractionId = sqlite3_last_insert_rowid(db);
    }

    sqlite3_free(escapedQuestion);
    sqlite3_free(escapedResponse);
}

void SessionTracker::finalizeCommand(int exit_code, const std::string& output) {
    if (!db || lastCommandId == -1) return;

    char *escapedOutput = sqlite3_mprintf("%q", output.c_str());

    std::string sql = "UPDATE commands SET execution_end = CURRENT_TIMESTAMP, exit_code = " +
                      std::to_string(exit_code) + ", output = '" + escapedOutput +
                      "' WHERE command_id = " + std::to_string(lastCommandId) + ";";

    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "DB update command failed: " << sqlite3_errmsg(db) << std::endl;
    }

    if (exit_code == 0 && lastInteractionId > 0) {
        sql = "UPDATE interactions SET resolved = 1 WHERE interaction_id = " + std::to_string(lastInteractionId);
        sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }

    sqlite3_free(escapedOutput);
    lastCommandId = -1;
}
