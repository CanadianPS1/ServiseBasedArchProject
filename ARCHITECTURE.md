# ET Game — Architecture & Design Reference

## 1. Game scope

A subset of the original Atari E.T. game. The player controls ET, navigates a small map of rooms with a fixed camera, and collects 3 phone pieces. ET has an energy meter that depletes over time; running out of energy is the lose condition. Candy is scattered through the world and can be consumed to restore energy.

**In scope:**
- Fixed camera; ET sprite moves with player input
- Rooms with tile grids; multiple tilesets so rooms look visually distinct
- Room-to-room transitions when ET walks off a screen edge
- 3 phone pieces scattered across the map, collected by walking over them
- Candy pickups scattered across the map, collected by walking over them
- Energy system: depletes over time, restored by consuming candy
- HUD displays current phone piece count and candy count
- A "consume candy" button in the GUI that increments energy and decrements candy count
- Win condition: all 3 phone pieces collected (with an optional ending animation of ET boarding the ship)
- Lose condition: energy reaches 0
- Save/load: one rolling save per user, both automatic and manual
- Multiple users with independent saves (game prompts for name on startup, looks up existing save or starts a new game)

**Out of scope:**
- FBI/scientist agents
- Pits and climb-out mechanics
- Power zones
- Timer (replaced by energy)
- Inventory system (counts are tracked directly, not as a bag of items)
- Concurrent users (one player at a time; users are distinguished by name but not authenticated)
- Real authentication (no passwords; the name is trusted)

## 2. System architecture

Three services communicating over three different patterns, all running on localhost via Docker Compose.

```
┌────────────────────┐    WebSocket (ws://)    ┌────────────────────┐
│  Vulkan Service    │◄──────────────────────►│  Game State Svc    │
│  (C++)             │     state ↔ input      │  (C++)             │
│  - Rendering       │                         │  - Simulation      │
│  - Input capture   │                         │  - Game logic      │
│  - GUI (HUD,       │                         │  - world.json      │
│    candy button)   │                         │                    │
└────────────────────┘                         └─────────┬──────────┘
                                                         │
                                       HTTP GET /save    │  publish save events
                                       (on connect)      │  (during play)
                                                         ▼
                                          ┌──────────────────────────┐
                                          │  RabbitMQ                │
                                          │  - "saves" queue         │
                                          └────────────┬─────────────┘
                                                       │ consume
                                                       ▼
                                          ┌──────────────────────────┐    ┌──────────┐
                                          │  Auth/Save Service       │───►│ MongoDB  │
                                          │  (Python, FastAPI)       │    └──────────┘
                                          │  - HTTP server (reads)   │
                                          │  - Queue consumer (writes)│
                                          └──────────────────────────┘
```

**Why each communication pattern:**

| Path | Pattern | Reason |
|---|---|---|
| Vulkan ↔ Game state | WebSocket | Persistent, bidirectional, low-latency. Game data flows continuously. |
| Game state → Save (load) | HTTP GET | Synchronous: the game state service needs the save right now to start. |
| Game state → Save (write) | RabbitMQ | Async, durable, fire-and-forget. Game loop never blocks on persistence. |

## 3. Service responsibilities

### Vulkan Service (C++)
- Owns: window, Vulkan instance, GPU resources, tileset PNGs, ET sprite, GUI/HUD assets, input capture
- Stateless about game logic — does not compute movement, collision, energy depletion, win/lose, etc.
- WebSocket client to game state service
- On startup: connect WS, receive initial state, begin render loop
- On input: send input event over WS, await state updates, re-render

### Game State Service (C++)
- Owns: `world.json` (loaded at startup), live in-memory simulation state (per active session)
- WebSocket server; Vulkan service connects to it
- HTTP client to save service for loading saves on connect
- RabbitMQ publisher for save events
- On Vulkan connect:
  1. `GET /save/{userId}` from save service
  2. Initialize in-memory state from save (or fresh state if 404)
  3. Send initial state over WebSocket
  4. Enter event loop: read inputs, mutate state, push state updates, publish save events
- On Vulkan disconnect: drop in-memory state (Mongo retains the truth)

### Auth/Save Service (Python, FastAPI)
- Owns: Mongo connection, save schema
- HTTP server: `GET /save/{userId}` returns latest save or 404
- RabbitMQ consumer: pulls save events from queue, upserts to Mongo
- Reads (HTTP) and writes (queue) run concurrently in one process (FastAPI + asyncio consumer)

## 4. Game state service — internal representation

The game state service represents the world as a **graph of rooms with per-room local coordinates**. Each room is a node; exits are directed edges between nodes. Each room has its own coordinate system starting at `(0, 0)` in its top-left.

### Why a graph with local coordinates

- Rooms are discrete units of gameplay with hard transitions, not cells in a continuous world. A graph models this directly.
- Each room can have different dimensions without weird gaps in a global grid.
- The graph can express non-Euclidean connections if needed (one-way exits, warps) without coordinate contradictions.
- Collision and pickup detection only ever consult the *current* room's data — simple and cache-friendly.
- The save schema can store `currentRoomId` as a semantic identifier rather than coordinates, which survives map edits.

The renderer (Vulkan service) is free to lay out rooms in a global render space if it wants — this is an internal decision on its side, decoupled from how the game state service represents the world. The WebSocket protocol speaks in per-room local tile coordinates; the renderer translates to its own render space as needed.

### Conceptual model

```
World (the whole game map)
 ├── Map<RoomId, Room>          // all rooms, loaded from world.json
 ├── PickupTypeRegistry          // pickup type definitions
 └── Config                      // starting energy, drain rate, etc.

Room (one node in the graph)
 ├── id: string                  // "house_01"
 ├── tileset: string             // "house" — reference to tileset asset
 ├── width, height: int          // tiles
 ├── tiles: vector<TileId>       // flat array, indexed as y*width + x
 ├── exits: vector<Exit>         // outgoing edges in the graph
 └── pickupSpawns: vector<PickupSpawn>

Exit (a directed edge in the graph)
 ├── trigger: ExitTrigger        // what makes this exit fire
 ├── destinationRoomId: string   // which node we jump to
 └── spawnPoint: Vec2            // where in the destination we appear
```

ET's position is `(currentRoomId, localX, localY)` — never a global coordinate. When ET walks east off `house_01` and enters `forest_01`, ET's coordinates reset to whatever `spawnPoint` the exit specifies in `forest_01`'s coordinate system.

### C++ types

Static world data (immutable after loading):

