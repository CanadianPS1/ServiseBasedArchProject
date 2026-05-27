import sys
import time
import json
import asyncio
from contextlib import asynccontextmanager

import websockets

SERVER_URL = "ws://localhost:8001/game"

@asynccontextmanager
async def session(user_id="alice", mode="new"):
    uri = f"{SERVER_URL}?userId={user_id}&mode={mode}"
    async with websockets.connect(uri) as ws:
        yield ws

async def recv_until(ws, predicate, timeout=5.0):
    deadline = time.monotonic() + timeout
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise asyncio.TimeoutError(f"No matching message within {timeout}s")
        raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
        msg = json.loads(raw)
        if predicate(msg):
            return msg

async def move(ws, direction, duration):
    await ws.send(json.dumps({
        "type": "input_move",
        "direction": direction,
        "pressed": True
    }))
    
    await asyncio.sleep(duration)
    await ws.send(json.dumps({
        "type": "input_move",
        "direction": direction,
        "pressed": False
    }))

async def drain_recv(ws, duration):
    deadline = time.monotonic() + duration
    last_msg = None

    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break

        try:
            raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
            last_msg = json.loads(raw)
        
        except asyncio.TimeoutError:
            break

    return last_msg

async def test_initial_room_load():
    print("\n=== Test: initial room_loaded ===")
    async with session() as ws:
        msg = await recv_until(ws, lambda m: m["type"] == "room_loaded")

        assert msg["room"]["id"] == "house_01", f"expected house_01, got {msg['room']['id']}"
        assert "arrivedFrom" not in msg, "initial room_loaded should not have arrivedFrom"
        assert msg["player"]["x"] == 2 and msg["player"]["y"] == 2, \
            f"unexpected spawn position: {msg['player']}"
        
        print(f"  PASS: spawned at ({msg['player']['x']}, {msg['player']['y']})")
        print(f"  PASS: activePickups = {len(msg['room']['activePickups'])} items")

async def test_walk_east_to_forest():
    print("\n=== Test: walk east to forest ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await move(ws, "E", 3.0)

        last = await drain_recv(ws, 0.5)
        print(f"  Last state: room? {last.get('type')}, player={last.get('player')}")

async def test_walk_east_then_back():
    print("\n=== Test: walk east, then west ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": True}))
        forest_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "forest_01",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": False}))

        assert forest_msg["arrivedFrom"] == "W", f"expected arrivedFrom=W, got {forest_msg.get('arrivedFrom')}"
        print(f"  PASS: transitioned to forest_01, arrivedFrom={forest_msg['arrivedFrom']}, "
              f"spawn=({forest_msg['player']['x']}, {forest_msg['player']['y']})")

        await asyncio.sleep(0.1)

        await ws.send(json.dumps({"type": "input_move", "direction": "W", "pressed": True}))
        house_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "house_01",
            timeout=5.0
        )
    
        await ws.send(json.dumps({"type": "input_move", "direction": "W", "pressed": False}))

        assert house_msg["arrivedFrom"] == "E", f"expected arrivedFrom=E, got {house_msg.get('arrivedFrom')}"
        print(f"  PASS: transitioned to house_01, arrivedFrom={house_msg['arrivedFrom']}, "
              f"spawn=({house_msg['player']['x']}, {house_msg['player']['y']})")

