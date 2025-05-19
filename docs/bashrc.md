# What bashrc need to be for OSC 333

```

# --- OSC 333 marks for the multiplexer ---
_osc333_preexec() {
    [[ -n "$_osc333_in_cmd" ]] && return
    printf '\e]333;C\a'
    _osc333_in_cmd=1
}

_osc333_postexec() {
    local ec=$?
    printf '\e]333;D;%d\a' "$ec"
    unset _osc333_in_cmd
}

trap '_osc333_preexec' DEBUG             # «pre‑exec»
PROMPT_COMMAND='_osc333_postexec'        # «post‑exec»

PS1='\[\e]333;A\a\]\u@\h:\w\$ \[\e]333;B\a\]'
PS2='\[\e]333;A\a\]> \[\e]333;B\a\]'
PS3='\[\e]333;A\a\]#? \[\e]333;B\a\]'
```

description of a standart - https://gitlab.freedesktop.org/Per_Bothner/specifications/-/blob/master/proposals/semantic-prompts.md
