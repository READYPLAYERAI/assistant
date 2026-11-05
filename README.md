# assistant

A monorepo for LLM chat clients.

## Contents

- [`desktop/`](desktop/README.md) — desktop chat client: C++17, Qt 6 Widgets, streaming OpenRouter replies.
- `mobile/` — planned.

## Quick start (desktop)

Requires Qt 6 (`qt6-base`), CMake ≥ 3.21 and a C++17 compiler.

```sh
cmake -S desktop -B desktop/build -DCMAKE_BUILD_TYPE=Release
cmake --build desktop/build -j"$(nproc)"
./desktop/build/llmchat
```

Paste an OpenRouter API key from <https://openrouter.ai/keys> into the key field. See [`desktop/README.md`](desktop/README.md) for details, configuration and limitations.
