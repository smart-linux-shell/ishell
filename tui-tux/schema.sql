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
    shell_command BOOLEAN DEFAULT 0,
    FOREIGN KEY(interaction_id) REFERENCES interactions(interaction_id)
);

