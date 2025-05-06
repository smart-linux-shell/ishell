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
    : db(nullptr), sessionDbId(-1), lastInteractionId(-1), lastCommandId(-1),
      currentCommandText(""), currentCommandOutput("") {
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
    currentCommandText.clear();
    currentCommandOutput.clear();
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
    currentCommandText.clear();
    currentCommandOutput.clear();
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
        lastInteractionId = -1;
    } else {
        lastInteractionId = sqlite3_last_insert_rowid(db);
    }

    sqlite3_free(escapedQuestion);
    sqlite3_free(escapedResponse);
}

void SessionTracker::addNewCommand(EventType command_type) {
    if (!db) return;

    bool is_shell_command = (command_type == EventType::ShellCommand);

    std::string sql = "INSERT INTO commands (interaction_id, command_text, shell_command) VALUES (" +
                     (lastInteractionId > 0 ? std::to_string(lastInteractionId) : "NULL") +
                     ", '', " + 
                     (is_shell_command ? "1" : "0") + 
                     ");";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to insert command: " << sqlite3_errmsg(db) << std::endl;
        lastCommandId = -1;
    } else {
        lastCommandId = sqlite3_last_insert_rowid(db);
        currentCommandText.clear();
        currentCommandOutput.clear();
    }
}

void SessionTracker::appendCommandText(const std::string& text) {
    if (!db || lastCommandId == -1) return;

    if (!currentCommandText.empty()) {
        currentCommandText += "\n";
    }
    currentCommandText += text;

    char *escapedText = sqlite3_mprintf("%q", currentCommandText.c_str());

    std::string sql = "UPDATE commands SET command_text = '" + std::string(escapedText) + 
                     "' WHERE command_id = " + std::to_string(lastCommandId) + ";";
    
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to update command text: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_free(escapedText);
}

void SessionTracker::setCommandOutput(const std::string& output) {
    if (!db || lastCommandId == -1) return;

    currentCommandOutput = output;

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

    if (exit_code == 0 && lastInteractionId > 0) {
        sql = "UPDATE interactions SET resolved = 1 WHERE interaction_id = " + std::to_string(lastInteractionId);
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to mark interaction as resolved: " << sqlite3_errmsg(db) << std::endl;
        }
    }
}

