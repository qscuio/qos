# Codex Cache Export Manifest

Source cache file: `/home/ubuntu/.codex/sessions/2026/03/13/rollout-2026-03-13T07-48-46-019ce62b-57f2-77d1-ae4d-b7ddb927c336.jsonl`
Export file: `/home/ubuntu/work/qos/docs/superpowers/sessions/2026-03-14-linux1-authentic-cache-sanitized.jsonl`

This export is a sanitized transcript derived from the local Codex rollout cache for the Linux 1.0.0 session.

Included records:
- `custom_tool_call`: 176
- `custom_tool_call_output`: 176
- `function_call`: 2121
- `function_call_output`: 2119
- `message:assistant`: 557
- `message:user`: 46
- `web_search_call`: 2

Protected records omitted:
- developer messages
- session metadata and base instructions
- encrypted reasoning records
- internal event plumbing (`event_msg`, `turn_context`, `compacted`)

Redacted tool outputs: 5

Omitted record counts:
- `compacted`: 13
- `event_msg`: 2560
- `message:developer`: 4
- `reasoning`: 1893
- `session_meta`: 1
- `turn_context`: 42

Interpretation:
- `message:user` and `message:assistant` are the visible prompt and response stream preserved in cache.
- `function_call` and `custom_tool_call` are tool invocations issued during the session.
- `function_call_output` and `custom_tool_call_output` are the recorded tool results.
- A small number of tool outputs were redacted because they echoed protected cache material during later cache inspection.
- The export intentionally excludes hidden system/developer material and encrypted reasoning blocks.