```cpp
using RoomId = std::string;
using PickupId = std::string;
using ItemTypeId = std::string;
using TileId = uint8_t;  // 0-255 tile indices

enum class Direction { North, South, East, West };

struct Vec2 {
    float x, y;
};

struct ExitTrigger {
    enum class Kind { Edge, Region };
    Kind kind;
    Direction edge;          // only used if kind == Edge
    // (if Region is added later: int x, y, w, h in tile coords)
};

struct Exit {
    ExitTrigger trigger;
    RoomId destinationRoomId;
    Vec2 spawnPoint;
};

struct PickupSpawn {
    PickupId id;             // unique across the world: "pickup_house01_antenna"
    ItemTypeId type;         // "phone_antenna", "candy", etc.
    int x, y;                // tile coordinates within this room
};

struct Room {
    RoomId id;
    std::string tilesetName;
    int width, height;
    std::vector<TileId> tiles;        // flat, indexed as y*width + x
    std::vector<Exit> exits;
    std::vector<PickupSpawn> pickupSpawns;

    TileId tileAt(int x, int y) const {
        return tiles[y * width + x];
    }
};

struct PickupTypeDef {
    enum class Kind { PhonePiece, Candy };
    Kind kind;
    std::string displayName;
};

struct WorldConfig {
    float startingEnergy;
    float energyDrainPerSecond;
    float energyPerCandy;
    RoomId startRoomId;
    Vec2 startSpawn;
    std::vector<ItemTypeId> requiredPhonePieces;
};

struct World {
    WorldConfig config;
    std::unordered_map<ItemTypeId, PickupTypeDef> pickupTypes;
    std::unordered_map<RoomId, Room> rooms;
};

struct Tileset {
    int tileSize;
    int tilesPerRow;
    struct TileDef {
        std::string name;
        bool walkable;
    };
    std::unordered_map<TileId, TileDef> tiles;
};
```

Dynamic session state (mutates during play, serialized to saves):

```cpp
enum class GameStatus { Playing, Won, Lost };

struct PlayerState {
    RoomId currentRoomId;
    Vec2 localPos;
    Direction facing;        // for sprite rendering
};

struct GameSession {
    std::string userId;
    PlayerState player;
    float energy;
    std::vector<ItemTypeId> phonePiecesCollected;
    int candyCount;
    std::unordered_set<PickupId> collectedPickups;
    GameStatus status;

    const Room& currentRoom(const World& world) const {
        return world.rooms.at(player.currentRoomId);
    }

    bool isPickupActive(const PickupId& id) const {
        return collectedPickups.find(id) == collectedPickups.end();
    }
};
```

Key separation: `World` is static and never mutates after `world.json` is loaded. `GameSession` is the dynamic per-session state — this is what gets serialized to RabbitMQ as save events and reconstructed from saves on connect.

### Runtime loop sketch

```cpp
void tick(GameSession& session, const World& world,
          const std::unordered_map<std::string, Tileset>& tilesets,
          float dt) {

    const Room& room = session.currentRoom(world);
    const Tileset& tileset = tilesets.at(room.tilesetName);

    // Movement + collision
    Vec2 proposed = computeProposedPosition(session.player, dt);
    int tileX = (int)proposed.x;
    int tileY = (int)proposed.y;
    TileId tile = room.tileAt(tileX, tileY);
    if (tileset.tiles.at(tile).walkable) {
        session.player.localPos = proposed;
    }

    // Exit check
    for (const Exit& exit : room.exits) {
        if (triggerHit(exit.trigger, session.player.localPos, room)) {
            session.player.currentRoomId = exit.destinationRoomId;
            session.player.localPos = exit.spawnPoint;
            publishSaveEvent(session, "room_transition");
            sendRoomLoadedToClient(session, world);
            break;
        }
    }

    // Pickup check
    for (const PickupSpawn& pickup : room.pickupSpawns) {
        if (!session.isPickupActive(pickup.id)) continue;
        if (!playerOverlapsTile(session.player.localPos, pickup.x, pickup.y)) continue;

        session.collectedPickups.insert(pickup.id);
        const PickupTypeDef& def = world.pickupTypes.at(pickup.type);
        if (def.kind == PickupTypeDef::Kind::PhonePiece) {
            session.phonePiecesCollected.push_back(pickup.type);
            checkWinCondition(session, world);
        } else if (def.kind == PickupTypeDef::Kind::Candy) {
            session.candyCount++;
        }
        publishSaveEvent(session, "pickup");
    }

    // Energy drain
    session.energy -= world.config.energyDrainPerSecond * dt;
    if (session.energy <= 0) {
        session.energy = 0;
        session.status = GameStatus::Lost;
        publishSaveEvent(session, "lose");
        sendGameOver(session);
    }

    sendStateUpdate(session);  // throttled
}
```

Every check is "static `World` data + dynamic `GameSession` state → decision." That's the data-driven design paying off.

### Load-time validation

When loading `world.json`, the game state service should validate:

- Every `destinationRoomId` in every exit refers to a real room
- Every `type` in every `pickupSpawn` refers to a real entry in `pickupTypes`
- Every ID in `requiredPhonePieces` has at least one pickup spawn somewhere
- Every room is reachable from `startRoomId` (BFS following exits)
- `startRoomId` exists and `startSpawn` is within its bounds

Fail loud at startup if any of these are violated. This catches the vast majority of authoring mistakes before they become runtime bugs.

## 5. MongoDB schema

One database `et_game`, one collection `saves`. The collection holds one document per user — each document is a complete snapshot of that user's most recent game state.

### Relationship to the RabbitMQ save flow (section 13)

Mongo is touched in only two places: reads from the FastAPI endpoints (section 12) and writes from the RabbitMQ consumer. The game state service never talks to Mongo directly — it publishes save events to RabbitMQ, and the consumer translates those events into upserts. This means:

- The schema below is the **shape of what the consumer writes**, derived from the save event payload (section 10).
- The schema is also **what the HTTP read endpoint returns** when the game state service loads a save on connect.
- The save event JSON and the Mongo document are nearly identical in shape — the consumer essentially unwraps the event's `state` field, adds the `userId` and `savedAt` from the envelope, and upserts.

This keeps the data flow simple: the game state service designs the save event shape, and the Mongo schema follows from it directly. There's no separate schema definition exercise.

### What a save document needs to contain

Every save document represents one user's current game state. At minimum it must hold:

- A **user identifier** so the document can be looked up.
- A **timestamp** of when this save was written, used both for display ("last saved at...") and for the out-of-order guard during writes.
- A **schema version** so future field changes can be migrated without breaking old saves.
- The **player's location**: which room they're in and where in that room.
- The **player's stats**: energy, anything else that depletes or accumulates.
- The **player's progress**: which phone pieces have been collected, how many candies the player currently holds.
- The **world's depletion state**: which pickup spawn points have already been collected (so they don't reappear).
- The **game status**: whether play is ongoing, won, or lost.

