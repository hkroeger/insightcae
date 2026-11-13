# ISCAD assistant evaluation tasks

Modeling tasks for the automated evaluation of the ISCAD chat assistant with
`iscad-assistant-eval`. Each `*.json` file contains one task (or an array of tasks).
The task format and all check types are documented in
`src/cad/iscad/assistant/evaluation.h`.

## Usage

```bash
# check the task definitions (initial scripts and reference solutions), no model needed
iscad-assistant-eval --validate

# Claude (ANTHROPIC_API_KEY must be set)
iscad-assistant-eval --model claude-sonnet-5-5

# local model with an OpenAI compatible server (e.g. Ollama)
iscad-assistant-eval --provider openai --base-url http://localhost:11434/v1 \
    --model qwen3-coder:30b --reference basic --tools basic

# only some tasks, each three times (results vary between runs)
iscad-assistant-eval --filter 'modify|fix' --repeat 3
```

Each run writes `<prefix>.json` (all results with transcripts) and `<prefix>.md`
(summary table). Without `--out`, the prefix contains the model name and the date.

## Adding tasks

- Give every task a unique `id` and a `category` (create, modify, fix, refactor, inspect).
- Name the feature to be checked in the prompt (e.g. "named `plate`"), or use
  `"feature": "*"` to accept any feature of the model.
- Add a reference `solution` (and `solution_answer` for questions) and run
  `--validate`: the checks must pass for the solution.
- Expected values are in model units (mm). Numeric checks use a relative tolerance
  of 0.5 % and an absolute tolerance of 0.02 by default (`rel_tol`, `abs_tol`).
