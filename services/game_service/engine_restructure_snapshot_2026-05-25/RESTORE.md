# Engine folder snapshot (2026-05-25)

Flat `engine/` layout before subfolder restructure. Use this to revert.

## Restore original flat layout

From `services/game_service/`:

```bash
rm -rf engine
cp -a engine_restructure_snapshot_2026-05-25/engine engine
```

Then update `main.cpp` includes back to flat paths (e.g. `engine/game_loop.hpp`).

## Original tree

```
engine/
├── game_event_handlers.cpp
├── game_event_handlers.hpp
├── game_logic.cpp
├── game_logic.hpp
├── game_loop.cpp
├── game_loop.hpp
├── game_state.cpp
├── game_state.hpp
├── input_event.hpp
├── input_queue.hpp
├── message_builders.cpp
├── message_builders.hpp
├── output_queue.hpp
├── save_client.cpp
├── save_client.hpp
├── sender_thread.hpp
├── visitor.hpp
├── websocket_server.cpp
├── websocket_server.hpp
├── world_loader.cpp
├── world_loader.hpp
├── world_parsing.cpp
├── world_parsing.hpp
├── world_validation.cpp
└── world_validation.hpp
```

See `MANIFEST.txt` for the full file list.