Anything derivable from `world.json` is *not* stored — room geometry, tileset references, pickup positions, etc. all stay in the static world data. The save only holds what differs per playthrough.

### Example document

```javascript
{
  _id: ObjectId("..."),
  userId: "...",
  savedAt: ISODate("..."),
  schemaVersion: 1,

  player: {
    currentRoomId: "...",
    localX: 0.0,
    localY: 0.0
  },

  energy: 0,

  phonePiecesCollected: [ ... ],   // list of phone piece IDs collected
  candyCount: 0,                   // current candy count in HUD

  collectedPickups: [ ... ],       // pickup spawn IDs that have been depleted

  status: "playing"                // "playing" | "won" | "lost"
}
```

**Field notes:**
- `userId` is the player's entered name (sanitized — see section 12).
- `phonePiecesCollected` and `collectedPickups` overlap in information but serve different purposes. `phonePiecesCollected` is the *win-condition check* (do you have all required pieces?). `collectedPickups` is the *world depletion check* (should this pickup spawn render?). Candy pickups appear in `collectedPickups` but not in `phonePiecesCollected`.
- `candyCount` is a number, not a list, because candies are fungible. It can also decrease (when consumed), whereas `collectedPickups` is monotonically increasing.
- `status` is persisted so loading a finished save shows the correct end screen rather than dropping the player back into a dead game.

### Indexes

- `{ userId: 1 }` — unique. Enforces one document per user, makes `findOne({ userId })` fast.

### Consumer code

The RabbitMQ consumer (running inside the auth/save service) processes each save event into a Mongo upsert. Using `motor` (async Mongo driver) and `aio-pika` (async RabbitMQ client), it looks roughly like this:

```python
import json
from datetime import datetime
import aio_pika
from motor.motor_asyncio import AsyncIOMotorClient

mongo = AsyncIOMotorClient("mongodb://mongo:27017")
db = mongo.et_game

async def handle_save_event(message: aio_pika.IncomingMessage):
    async with message.process(requeue=True):
        event = json.loads(message.body)
        user_id = event["userId"]
        saved_at = datetime.fromisoformat(event["savedAt"])
        state = event["state"]

        # Upsert with the savedAt guard: only write if incoming is newer
        # than what's already stored (or if no document exists yet).
        await db.saves.update_one(
            {
                "userId": user_id,
                "$or": [
                    { "savedAt": { "$lt": saved_at } },
                    { "savedAt": { "$exists": False } }
                ]
            },
            {
                "$set": {
                    "savedAt": saved_at,
                    "schemaVersion": 1,
                    "player": state["player"],
                    "energy": state["energy"],
                    "phonePiecesCollected": state["phonePiecesCollected"],
                    "candyCount": state["candyCount"],
                    "collectedPickups": state["collectedPickups"],
                    "status": state["status"]
                },
                "$setOnInsert": { "userId": user_id }
            },
            upsert=True
        )

async def consume_saves():
    connection = await aio_pika.connect_robust("amqp://rabbitmq/")
    channel = await connection.channel()
    queue = await channel.declare_queue("saves", durable=True)
    await queue.consume(handle_save_event)
```

The `savedAt` guard in the filter is what prevents stale (out-of-order redelivered) messages from clobbering newer saves. If the incoming event's `savedAt` isn't newer than what's stored, the filter doesn't match, and the upsert silently does nothing — which is the correct behavior for a stale message.

`message.process(requeue=True)` automatically acks the message on success and nacks-with-requeue on exception, so Mongo errors trigger a retry.

## 6. `world.json` structure

Static content, shipped with the game state service. Loaded once at startup, validated, then held in memory immutably for the lifetime of the process. Authored by hand — the file *is* the level editor.

The file has three top-level objects: `config` (global game parameters), `pickupTypes` (the registry of every kind of pickup that can exist), and `rooms` (every room in the world). Plus a `schemaVersion` field that lets future-you add fields without breaking old data.

### Top-level structure

```json
{
  "schemaVersion": 1,
  "config": { ... },
  "pickupTypes": { ... },
  "rooms": { ... }
}
```

### `schemaVersion`

**Type:** integer.
**Required:** yes.

Used to detect and migrate older `world.json` files if you ever change the file format. For MVP it's always `1`. If you change the structure later (e.g., split rooms into separate files, add a new exit type), increment this and have the loader branch on it.

### `config`

Global game-wide parameters that the simulation reads at startup.

```json
"config": {
  "startingEnergy": 100,
  "energyDrainPerSecond": 0.5,
  "energyPerCandy": 25,
  "startRoomId": "house_01",
  "startSpawn": { "x": 5, "y": 6 },
  "requiredPhonePieces": ["phone_antenna", "phone_dial", "phone_speaker"]
}
```

| Field | Type | Description |
|---|---|---|
| `startingEnergy` | number | Energy a brand-new game starts with. The save's `energy` field is initialized to this. |
| `energyDrainPerSecond` | number | How fast energy depletes during play. Tune for difficulty — `0.5` gives 200s of play from a full bar. |
| `energyPerCandy` | number | How much energy is restored when the player consumes one candy. |
| `startRoomId` | string | The room a new game spawns in. Must be a key in `rooms`. |
| `startSpawn` | `{x, y}` (numbers) | Tile coordinates within `startRoomId` where ET appears at game start. Must be a walkable tile. |
| `requiredPhonePieces` | array of strings | Pickup-type IDs that must all be collected to win. Each must be a key in `pickupTypes` with `kind: "phonePiece"`, and each must have at least one corresponding `pickupSpawn` somewhere in `rooms`. |

### `pickupTypes`

A registry of every kind of pickup that can exist in the game. Keyed by pickup-type ID.

```json
"pickupTypes": {
  "phone_antenna":  { "kind": "phonePiece", "displayName": "Antenna" },
  "phone_dial":     { "kind": "phonePiece", "displayName": "Dial" },
  "phone_speaker":  { "kind": "phonePiece", "displayName": "Speaker" },
  "candy":          { "kind": "candy",      "displayName": "Reese's Pieces" }
}
```

| Field | Type | Description |
|---|---|---|
| `kind` | string | One of `"phonePiece"` or `"candy"`. The simulation branches on this when a pickup is collected: `phonePiece` is added to `phonePiecesCollected` and triggers a win-condition check; `candy` increments `candyCount`. |
| `displayName` | string | Human-readable name. Currently unused (the HUD shows counts, not names), but useful for debugging and future tooltips. |

