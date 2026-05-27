from motor.motor_asyncio import AsyncIOMotorClient

from app.config import (
    MONGO_URI,
    DATABASE_NAME
)

client = AsyncIOMotorClient(MONGO_URI)

db = client[DATABASE_NAME]


async def create_indexes():

    await db.saves.create_index(
        "userId",
        unique=True
    )

    print("MongoDB indexes created")