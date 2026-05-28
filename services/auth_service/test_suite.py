import json
import time

import pika
import requests

import asyncio
from contextlib import asynccontextmanager


# CONFIG
BASE_URL = "http://localhost/auth"

RABBITMQ_HOST = "localhost"
QUEUE_NAME = "saves"

TEST_PLAYER_ID = "player1"


# RABBITMQ HELPER
def publish_test_player():
    """
    Publishes a fake save-state message into RabbitMQ
    so the auth-service can consume and store it.
    Payload must match SaveEventModel schema.
    """

    credentials = pika.PlainCredentials("guest", "guest")

    connection = pika.BlockingConnection(
        pika.ConnectionParameters(
            host=RABBITMQ_HOST,
            credentials=credentials
        )
    )

    channel = connection.channel()

    channel.queue_declare(
        queue=QUEUE_NAME,
        durable=True
    )

    # Must match SaveEventModel: userId, savedAt, trigger, state
    # state must match SaveStateModel: player, energy, phonePiecesCollected,
    #                                  candyCount, collectedPickups, status
    # player must match PlayerModel: currentRoomId, localX, localY
    payload = {
        "userId": TEST_PLAYER_ID,
        "savedAt": "2026-05-25T14:00:00",
        "trigger": "manual",
        "state": {
            "player": {
                "currentRoomId": "house_01",
                "localX": 5.0,
                "localY": 6.0
            },
            "energy": 75.0,
            "phonePiecesCollected": [],
            "candyCount": 2,
            "collectedPickups": [],
            "status": "playing"
        }
    }

    channel.basic_publish(
        exchange="",
        routing_key=QUEUE_NAME,
        body=json.dumps(payload),
        properties=pika.BasicProperties(
            delivery_mode=2
        )
    )

    connection.close()

    print("  Published test player save event")


# TESTS
def test_service_reachable():
    print("\n=== Test: service reachable ===")

    response = requests.get(f"{BASE_URL}/docs")

    if response.status_code == 200:
        print("  PASS: auth-service reachable through gateway")
    else:
        print(f"  FAIL: expected 200, got {response.status_code}")


def test_existing_player():
    print("\n=== Test: existing player ===")

    publish_test_player()

    # Give consumer time to save into Mongo
    time.sleep(2)

    response = requests.get(
        f"{BASE_URL}/get_game_state/{TEST_PLAYER_ID}"
    )

    if response.status_code == 200:
        print("  PASS: existing player returned 200")
        print(response.json())
    else:
        print(f"  FAIL: expected 200, got {response.status_code}")


def test_nonexistent_player():
    print("\n=== Test: nonexistent player ===")

    response = requests.get(
        f"{BASE_URL}/get_game_state/does_not_exist"
    )

    if response.status_code == 404:
        print("  PASS: nonexistent player correctly returned 404")
    else:
        print(f"  FAIL: expected 404, got {response.status_code}")


def test_player_state_fields():
    print("\n=== Test: player state fields ===")

    response = requests.get(
        f"{BASE_URL}/get_game_state/{TEST_PLAYER_ID}"
    )

    if response.status_code != 200:
        print(f"  FAIL: expected 200, got {response.status_code}")
        return

    data = response.json()

    # The service returns fields flat (no 'gameState' wrapper)
    # matching the upsert structure in save_service.py
    required_fields = [
        "userId",
        "savedAt",
        "player",
        "energy",
        "phonePiecesCollected",
        "candyCount",
        "collectedPickups",
        "status"
    ]

    missing = [f for f in required_fields if f not in data]

    if missing:
        print(f"  FAIL: missing fields {missing}")
        return

    player = data["player"]
    player_fields = ["currentRoomId", "localX", "localY"]
    missing_player = [f for f in player_fields if f not in player]

    if missing_player:
        print(f"  FAIL: missing player fields {missing_player}")
    else:
        print("  PASS: all expected fields exist")


def test_multiple_rapid_requests():
    print("\n=== Test: multiple rapid requests ===")

    success = 0

    for _ in range(10):
        response = requests.get(
            f"{BASE_URL}/get_game_state/{TEST_PLAYER_ID}"
        )

        if response.status_code == 200:
            success += 1

    if success == 10:
        print("  PASS: all rapid requests succeeded")
    else:
        print(f"  FAIL: only {success}/10 requests succeeded")


def test_invalid_route():
    print("\n=== Test: invalid route ===")

    response = requests.get(
        f"{BASE_URL}/this_route_does_not_exist"
    )

    if response.status_code == 404:
        print("  PASS: invalid route correctly returned 404")
    else:
        print(f"  FAIL: expected 404, got {response.status_code}")


def test_delete_save():
    print("\n=== Test: delete save ===")

    response = requests.delete(
        f"{BASE_URL}/delete_save/{TEST_PLAYER_ID}"
    )

    if response.status_code == 200:
        print("  PASS: save deleted successfully")
    else:
        print(f"  FAIL: expected 200, got {response.status_code}")

    # Confirm it's gone
    response = requests.get(
        f"{BASE_URL}/get_game_state/{TEST_PLAYER_ID}"
    )

    if response.status_code == 404:
        print("  PASS: deleted save correctly returns 404")
    else:
        print(f"  FAIL: expected 404 after delete, got {response.status_code}")

ALL_TESTS = [
    test_service_reachable,
    test_existing_player,
    test_nonexistent_player,
    test_player_state_fields,
    test_multiple_rapid_requests,
    test_invalid_route,
    test_delete_save
]

# MAIN
async def main():
    for test in ALL_TESTS:
        test()

if __name__ == "__main__":
    asyncio.run(main())