Note the distinction: `pickupTypes` defines what *kinds* of pickups exist; individual pickup *spawns* in rooms reference these types. Many candy spawns can all reference the single `"candy"` type, but each candy spawn has its own unique `id` in `collectedPickups`.

### `rooms`

A map from `RoomId` (string) to a `Room` object. Each room is one node in the game's room graph.

```json
"rooms": {
  "house_01": {
    "tileset": "house",
    "width": 20,
    "height": 12,
    "tiles": [
      "11111111111111111111",
      "10000000000000000001",
      "10000000033000000001",
      "..."
    ],
    "exits": [
      {
        "trigger": { "kind": "edge", "side": "E" },
        "destinationRoomId": "forest_01",
        "spawnPoint": { "x": 1, "y": 6 }
      }
    ],
    "pickupSpawns": [
      { "id": "pickup_house01_antenna", "type": "phone_antenna", "x": 9, "y": 2 },
      { "id": "pickup_house01_candy_a", "type": "candy",         "x": 14, "y": 8 }
    ]
  },
  "forest_01": { ... }
}
```

Each room object has:

| Field | Type | Description |
|---|---|---|
| `tileset` | string | Name of the tileset this room uses for rendering and walkability. Must correspond to a tileset file in `assets/tilesets/` (see section 7). |
| `width` | integer | Width of the room in tiles. |
| `height` | integer | Height of the room in tiles. |
| `tiles` | array of strings | The room's tile grid. Each string is one row, each character is a tile-index (`'0'`-`'9'`, or letters for >10 types). Must have exactly `height` rows, each with exactly `width` characters. |
| `exits` | array of Exit | Outgoing edges in the room graph. Can be empty (a dead-end room). |
| `pickupSpawns` | array of PickupSpawn | Where pickups appear in this room. Can be empty. |

#### `tiles`: how to read and write the grid

Each character is a single-digit tile index that maps to a tile definition in the room's tileset metadata (section 7). The string layout corresponds directly to what the player sees: row 0 is the top of the room, column 0 is the left.

For example, with the tileset mapping `0 = floor`, `1 = wall`, `3 = carpet`, the row `"10000000033000000001"` reads as:
- Wall, eleven floors, two carpets, six floors, wall.

This format means you can literally see the room when you open the JSON file. The loader parses each character into the flat `tiles` vector with `tiles[y * width + x] = rowStr[x] - '0'`.

Indices `'0'`–`'9'` cover ten tile types per tileset. If a tileset needs more, the convention is to extend with letters (`'a'` = 10, `'b'` = 11, etc.) and adjust the loader. For MVP, 10 types per tileset is plenty.

#### `exits`: how room-to-room transitions are described

Each exit is a **directed edge** in the room graph: a trigger condition plus what happens when the trigger fires.

```json
{
  "trigger": { "kind": "edge", "side": "E" },
  "destinationRoomId": "forest_01",
  "spawnPoint": { "x": 1, "y": 6 }
}
```

| Field | Type | Description |
|---|---|---|
| `trigger` | object | The condition that fires this exit. For MVP, only `kind: "edge"` exists. |
| `trigger.kind` | string | `"edge"` — the player walking off one of the room's four edges. (Reserved: `"region"` for tile-rectangle triggers like pits or warps.) |
| `trigger.side` | string | One of `"N"`, `"S"`, `"E"`, `"W"`. Which edge of the room triggers this exit. |
| `destinationRoomId` | string | The room the player jumps to. Must be a key in `rooms`. |
| `spawnPoint` | `{x, y}` | Tile coordinates in the *destination* room where the player appears. Should be on a walkable tile and not on top of the destination's matching exit edge (or the player will immediately re-trigger an exit). |

Exits are directional. Walking east from `house_01` to `forest_01` does not imply walking west from `forest_01` back to `house_01`. Symmetric connections require declaring both edges separately — one exit in `house_01.exits` for going east, and one exit in `forest_01.exits` for going west. This is intentional: it lets you express one-way passages, warps, and non-Euclidean connections if needed.

#### `pickupSpawns`: where collectibles appear in this room

```json
{ "id": "pickup_house01_antenna", "type": "phone_antenna", "x": 9, "y": 2 }
```

| Field | Type | Description |
|---|---|---|
| `id` | string | Globally unique identifier for this specific spawn point. Used to track depletion in the save's `collectedPickups`. Convention: `pickup_<roomId>_<typeName>[_<suffix>]` if a room has multiple of the same type (e.g., `pickup_forest01_candy_a`, `pickup_forest01_candy_b`). |
| `type` | string | Pickup-type ID. Must be a key in `pickupTypes`. |
| `x`, `y` | integers | Tile coordinates within this room where the pickup appears. Should be on a walkable tile. |

When a pickup is collected, the simulation:
1. Inserts the spawn's `id` into `collectedPickups` (so it stops rendering and can't be collected again).
2. Branches on the pickup's `type.kind`:
   - `phonePiece` → appends the `type` to `phonePiecesCollected`, runs the win-condition check.
   - `candy` → increments `candyCount`.

### Complete worked example

A small two-room world with a phone piece, a candy spawn, and a symmetric connection between rooms:

```json
{
  "schemaVersion": 1,
  "config": {
    "startingEnergy": 100,
    "energyDrainPerSecond": 0.5,
    "energyPerCandy": 25,
    "startRoomId": "house_01",
    "startSpawn": { "x": 2, "y": 2 },
    "requiredPhonePieces": ["phone_antenna"]
  },
  "pickupTypes": {
    "phone_antenna": { "kind": "phonePiece", "displayName": "Antenna" },
    "candy":         { "kind": "candy",      "displayName": "Reese's Pieces" }
  },
  "rooms": {
    "house_01": {
      "tileset": "house",
      "width": 10,
      "height": 6,
      "tiles": [
        "1111111111",
        "1000000001",
        "1000300001",
        "1000000000",
        "1000000001",
        "1111111111"
      ],
      "exits": [
        {
          "trigger": { "kind": "edge", "side": "E" },
          "destinationRoomId": "forest_01",
          "spawnPoint": { "x": 1, "y": 3 }
        }
      ],
      "pickupSpawns": [
        { "id": "pickup_house01_candy_a", "type": "candy", "x": 4, "y": 2 }
      ]
    },
    "forest_01": {
      "tileset": "forest",
      "width": 10,
      "height": 6,
      "tiles": [
        "1111111111",
        "0000000001",
        "0000000001",
        "0000005001",
        "0000000001",
        "1111111111"
      ],
      "exits": [
        {
          "trigger": { "kind": "edge", "side": "W" },
          "destinationRoomId": "house_01",
          "spawnPoint": { "x": 8, "y": 3 }
        }
      ],
      "pickupSpawns": [
        { "id": "pickup_forest01_antenna", "type": "phone_antenna", "x": 6, "y": 3 }
      ]
    }
  }
}
```

