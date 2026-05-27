import json
import asyncio
import aio_pika

from app.config import (
    RABBITMQ_URL,
    SAVE_QUEUE
)

from app.models import SaveEventModel
from app.services.save_service import SaveService

async def consume_save_events():
    connection = None
    while connection is None:
        try:
            print("Connecting to RabbitMQ...")

            connection = await aio_pika.connect_robust(
                RABBITMQ_URL
            )

            print("Connected to RabbitMQ")
        except Exception as e:
            print(
                f"RabbitMQ connection failed: {e}"
            )

            print("Retrying in 5 seconds...")

            await asyncio.sleep(5)

    channel = await connection.channel()

    queue = await channel.declare_queue(
        SAVE_QUEUE,
        durable=True
    )

    await queue.purge() # In case you want to keep old messages, get rid of this

    print(
        f"Listening for messages on "
        f"queue: {SAVE_QUEUE}"
    )

    async with queue.iterator() as queue_iter:
        async for message in queue_iter:
            async with message.process():
                try:
                    body = message.body.decode()
                    data = json.loads(body)
                    validated = SaveEventModel(**data)

                    await SaveService.upsert_save(
                        validated.dict()
                    )

                    print(
                        f"Saved data for "
                        f"{validated.userId}"
                    )

                except Exception as e:
                    print(
                        f"Consumer error: {e}"
                    )