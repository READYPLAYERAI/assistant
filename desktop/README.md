# LLM Chat (desktop)

A minimal desktop chat client for [OpenRouter](https://openrouter.ai), written in C++17 with Qt 6 Widgets. It keeps a single conversation with streaming replies and a plain-text transcript. It targets KDE Plasma on Linux but is plain Qt.

## Requirements

On Arch Linux, install the Arch packages `qt6-base` (provides Widgets and Network), plus `cmake` and `g++`. No QML or `qt6-declarative` is needed. The build requires CMake ≥ 3.21 and a C++17 toolchain.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## Run

```sh
./build/llmchat
```

## API key

Get a key at <https://openrouter.ai/keys> and paste it into the key field (the field is masked). If "Remember key" is left unchecked the key stays in memory for the session only. If it is checked the key is persisted (see Config below).

## Config

The configuration file lives at `~/.config/llmchat/config.ini`. It uses the keys `api/model` and `api/rememberKey`, and `api/key` only when "Remember key" is enabled. Permissions are forced to `600` (owner read/write). The key is stored as plain text, so anyone who can read your home directory config can read it.

## Model

The model defaults to `deepseek/deepseek-flash-latest`. The model field accepts any OpenRouter model id.

## Usage

Type in the input box. `Enter` sends the message, `Shift+Enter` inserts a newline. `Stop` ends a streaming reply; the partial text is kept and marked `[stopped]`. `Clear` resets the conversation. Errors appear in the status bar and inline in the transcript as `[error] …`.

## Development

Set `LLMCHAT_OPENROUTER_URL` to point the client at a local stub server instead of the real endpoint. The default is `https://openrouter.ai/api/v1/chat/completions`.

## Install (optional)

```sh
cmake --install build --prefix ~/.local
```

This installs the `llmchat` binary to `~/.local/bin` and the desktop entry to `~/.local/share/applications`.

## Current limitations

- Plain text only: no markdown, HTML, or code highlighting.
- One conversation at a time, with no saved history.
- No token or cost display.
- No attachments or tool calling.
