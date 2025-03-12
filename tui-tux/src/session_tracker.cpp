#include "session_tracker.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

SessionTracker& SessionTracker::getInstance() {
    static SessionTracker instance;
    return instance;
}

SessionTracker::SessionTracker() : lastInteractionOpen(false) {
    std::string session_id = generateSessionId();
    std::string dir_path = "./local";
    std::string filename = dir_path + "/session_" + session_id + ".json";

    struct stat info;
    if (stat(dir_path.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        if (mkdir(dir_path.c_str(), 0755) != 0) {
            perror("mkdir");
            std::cerr << "Cannot create directory: " << dir_path << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    logFile.open(filename);
    if (!logFile.is_open()) {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    sessionData["session_id"] = session_id;
}

std::string SessionTracker::generateSessionId() {
    // maybe it should be changed to real UUID-lib
    static const char* hex = "0123456789ABCDEF";
    std::ostringstream oss;
    std::srand(std::time(nullptr));

    for (int i = 0; i < 12; ++i) {
        int r = std::rand() % 16;
        oss << hex[r];
    }
    return oss.str();
}

void SessionTracker::startSession() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    sessionData["start_time"] = ts.str();
    sessionData["interactions"] = nlohmann::json::array();
}

void SessionTracker::logUserRequest(const std::string& query) {
    nlohmann::json interaction;
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream ts;

    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    interaction["timestamp"] = ts.str();
    interaction["user_request"] = query;

    sessionData["current_interaction"] = interaction;
    lastInteractionOpen = true;
}

void SessionTracker::logAgentResponse(const std::string& response) {
    if (!lastInteractionOpen) {
        logUserRequest("");
    }
    sessionData["current_interaction"]["agent_response"] = response;
    sessionData["interactions"].push_back(sessionData["current_interaction"]);
    sessionData.erase("current_interaction");
    lastInteractionOpen = false;
    // logFile << sessionData.dump(4) << std::endl;
}

void SessionTracker::logUserCommand(const std::string& command) {
    if (sessionData["interactions"].empty()) {
        nlohmann::json interaction;
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::ostringstream ts;

        ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        interaction["timestamp"] = ts.str();
        interaction["user_command"] = command;
        sessionData["interactions"].push_back(interaction);
    } else {
        nlohmann::json& last = sessionData["interactions"].back();
        if (last.contains("user_command")) {
            nlohmann::json interaction;
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);
            std::ostringstream ts;

            ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            interaction["timestamp"] = ts.str();
            interaction["user_command"] = command;
            sessionData["interactions"].push_back(interaction);
        } else {
            last["user_command"] = command;
        }
    }
}

void SessionTracker::logCommandResult(int exit_status, const std::string& output) {
    if (sessionData["interactions"].empty()) return;

    nlohmann::json& last = sessionData["interactions"].back();
    nlohmann::json result;

    result["exit_status"] = exit_status;
    result["output"] = output;
    last["command_result"] = result;
    // logFile << sessionData.dump(4) << std::endl;
}

void SessionTracker::endSession() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream ts;

    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    sessionData["end_time"] = ts.str();

    if (logFile.is_open()) {
        logFile << sessionData.dump(4);
        logFile.close();
    }
}

SessionTracker::~SessionTracker() {
    if (sessionData.contains("end_time") == false) {
        endSession();
    }
}