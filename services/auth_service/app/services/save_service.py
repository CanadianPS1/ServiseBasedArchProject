from app.db import db

class SaveService:
    @staticmethod
    async def get_save(user_id: str):

        save = await db.saves.find_one(
            {"userId": user_id},
            {"_id": 0}
        )

        return save

    @staticmethod
    async def save_exists(user_id: str):

        save = await db.saves.find_one(
            {"userId": user_id},
            {
                "_id": 0,
                "savedAt": 1,
                "status": 1
            }
        )

        return save

    @staticmethod
    async def delete_save(user_id: str):

        result = await db.saves.delete_one(
            {"userId": user_id}
        )

        return result.deleted_count

    @staticmethod
    async def upsert_save(save_event: dict):
        user_id = save_event["userId"]
        saved_at = save_event["savedAt"]
        state = save_event["state"]

        await db.saves.update_one(
            {
                "userId": user_id
            },
            {
                "$set": {
                    "userId": user_id,
                    "savedAt": saved_at,
                    "schemaVersion": 1,

                    "player": state["player"],
                    "energy": state["energy"],

                    "phonePiecesCollected":
                        state["phonePiecesCollected"],
                    "candyCount":
                        state["candyCount"],
                    "collectedPickups":
                        state["collectedPickups"],
                    "status":
                        state["status"]
                }
            },
            upsert=True
        )