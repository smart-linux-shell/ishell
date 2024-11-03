This file contains basic agent descriptions and functionality. Agents functionality defined as possible dialogs starts where U is user. Before answering, the agent should be instructed by exact linux distribution version and list of installed packages
### Assistant agent

This agent knows the system and can help user to learn how to use command line tools for performing particular tasks. It also can help with describing general concepts. 
#### Dialogs
- U: how to do...
- U: where can I find...
- U: which program(s) should I run for ...
- U: give me alternative program for...
- U: what is ...
- U: explain how does ... work
- U: generate shell commands for ...
- U: explain last output
### Inspector agent

This agent knows about system status and diagnostics tools and could bind them to fulfil user requests. 
#### Dialogs
- U: give general system overview
- U: give performance report
- U: give memory report
- U: give networking report
- U: give storage report
- U: give filesystem and storage report
- U: find potential bottlenecks
### Debugger agent

This agent knows about services and their config files and logs and can provide explanations and possible actions.
#### Dialogs
- U: give general services health
- U: find and explain service X errors during last X hours/minutes
- U: how to fix ...
- U: create configuration for ... service which ...
- TBD

