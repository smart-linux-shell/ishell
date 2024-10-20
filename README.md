# AI Powered Linux Shell

[![Build and push docker image to Docker Hub](https://github.com/smart-linux-shell/ishell/actions/workflows/on-push.yml/badge.svg)](https://github.com/smart-linux-shell/ishell/actions/workflows/on-push.yml)

This project is a dual-pane Linux terminal designed to enhance user experience by integrating an AI-powered agent in one pane and a traditional bash shell in the other. The agent helps users with different levels of experience, by answering queries, running Linux commands, and managing bookmarks for frequently used commands, making it easier to navigate and troubleshoot Linux systems.

### Contributors

This project was part of a Research Internship at Jetbrains. It was collaboratively developed by:  
  - [Iulian Alexa](https://github.com/iulianalexa), 
  - [Milica Sladaković](https://github.com/coma007),  

under mentorhsip of [Kirill Krinkin](https://github.com/krinkin). 

## Table of Contents

- [Overview](#overview)
- [Installation](#installation)
  - [Option 1: Installing via `apt`](#option-1-installing-via-apt)
  - [Option 2: Compiling Manually](#option-2-compiling-manually)
  - [Running the Server](#running-the-server)
  - [Environment Variables](#environment-variables) 
- [Usage](#usage)
  - [Keybinds](#keybinds)
  - [System Mode](#system-mode)
  - [Demo](#demo)
  - [Tests](#tests)

## Overview

The **AI Powered Linux Shell** splits terminal into two sections:

- **Bash (bottom section):** A regular bash shell where you can run your standard Linux commands, as well as interactive applications, such as `nano` and `top`. 
- **Agent (top section):** An AI-powered agent that helps with Linux queries, troubleshooting, and command management.  The agent uses a large language model (LLM) to answer Linux-related questions, keeping context within a session, with the ability to bookmark frequently used queries.

## Installation

### Option 1: Installing via `apt` 

**[WIP - need to update address/deb to the correct URL]**

A GPG key is required to sign the .deb package:
- _public key_ should be hosted in: `pkg/KEY.gpg`
- _private key_ should be pasted as a secret in: `secrets.DEB_GPG_KEY`

Add repository to `apt`:
```bash
curl -s --compressed "_address_/deb/KEY.gpg" | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/ishell.gpg >/dev/null
sudo curl -s --compressed -o /etc/apt/sources.list.d/ishell.list "_address_/deb/ishell.list"
sudo apt update
```

Then, install:
```bash
sudo apt install ishell
```

Run with command `ishell`.

### Option 2: Compiling Manually
Clone the source code:
```bash
git clone git@github.com:smart-linux-shell/ishell.git
```
Compile and run ishell:
```bash
cd ishell
./run.sh
```

### Running the Server

_**Important information**_: In order to use the inspector agent, a SSH server must be setup on the user's machine, and a provided public key must be installed. Inspector agent can be found [here](https://github.com/smart-linux-shell/agency).

Clone the source code and run server:
```bash
git@github.com:smart-linux-shell/agency.git
cd agency
./run.sh
```

### Environment Variables 

- `ISHELL_AGENCY_URL` - base url for agency server (**optional** - by default will use our own agency)
- `ISHELL_LOCAL_DIR` - directory for local files (**optional** - by default creates and uses `~/.ishell`)
- `SSH_IP` - IP address for SSH server running on the user's system (**required** - for inspector agent)
- `SSH_PORT` - port for SSH server runinng on the user's system (**required** - for inspector agent, by default `22`)

## Usage

Once `ishell` is up and going, you can use the bottom pane as a regular bash, and top pane as an agent.

### Keybinds

- `CTRL-D` to exit
- `CTRL-B; TAB` to switch the focused window
- `CTRL-B; Z` to zoom in/out
- `CTRL-B; [` to enter/leave manual scrolling mode
  - if focused on a window with manual scrolling mode enabled, scroll up and down can be down using arrow keys
- `TAB` in agent window, to switch to the System Mode

### System Mode
    
While focused on the agent window, special commands can be executed in System Mode. Those commands will not be sent to the LLM as queries, but will rather be processed as user commands. They include:
- `clear` - clears context
- `bookmark <index> <alias>` - Bookmark the query at the given history index with the specified alias.
- `bookmark <alias>` - Bookmark the last executed query with the specified alias.
- `bookmark -l` or `bookmark list`- List all saved bookmarks.
- `bookmark -r <alias>` or `bookmark --remove <alias>` - Remove the bookmark with the specified alias.
- `bookmark --help`  - Show help message.
- `<alias>` - Executes query with specified alias.

### Demo

Take a look at the this comprehensive demo that Iulian recorded:


https://github.com/user-attachments/assets/85efd772-f5ad-4a71-a060-4004ae521306




### Tests

Source code is well covered with unit tests that can be found [here](https://github.com/smart-linux-shell/ishell/tree/main/tui-tux/test).

To test, run:
```
cd ishell
./run.sh -t # or --test
```
#### Smoke Test
As the deb package is not deployed yet, the smoke test is not automated. To ensure that the ishell works correctly, please:
- Follow the installation guide and run the ishell.
- Verify that the shell opens in split-screen mode (assistant at the top, bash at the bottom).
- Run a simple bash command (e.g., `ls`) to check if bash is functional.
- Ask a Linux-related query in the assistant section to ensure it responds correctly.
- Use `Ctrl + TAB` to switch between the assistant and bash, confirming smooth switching.
- Run interactive applications (e.g., `top`, `nano`) in bash and verify they work properly.
- Exit the shell using `exit` and ensure both the assistant and bash terminate without issues.


### Happy Shelling 💻
