# Future Work

### Investigate whether a cheaper model would still work just as well

Currently, GPT-4o mini is much cheaper to use than the regular GPT-4o, while
still being a generally performant model. It would be a good idea to investigate
whether the app would work just as well with GPT-4o mini.

### Installed package information

Currently, the text app sends a list of all the installed packages to the
agent on each request -- this is very inefficient and expensive (to the
point that temporarily disabling the package list might be a good idea,
because the list can get very long -- right now, it costs as much as a few cents
per request to run the agency).

A possible optimisation would be giving the agent the necessary tools to
query the client and see if a particular package is installed or not on the
user's system, allowing the LLM to check directly for each package it needs to know
about, therefore spending much fewer tokens on useless information.
