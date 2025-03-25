#include "session_tracker.hpp"
#include <iostream>

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

    // sessions -- table with different sessions
    // interactions -- table for user chat with agent
    // commands -- bash commands
    // system_commands -- commands in system mode
    const char *schemaSql = R"SQL(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            end_time TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS interactions (
            interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id INTEGER,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            agent_question TEXT,
            agent_message TEXT,
            recommendation_id TEXT,
            resolved BOOLEAN DEFAULT 0,
            FOREIGN KEY(session_id) REFERENCES sessions(session_id)
        );
        CREATE TABLE IF NOT EXISTS commands (
            command_id INTEGER PRIMARY KEY AUTOINCREMENT,
            interaction_id INTEGER,
            command_text TEXT,
            execution_start TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            execution_end TIMESTAMP,
            exit_code INTEGER,
            output TEXT,
            FOREIGN KEY(interaction_id) REFERENCES interactions(interaction_id)
        );
        CREATE TABLE IF NOT EXISTS system_commands (
            system_command_id INTEGER PRIMARY KEY AUTOINCREMENT,
            interaction_id INTEGER,
            command_text TEXT,
            execution_start TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            execution_end TIMESTAMP,
            exit_code INTEGER,
            output TEXT,
            FOREIGN KEY(interaction_id) REFERENCES interactions(interaction_id)
        );
    )SQL";

    if (sqlite3_exec(db, schemaSql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to create schema: " << sqlite3_errmsg(db) << std::endl;
    }
}

void SessionTracker::logEvent(EventType event_type, const std::string& data) {
    if (!db) return;

    char *escapedData = sqlite3_mprintf("%q", data.c_str());
    std::string sql;

    switch (event_type) {
        case EventType::ShellCommand:
            sql = "INSERT INTO commands (interaction_id, command_text) VALUES (" +
                  (lastInteractionId > 0 ? std::to_string(lastInteractionId) : "NULL") +
                  ", '" + escapedData + "');";
            if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK)
                lastCommandId = sqlite3_last_insert_rowid(db);
            break;

        case EventType::SystemCommand:
            sql = "INSERT INTO system_commands (interaction_id, command_text) VALUES (" +
                  (lastInteractionId > 0 ? std::to_string(lastInteractionId) : "NULL") +
                  ", '" + escapedData + "');";
            if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK)
                lastCommandId = sqlite3_last_insert_rowid(db);
            break;

        case EventType::SystemMessage: // for future
        case EventType::UserQuestion:
        case EventType::AgentResponse:
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
