from fastapi import APIRouter
from fastapi import HTTPException

from app.services.save_service import SaveService

router = APIRouter()


@router.get("/get_game_state/{user_id}")
async def get_game_state(user_id: str):
    save = await SaveService.get_save(user_id)

    if not save:
        raise HTTPException(
            status_code=404,
            detail="Save not found"
        )

    return save


@router.delete("/delete_save/{user_id}")
async def delete_save(user_id: str):
    deleted_count = await SaveService.delete_save(
        user_id
    )

    if deleted_count == 0:
        raise HTTPException(
            status_code=404,
            detail="Save not found"
        )

    return {
        "message": "Save deleted"
    }