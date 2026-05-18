from fastapi import APIRouter
from fastapi import HTTPException

from app.services.save_service import SaveService

router = APIRouter()


@router.get("/save/{user_id}")
async def get_save(user_id: str):
    save = await SaveService.get_save(user_id)

    if not save:
        raise HTTPException(
            status_code=404,
            detail="Save not found"
        )

    return save


@router.get("/save/{user_id}/exists")
async def save_exists(user_id: str):
    save = await SaveService.save_exists(user_id)

    if not save:
        return {
            "exists": False
        }

    return {
        "exists": True,
        "savedAt": save["savedAt"],
        "status": save["status"]
    }


@router.delete("/save/{user_id}")
async def delete_save(user_id: str):
    deleted_count = await SaveService.delete_save(user_id)

    if deleted_count == 0:
        raise HTTPException(
            status_code=404,
            detail="Save not found"
        )

    return {
        "message": "Save deleted"
    }