async def test_wall_clamping():
    """Walk south (no exit on house_01 south edge) — should clamp."""
    print("\n=== Test: wall clamping (south edge) ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await move(ws, "S", 5.0)

        last = await drain_recv(ws, 0.2)
        y = last["player"]["y"]

        assert 5.9 < y < 6.0, f"expected y clamped near 6.0, got {y}"
        print(f"  PASS: player clamped at y={y:.4f}")

async def test_pickup_collection():
    """Walk to a pickup and verify it's collected."""
    print("\n=== Test: pickup collection (candy at 4,4) ===")
    async with session() as ws:
        initial = await recv_until(ws, lambda m: m["type"] == "room_loaded")
        initial_pickup_count = len(initial["room"]["activePickups"])

        # Player spawns at (2, 2). Candy is at (4, 4). Move E 2 tiles, then S 2 tiles.
        # E 2 tiles at 4 tiles/sec = 0.5s; S 2 tiles = 0.5s.
        # Add a little margin to make sure we land on the tile.
        await move(ws, "E", 0.55)
        await asyncio.sleep(0.05)
        await move(ws, "S", 0.55)

        last = await drain_recv(ws, 0.3)
        if last is None or last["type"] != "state_update":
            print(f"  WARN: last message was {last}")
            return

        candy = last.get("candyCount", 0)
        pickups_remaining = len(last.get("activePickups", []))

        if candy > 0:
            print(f"  PASS: candyCount={candy}, activePickups={pickups_remaining} (was {initial_pickup_count})")
        else:
            print(f"  FAIL: candyCount={candy} (expected >0)")
            print(f"    last player position: {last['player']}")

async def test_consume_candy():
    print("\n=== Test: consume candy ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await move(ws, "E", 0.55)
        await asyncio.sleep(0.05)
        await move(ws, "S", 0.55)
        await drain_recv(ws, 0.3)

        await ws.send(json.dumps({"type": "input_consume_candy"}))

        last = await drain_recv(ws, 0.3)
        candy = last.get("candyCount", -1)
        energy = last.get("energy", -1)

        print(f"  After consume: candyCount={candy}, energy={energy:.2f}")
        if candy == 0:
            print(f"  PASS: candy consumed")
        else:
            print(f"  FAIL: candy still {candy}")

async def test_rejection():
    print("\n=== Test: rejection of second client ===")
    async with session(user_id="alice") as ws1:
        await recv_until(ws1, lambda m: m["type"] == "room_loaded")

        # Try opening a second connection
        try:
            uri = f"{SERVER_URL}?userId=bob&mode=new"
            async with websockets.connect(uri) as ws2:
                try:
                    msg = await asyncio.wait_for(ws2.recv(), timeout=2.0)
                    print(f"  FAIL: second connection accepted, received {msg}")
                except asyncio.TimeoutError:
                    print(f"  ??? second connection opened but no message")
        except websockets.exceptions.ConnectionClosedError as e:
            print(f"  PASS: second connection rejected (code={e.code}, reason='{e.reason}')")

async def test_loss_by_energy():
    print("\n=== Test: lose by energy depletion ===")

    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        try:
            msg = await recv_until(
                ws,
                lambda m: m["type"] == "game_over",
                timeout=15.0
            )
            
            if msg["outcome"] == "lost":
                print(f"  PASS: game_over outcome=lost")
            else:
                print(f"  FAIL: expected outcome=lost, got {msg['outcome']}")

        except asyncio.TimeoutError:
            print(f"  TIMEOUT: no game_over within 15s. "
                  f"Make sure energyDrainPerSecond is high (50+) in test_world.json")

async def test_win_by_collecting_pieces():
    print("\n=== Test: win by collecting all phone pieces ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        # House_01: walk to antenna at (6, 2). Spawn is (2, 2). Move E 4 tiles.
        await move(ws, "E", 1.05)
        await drain_recv(ws, 0.3)

        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": True}))
        await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "forest_01",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": False}))

        # Forest_01: spawn at (1, 3). Dial at (5, 2), Speaker at (7, 3).
        # Walk E 4 tiles, then N 1 tile to land on (5, 2).
        await move(ws, "E", 1.0)
        await move(ws, "N", 0.25)
        await drain_recv(ws, 0.3)

        # Walk S 1 tile, E 2 tiles to land on (7, 3).
        await move(ws, "S", 0.25)
        await move(ws, "E", 0.5)
        await drain_recv(ws, 0.3)

        try:
            msg = await recv_until(ws, lambda m: m["type"] == "game_over", timeout=3.0)
            if msg["outcome"] == "won":
                print(f"  PASS: game_over outcome=won")
            else:
                print(f"  FAIL: expected outcome=won, got {msg['outcome']}")

        except asyncio.TimeoutError:
            print(f"  TIMEOUT: no game_over after collecting pieces")

            last = await drain_recv(ws, 0.2)
            print(f"    last state: {last}")

ALL_TESTS = [
    test_initial_room_load,
    test_walk_east_to_forest,
    test_walk_east_then_back,
    test_wall_clamping,
    test_pickup_collection,
    test_consume_candy,
    test_rejection,
    test_loss_by_energy,
    test_win_by_collecting_pieces
]

async def main():
    if len(sys.argv) > 1:
        name = sys.argv[1]
        test = next((t for t in ALL_TESTS if t.__name__ == name), None)
        if test is None:
            print(f"Unknown test: {name}")
            print(f"Available: {', '.join(t.__name__ for t in ALL_TESTS)}")
            sys.exit(1)

        await test()

    else:
        for test in ALL_TESTS:
            try:
                await test()

            except Exception as e:
                print(f"  ERROR in {test.__name__}: {type(e).__name__}: {e}")
            
            await asyncio.sleep(0.5)

if __name__ == "__main__":
    asyncio.run(main())
