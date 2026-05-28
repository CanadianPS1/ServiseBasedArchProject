import sys
import time
import json
import asyncio
from contextlib import asynccontextmanager

import websockets

SERVER_URL = "ws://localhost:8001/game"

@asynccontextmanager
async def session(user_id="darryl", mode="new"):
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

        assert msg["room"]["id"] == "forest_1", f"expected forest_1, got {msg['room']['id']}"
        assert "arrivedFrom" not in msg, "initial room_loaded should not have arrivedFrom"
        assert msg["player"]["x"] == 5 and msg["player"]["y"] == 5, \
            f"unexpected spawn position: {msg['player']}"

        print(f"  PASS: spawned at ({msg['player']['x']}, {msg['player']['y']})")
        print(f"  PASS: activePickups = {len(msg['room']['activePickups'])} items")

async def test_walk_north_to_tophole():
    print("\n=== Test: walk north to topHole_2 ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        tophole_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))

        assert tophole_msg["arrivedFrom"] == "S", f"expected arrivedFrom=S, got {tophole_msg.get('arrivedFrom')}"
        print(f"  PASS: transitioned to topHole_2, arrivedFrom={tophole_msg['arrivedFrom']}, "
              f"spawn=({tophole_msg['player']['x']}, {tophole_msg['player']['y']})")

async def test_walk_north_then_back():
    print("\n=== Test: walk north, then south ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        tophole_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))

        assert tophole_msg["arrivedFrom"] == "S", f"expected arrivedFrom=S, got {tophole_msg.get('arrivedFrom')}"
        print(f"  PASS: transitioned to topHole_2, arrivedFrom={tophole_msg['arrivedFrom']}")

        await asyncio.sleep(0.1)

        await ws.send(json.dumps({"type": "input_move", "direction": "S", "pressed": True}))
        forest_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "forest_1",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "S", "pressed": False}))

        assert forest_msg["arrivedFrom"] == "N", f"expected arrivedFrom=N, got {forest_msg.get('arrivedFrom')}"
        print(f"  PASS: transitioned to forest_1, arrivedFrom={forest_msg['arrivedFrom']}")

async def test_pickup_collection():
    print("\n=== Test: pickup collection (antenna in topHole_2) ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        tophole_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )
        initial_pickup_count = len(tophole_msg["room"]["activePickups"])

        await asyncio.sleep(1.0)
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))

        last = await drain_recv(ws, 0.3)
        if last is None or last["type"] != "state_update":
            print(f"  WARN: last message was {last}")
            return

        pieces = last.get("phonePieceCount", 0)
        pickups_remaining = len(last.get("activePickups", []))

        if pieces > 0:
            print(f"  PASS: phonePieceCount={pieces}, activePickups={pickups_remaining} (was {initial_pickup_count})")
        else:
            print(f"  FAIL: phonePieceCount={pieces} (expected >0)")
            print(f"    last player position: {last['player']}")


async def test_consume_candy():
    print("\n=== Test: consume candy (candy in topHole_2) ===")
    async with session() as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )
        await asyncio.sleep(1.0)
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))

        before = await drain_recv(ws, 0.3)
        candy_before = before.get("candyCount", 0) if before else 0
        if candy_before <= 0:
            print(f"  WARN: no candy collected (candyCount={candy_before}); skipping consume check")
            return

        await ws.send(json.dumps({"type": "input_consume_candy"}))
        last = await drain_recv(ws, 0.3)
        candy = last.get("candyCount", -1)
        energy = last.get("energy", -1)

        print(f"  After consume: candyCount={candy}, energy={energy:.2f}")
        if candy == candy_before - 1:
            print(f"  PASS: candy consumed ({candy_before} -> {candy})")
        else:
            print(f"  FAIL: expected {candy_before - 1}, got {candy}")

async def test_rejection():
    print("\n=== Test: rejection of second client ===")
    async with session(user_id="alice") as ws1:
        await recv_until(ws1, lambda m: m["type"] == "room_loaded")

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
                  f"Make sure energyDrainPerSecond is high (50+) to test this.")

async def test_collision_blocks_into_wall():
    print("\n=== Test: collision blocks walking into wall ===")
    async with session(user_id="collision_test_1") as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))

        await move(ws, "E", 0.5)
        last_state = await drain_recv(ws, 0.3)

        final_x = last_state["player"]["x"]
        assert final_x < 1.0, f"Expected player blocked before x=1.0, got x={final_x}"
        print(f"  PASS: player blocked at x={final_x:.4f} (couldn't enter hole)")

async def test_collision_slides_along_wall():
    print("\n=== Test: collision slides along wall ===")
    async with session(user_id="collision_test_2") as ws:
        await recv_until(ws, lambda m: m["type"] == "room_loaded")

        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": True}))
        tophole_msg = await recv_until(
            ws,
            lambda m: m["type"] == "room_loaded" and m["room"]["id"] == "topHole_2",
            timeout=5.0
        )

        initial_y = tophole_msg["player"]["y"]

        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": True}))
        await asyncio.sleep(0.3)
        await ws.send(json.dumps({"type": "input_move", "direction": "N", "pressed": False}))
        await ws.send(json.dumps({"type": "input_move", "direction": "E", "pressed": False}))

        last_state = await drain_recv(ws, 0.3)

        final_x = last_state["player"]["x"]
        final_y = last_state["player"]["y"]
        assert final_x < 1.0, f"East should be blocked, but x={final_x}"
        assert final_y < initial_y, f"North should still work, but y stayed at {final_y} (initial {initial_y})"
        print(f"  PASS: player slid along wall: x={final_x:.4f}, y={final_y:.4f} (initial y={initial_y})")

ALL_TESTS = [
    test_initial_room_load,
    test_walk_north_to_tophole,
    test_walk_north_then_back,
    test_pickup_collection,
    test_consume_candy,
    test_rejection,
    test_loss_by_energy,
    test_collision_blocks_into_wall,
    test_collision_slides_along_wall,
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