Reading the rooms visually:
- `house_01` is a 10×6 room with walls around the border, a carpet tile (`3`) near the spawn, and an opening on the east edge at row 3 (the `0` in the rightmost column of row 3).
- `forest_01` has no west wall (the leftmost column is all `0`s) so the player can enter freely from house_01. There's a special tile `5` (whatever the forest tileset defines as) where the phone piece sits.

The exits form a symmetric pair: walking east from house_01 lands at (1, 3) in forest_01, and walking west from forest_01 lands at (8, 3) in house_01. Both spawn points are deliberately placed away from the exit edge — otherwise the player would immediately re-trigger the exit and bounce back.

### Load-time validation

When loading `world.json`, the game state service should verify all of the following and fail with a clear error message on any failure:

1. `schemaVersion` exists and is supported.
2. `config.startRoomId` is a key in `rooms`.
3. `config.startSpawn` is within the bounds of `startRoomId` and on a walkable tile.
4. Every entry in `config.requiredPhonePieces` is a key in `pickupTypes` with `kind: "phonePiece"`.
5. Every entry in `config.requiredPhonePieces` has at least one `pickupSpawn` with that `type` somewhere in `rooms` (otherwise the game is unwinnable).
6. Every room's `tiles` array has exactly `height` rows, each exactly `width` characters long.
7. Every tile character in `tiles` is a known tile index in the room's tileset.
8. Every `exit.destinationRoomId` is a key in `rooms`.
9. Every `exit.spawnPoint` is within the bounds of its destination room and on a walkable tile.
10. Every `pickupSpawn.id` is globally unique across all rooms.
11. Every `pickupSpawn.type` is a key in `pickupTypes`.
12. Every `pickupSpawn.x, y` is within the room's bounds and on a walkable tile.
13. Every room is reachable from `config.startRoomId` (BFS following exits; flag any unreachable rooms).

Fail loudly at startup, not silently at runtime when a player walks into a broken room. A few hours spent on validation now saves dozens of hours of "why does ET disappear in this room" later.

### Future extensions

Things `world.json` does not currently support but could later:

- Region triggers (`trigger.kind: "region"`) for tile-rectangle exits — pits, teleporters, region-based scripted events.
- Per-room music or ambient sound IDs.
- Per-room background colors or atmospheric effects.
- NPCs or hazards with spawn positions and behaviors.

For MVP, none of this is needed. The schema is designed to extend additively — new optional fields can be added without breaking existing rooms.

## 7. Tileset format

Each tileset is a PNG atlas plus a JSON metadata file. The game state service reads the metadata for walkability; the Vulkan service reads it for rendering.

```
assets/tilesets/
  house.png
  house.json
  forest.png
  forest.json
```

```json
// house.json
{
  "tileSize": 16,
  "tilesPerRow": 8,
  "tiles": {
    "0": { "name": "floor",  "walkable": true  },
    "1": { "name": "wall",   "walkable": false },
    "2": { "name": "door",   "walkable": true  },
    "3": { "name": "carpet", "walkable": true  }
  }
}
```

Tileset names referenced in `world.json` (e.g., `"tileset": "house"`) must correspond to files in this directory. The contract here is shared between the game state service and the Vulkan service.

## 8. Coordinate translation (guidance for the Vulkan service)

The game state service sends positions in **per-room tile-local coordinates**: every position the renderer receives is implicitly "relative to the top-left of the current room." This is the natural representation for the simulation (see section 4), but the renderer typically wants a **global render-space coordinate system** so it can lay rooms out next to each other and move a camera around. This section explains how the renderer translates between the two.

This is entirely a Vulkan-side concern. The game state service does no translation, does not know about global coordinates, and does not care how the renderer arranges rooms visually. Everything described in this section happens inside the Vulkan service.

### The two coordinate spaces

**Room-local (tile) coordinates** — what comes over the WebSocket:
- Origin `(0, 0)` is the top-left of the *current* room.
- Units are tiles. Player positions can be fractional (e.g., `(5.5, 6.0)`); pickup positions are integers.
- Implicitly scoped to the room ID most recently announced via `room_loaded`.

**Global render coordinates** — what the renderer uses internally:
- Origin `(0, 0)` is wherever the renderer decided to anchor its world. A natural choice is "the first room loaded sits at `(0, 0)`."
- Units depend on the renderer. Could still be tiles (just bigger numbers), could be pixels, could be world units. Doesn't matter — pick one and stick with it.
- Each room has an `origin` in this space; tiles and pickups in that room are rendered relative to that origin.

The renderer maintains a map of room-id → global-origin. As new rooms are discovered (via `room_loaded`), it assigns each one an origin and remembers it.

### The translation math

Given a room with global origin `(roomOriginX, roomOriginY)` and a position in room-local coordinates `(localX, localY)`, the global coordinate is:

```
globalX = roomOriginX + localX
globalY = roomOriginY + localY
```

That's it. One addition per axis, per position, per frame. The same formula applies to the player position, pickup positions, and tile positions. To go the other direction (e.g., for a mouse click on a tile, if you ever need it):

```
localX = globalX - roomOriginX
localY = globalY - roomOriginY
```

### Maintaining the room-origin map

The renderer keeps a structure like:

```cpp
struct RoomLayout {
    Vec2 origin;       // global render coords of this room's top-left
    int width;         // in tile units (from room_loaded)
    int height;
    // ... GPU resource handles (vertex buffer, etc.) ...
};

std::unordered_map<RoomId, RoomLayout> roomLayouts;
RoomId currentRoomId;  // updated on every room_loaded
```

When `room_loaded` arrives, there are two cases:

**Case 1 — the room is already known.** The renderer has seen this room before, so its origin and tile data are already in `roomLayouts`. Just update `currentRoomId` and continue rendering. No GPU upload needed; nothing to compute.

**Case 2 — the room is new.** The renderer assigns it a global origin and uploads its tile data to GPU memory.

The first time `room_loaded` ever arrives (on initial connect), the message has no `arrivedFrom` field (the player is spawning, not arriving from anywhere). The renderer places this room at its anchor — typically `(0, 0)`:

```cpp
roomLayouts[room.id] = { .origin = Vec2(0, 0), .width = room.width, .height = room.height };
```

For subsequent new rooms, `arrivedFrom` indicates which edge of the *new* room the player entered through, which lets the renderer position the new room adjacent to the previous one in the natural direction. The mapping:

| `arrivedFrom` | The new room is on the ___ of the previous room | New room's origin |
|---|---|---|
| `"W"` | east (player walked east, entered through new room's west edge) | `(prev.origin.x + prev.width, prev.origin.y)` |
| `"E"` | west | `(prev.origin.x - new.width, prev.origin.y)` |
| `"N"` | south | `(prev.origin.x, prev.origin.y + prev.height)` |
| `"S"` | north | `(prev.origin.x, prev.origin.y - new.height)` |

In code:

```cpp
void onRoomLoaded(const RoomLoadedMessage& msg) {
    if (roomLayouts.find(msg.room.id) == roomLayouts.end()) {
        // New room — assign an origin
        Vec2 origin;
        if (msg.arrivedFrom.has_value()) {
            const RoomLayout& prev = roomLayouts.at(currentRoomId);
            switch (*msg.arrivedFrom) {
                case Direction::West:
                    origin = { prev.origin.x + prev.width, prev.origin.y };
                    break;
                case Direction::East:
                    origin = { prev.origin.x - msg.room.width, prev.origin.y };
                    break;
                case Direction::North:
                    origin = { prev.origin.x, prev.origin.y + prev.height };
                    break;
                case Direction::South:
                    origin = { prev.origin.x, prev.origin.y - msg.room.height };
                    break;
            }
        } else {
            // First room — anchor at origin
            origin = { 0, 0 };
        }
        roomLayouts[msg.room.id] = {
            .origin = origin,
            .width = msg.room.width,
            .height = msg.room.height,
            // ... upload tiles to GPU here ...
        };
    }
    currentRoomId = msg.room.id;
    // Translate player position to global coords for rendering:
    Vec2 playerGlobal = roomLayouts[currentRoomId].origin + msg.player;
    // ... etc ...
}
```

### Translating state updates

After the initial `room_loaded`, `state_update` messages carry positions in the current room's local space (the room set by the most recent `room_loaded`). To render, translate them through the current room's origin:

```cpp
void onStateUpdate(const StateUpdateMessage& msg) {
    const RoomLayout& room = roomLayouts.at(currentRoomId);
    Vec2 playerGlobal = room.origin + msg.player;
    // ... render ET at playerGlobal ...

    for (const auto& pickup : msg.activePickups) {
        Vec2 pickupGlobal = { room.origin.x + pickup.x, room.origin.y + pickup.y };
        // ... render pickup at pickupGlobal ...
    }
}
```

The renderer never stores positions in global coordinates — it always converts on the fly during rendering. This keeps things simple: the source of truth for positions is what came over the WebSocket (local), and global is just a computed view for rendering.

### Camera

The camera lives in global render space. Following the player means setting the camera's center to the player's global position each frame:

```cpp
Vec2 playerGlobal = roomLayouts[currentRoomId].origin + lastKnownPlayerLocal;
camera.center = playerGlobal;
```

Because rooms are placed adjacent to each other in global space, the camera moves smoothly into the next room as the player walks across an edge — assuming the game state service sends a `room_loaded` for the new room at the right moment. (In practice the transition is discrete: the player walks off the edge of room A, the server sends `room_loaded` for room B, and the camera snaps to the new player position in room B. If you want a smooth visual transition, that's a renderer-side animation between the old and new player global positions.)

### A worked example

Player starts in `house_01` (10 wide, 6 tall) at local `(2, 2)`. They walk east, cross into `forest_01` (10 wide, 6 tall) at local `(1, 3)`. Then walk south into `cave_01` (8 wide, 8 tall) at local `(4, 1)`.

Renderer-side state after each event:

```
After first room_loaded (house_01, no arrivedFrom):
  roomLayouts = { "house_01": { origin: (0, 0),  width: 10, height: 6 } }
  currentRoomId = "house_01"
  player global = (0, 0) + (2, 2) = (2, 2)

After room_loaded (forest_01, arrivedFrom: "W"):
  // forest_01 is east of house_01, origin = (0+10, 0) = (10, 0)
  roomLayouts = {
    "house_01":  { origin: (0, 0),   width: 10, height: 6 },
    "forest_01": { origin: (10, 0),  width: 10, height: 6 }
  }
  currentRoomId = "forest_01"
  player global = (10, 0) + (1, 3) = (11, 3)

After room_loaded (cave_01, arrivedFrom: "N"):
  // cave_01 is south of forest_01, origin = (10, 0+6) = (10, 6)
  roomLayouts = {
    "house_01":  { origin: (0, 0),   width: 10, height: 6 },
    "forest_01": { origin: (10, 0),  width: 10, height: 6 },
    "cave_01":   { origin: (10, 6),  width: 8,  height: 8 }
  }
  currentRoomId = "cave_01"
  player global = (10, 6) + (4, 1) = (14, 7)
```

If the player then walks back north into `forest_01`, the message arrives with `arrivedFrom: "S"`. But `forest_01` is already in `roomLayouts` — its origin stays `(10, 0)`. The renderer doesn't re-place known rooms; it just updates `currentRoomId` and resumes.

### Edge cases worth knowing about

**Non-Euclidean maps.** If your `world.json` ever has connections that don't lay out cleanly in 2D — e.g., room A's east exit goes to room B, but room B's west exit goes to room C — the renderer's "place new rooms adjacent" strategy will produce overlapping origins. For MVP this won't happen (your map is Euclidean), but if you ever add warps or pits, the renderer would need a different layout strategy (e.g., place non-adjacent rooms far apart and accept that the camera will jump on transition).

**Room already known but reached from a different direction.** The renderer should not reposition rooms it has already seen — first-seen position wins. This matches normal player movement: rooms have a fixed spatial relationship as the player discovers them.

**GPU resource caching.** Once a room's tiles are uploaded to GPU memory, keep them. With ~50 rooms total and tile data on the order of kilobytes per room, the total GPU footprint is trivial (well under a megabyte). Evicting and re-uploading on every room change is unnecessary work.

**Pickup positions.** Pickups in `activePickups` are room-local, just like the player. Translate them the same way: `globalPickup = currentRoom.origin + pickup.{x, y}`. When a pickup is collected, the next `state_update` simply omits it from `activePickups` — the renderer stops drawing it.

### Summary

The renderer's job, in one paragraph: maintain a map from `RoomId` to global render origin. On `room_loaded`, if the room is new, compute its origin from the `arrivedFrom` hint and the previous room's origin/dimensions, then upload its tiles to GPU. On every render frame, translate the most recent player position by the current room's origin to get the global position; render ET there; do the same for active pickups; center the camera on the player.

## 9. WebSocket message protocol

**Connection:** `ws://localhost:8001/game?userId={name}&mode={new|continue}`

- `userId` is the name the player entered on the start screen.
- `mode` indicates the player's choice: `continue` loads the existing save; `new` discards any existing save and starts fresh (used after "New Game" is selected from the start screen).

The game state service uses both parameters on connect: if `mode=continue`, it `GET`s the save from the auth service; if `mode=new`, it `DELETE`s any existing save and starts from `world.config` defaults.

All messages are JSON. Every message has a `type` field that determines its shape.

### Client → Server messages

**`input_move`** — Player movement input
```json
{
  "type": "input_move",
  "direction": "N" | "S" | "E" | "W",
  "pressed": true
}
```
Sent on keydown and keyup (`pressed: false` on release). The game state service applies movement at its own tick rate.

**`input_consume_candy`** — Player clicked the consume-candy button
```json
{
  "type": "input_consume_candy"
}
```

**`input_save`** — Player triggered manual save
```json
{
  "type": "input_save"
}
```

### Server → Client messages

**`room_loaded`** — Sent on initial connect and on every room transition. Carries everything the client needs to render the new room.
```json
{
  "type": "room_loaded",
  "room": {
    "id": "house_01",
    "tileset": "house",
    "width": 20,
    "height": 12,
    "tiles": ["11111111111111111111", "..."],
    "activePickups": [
      { "id": "pickup_house01_candy_a", "type": "candy", "x": 14, "y": 8 }
    ]
  },
  "player": { "x": 5.0, "y": 6.0 },
  "arrivedFrom": "W"
}
```
`activePickups` only includes pickups that haven't been collected yet — the client doesn't need to know about depleted ones.

`arrivedFrom` is an optional hint indicating which edge of the *new* room the player entered through (`"N"`, `"S"`, `"E"`, `"W"`). It's omitted on the very first `room_loaded` after connect (since the player isn't arriving from anywhere — they're spawning). The renderer uses this to position the new room relative to the previous one in its global render space; see section 8 for details.

**`state_update`** — Sent whenever dynamic state changes (player moves, pickup collected, candy consumed, energy ticks down meaningfully, etc.)
```json
{
  "type": "state_update",
  "player": { "x": 5.5, "y": 6.0 },
  "energy": 73,
  "phonePieceCount": 1,
  "candyCount": 2,
  "activePickups": [
    { "id": "pickup_house01_candy_a", "type": "candy", "x": 14, "y": 8 }
  ]
}
```
Energy is sent as an integer or low-precision float; the client doesn't need 6 decimal places. Reasonable to send `state_update` at ~10Hz during play to keep the HUD smooth without flooding the wire.

**`game_over`** — Win or lose condition triggered
```json
{
  "type": "game_over",
  "outcome": "won" | "lost"
}
```
On `won`, the client plays the ending animation. On `lost`, the client shows the lose screen. Game state service stops sending state updates after this.

**`save_ack`** — Confirmation that a manual save was queued (optional, but nice for UX)
```json
{
  "type": "save_ack",
  "savedAt": "2026-05-13T..."
}
```

**`error`** — Something went wrong (failed to load save, unknown message type, etc.)
```json
{
  "type": "error",
  "message": "Failed to load save from auth service"
}
```

### Message sending policy

- The game state service is event-driven: it sends `state_update` only when state has meaningfully changed, *plus* a periodic update (every ~1 second) for energy so the HUD doesn't lag.
- Movement updates are sent every game tick if the player is moving (~30Hz is plenty for a tile-based game).
- The client should not extrapolate or simulate locally between updates. Render exactly what was last received.

### Disconnect / reconnect

- If the WebSocket drops, the game state service discards the in-memory state.
- On reconnect, the client opens a new WebSocket; the game state service reloads from Mongo and starts a fresh session from the last save. Any unsaved progress since the last save event is lost (this is expected — autosaves are frequent enough to make this rare).

## 10. Save event payload (game state → RabbitMQ)

Published to the `saves` queue. The save service consumes these and upserts to Mongo.

```json
{
  "userId": "player1",
  "savedAt": "2026-05-13T...",
  "trigger": "room_transition" | "pickup" | "candy_consumed" | "manual" | "win" | "lose",
  "state": {
    "player": { "currentRoomId": "forest_01", "localX": 8.5, "localY": 4.0 },
    "energy": 73,
    "phonePiecesCollected": ["phone_antenna"],
    "candyCount": 2,
    "collectedPickups": ["pickup_house01_antenna", "pickup_forest01_candy_a"],
    "status": "playing"
  }
}
```

The `trigger` field is informational (useful for debugging/logging) but doesn't change how the save service writes.

**Queue configuration:**
- Queue name: `saves`
- Durable: `true`
- Message delivery mode: `2` (persistent)
- This ensures messages survive a RabbitMQ restart.

## 11. Autosave triggers

The game state service publishes a save event on:

- Room transition (player crossed into a new room)
- Pickup collected (phone piece or candy)
- Candy consumed (energy went up)
- Status change (win or lose)
- Manual save request from client

Autosave is fire-and-forget — never blocks the game loop. Manual saves send a `save_ack` to the client after queueing.

## 12. HTTP endpoints (auth/save service)

The save service exposes HTTP endpoints for the read path (loading saves) and for the startup-flow user lookup. The write path goes through RabbitMQ (see section 13).

**`GET /save/{userId}`** — Returns the latest save for a user.
- 200: Save document JSON
- 404: No save exists (game state service starts fresh)

**`GET /save/{userId}/exists`** — Lightweight existence check, used by the start screen to decide whether to offer "Continue" or only "New Game."
- 200: `{ "exists": true, "savedAt": "2026-05-13T...", "status": "playing" }`
- 200: `{ "exists": false }` (no save found — note: still 200, the lookup succeeded)

**`DELETE /save/{userId}`** — Removes a user's save. Used when the player selects "New Game" with an existing save, or for a hard reset.
- 204: Save deleted
- 404: No save existed

### Startup flow

1. Player launches the Vulkan client.
2. Client shows a name entry screen; player enters a name.
3. Client calls `GET /save/{name}/exists` against the auth service.
4. Based on the response:
   - If `exists: false`: client offers only "Start New Game."
   - If `exists: true`: client offers "Continue" (showing `savedAt`) and "New Game."
5. Player selects an option. Client opens the WebSocket with the appropriate `mode` parameter:
   - "Continue" → `ws://localhost:8001/game?userId={name}&mode=continue`
   - "New Game" → `ws://localhost:8001/game?userId={name}&mode=new`
6. Game state service handles the connection accordingly:
   - `mode=continue`: `GET /save/{userId}` from auth service, initialize session from save.
   - `mode=new`: `DELETE /save/{userId}` (ignore 404), initialize session from `world.config` defaults.
7. Game state service sends initial `room_loaded` over WebSocket. Play begins.

The Vulkan client talks directly to the auth service over HTTP only for these startup-flow endpoints. All gameplay communication goes through the game state service via WebSocket.

## 13. RabbitMQ — save event queue

The save event queue is the write path for saves. The game state service publishes save events as a fire-and-forget side effect of gameplay; the auth/save service consumes them and upserts to Mongo.

### Why a queue here

- Saves are asynchronous by nature. The game doesn't need an immediate ack to keep playing — it just keeps simulating.
- Durability: if the auth service is briefly down or slow, messages buffer in the queue and get processed when it recovers. No save events are lost.
- Decoupling: the game state service never directly talks to Mongo or the auth service for writes. It only publishes to the queue.
- Back-pressure: if save events arrive faster than the auth service can write them, the queue absorbs the burst.

### Queue configuration

- **Exchange:** default (direct) exchange
- **Queue name:** `saves`
- **Queue properties:** `durable=true` (queue survives broker restart)
- **Message properties:** `delivery_mode=2` (persistent — messages survive broker restart)
- **Consumer ack mode:** manual ack after successful Mongo write (so a crash mid-write redelivers the message)

These settings ensure no save events are lost across broker restarts or consumer crashes.

### Producer (game state service)

The game state service maintains one long-lived AMQP connection and channel to RabbitMQ. On each save trigger, it:

1. Serializes the current `GameSession` to the save event JSON (see section 10).
2. Publishes to the `saves` queue with `delivery_mode=2`.
3. Returns immediately — the game loop does not block.

If the AMQP connection drops, the game state service reconnects in the background. Save events generated during a disconnect are buffered in-process (a bounded queue) and flushed on reconnect. If the in-process buffer overflows (extremely unlikely at this scale), the oldest events are dropped in favor of the newest — losing an old save event is acceptable because newer events overwrite older state anyway.

### Consumer (auth/save service)

The auth/save service runs a RabbitMQ consumer alongside the FastAPI HTTP server, in the same process via asyncio. On each delivered message:

1. Parse the message JSON.
2. Validate (`userId` is a non-empty string, `savedAt` is a parseable timestamp, `state` has required fields).
3. Run the Mongo upsert with the `savedAt` guard:
   ```python
   result = await db.saves.update_one(
       { "userId": userId, "$or": [
           { "savedAt": { "$lt": incoming_savedAt } },
           { "savedAt": { "$exists": False } }
       ]},
       { "$set": { ...incoming_fields, "savedAt": incoming_savedAt } },
       upsert=True
   )
   ```
4. Ack the message on success.
5. On Mongo failure: nack with requeue, so the message is retried.
6. On validation failure: ack and log (don't retry malformed messages — they'll just keep failing).

The `savedAt` guard prevents stale messages (redelivered after the player has already moved further) from clobbering newer state. Without it, retried out-of-order messages could roll back progress.

### Ordering and concurrency notes

- With one consumer instance, messages for a given user are processed in delivery order. Out-of-order is rare but possible under redelivery, which is why the `savedAt` guard exists.
- If you scale to multiple consumers later, the `savedAt` guard becomes essential — two consumers might process events for the same user concurrently, and the guard ensures only the newest one wins.
- For MVP (single consumer, one player at a time), the guard is belt-and-suspenders, but cheap to include.

### Health and observability

For demoing and debugging:

- RabbitMQ's management UI runs on `http://localhost:15672` (default credentials `guest:guest` if you don't configure otherwise). Useful for inspecting queue depth, consumer status, and message rates.
- The consumer should log every successfully processed save with `userId` and `trigger`. This makes it easy to verify save events are flowing during a demo.

## 14. Docker Compose layout

```
project/
├── docker-compose.yml
├── auth-service/
│   ├── Dockerfile
│   ├── requirements.txt
│   └── app/
│       ├── main.py
│       ├── consumer.py
│       ├── models.py
│       └── db.py
├── game-service/
│   ├── Dockerfile
│   ├── CMakeLists.txt
│   ├── world.json
│   └── src/
├── vulkan-service/
│   ├── CMakeLists.txt
│   ├── assets/tilesets/
│   └── src/
└── ARCHITECTURE.md  (this doc)
```

Docker Compose orchestrates `mongo`, `rabbitmq`, `auth-service`, and `game-service`. The Vulkan service likely runs on the host (GPU passthrough into Docker is fiddly) and connects to `ws://localhost:8001`.

## 15. Build order

1. Mongo + Docker Compose skeleton — get `docker-compose up` running Mongo; insert a fake save by hand.
2. Auth/Save service HTTP read — FastAPI app with `GET /save/{userId}` against Mongo.
3. Auth/Save service queue consumer — RabbitMQ in Compose, consumer that upserts to Mongo. Test by publishing manually.
4. Game state service skeleton — loads `world.json`, HTTP-calls auth service. No gameplay yet.
5. Game state service WebSocket server — accepts connections, sends `room_loaded` on connect. Test with `wscat`.
6. Game state service game loop — input handling, movement, collision, exits, pickups, candy, energy, win/lose, save event publishing.
7. Vulkan service minimal renderer — window, Vulkan setup, render one tile.
8. Vulkan service WebSocket integration — connect, render state, send inputs, HUD.
9. Polish, more rooms, ending animation, manual save UI.

Steps 1–6 give you a fully testable backend without any rendering — important fallback if Vulkan eats too much time.

## 16. Open questions / things to align on as a team

- **Tile size and screen resolution** — what's the target window size? Tiles per screen? This affects Vulkan setup and room dimensions in `world.json`.
- **GUI/HUD framework** — is the Vulkan person using ImGui (common, easy) or hand-rolling UI? Affects how the candy-consume button gets implemented.
- **ET sprite specs** — animated frames? One static sprite? Facing direction needed?
- **Initial room count** — how many rooms in the MVP map? Three or four is plenty to demonstrate transitions.
- **Energy drain rate** — `energyDrainPerSecond` value needs tuning, but pick a starting point (0.5/sec → 200 seconds of play from full).

These don't block starting work but should be decided in the first week.
