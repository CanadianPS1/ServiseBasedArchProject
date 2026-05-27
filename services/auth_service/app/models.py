from pydantic import BaseModel
from typing import List


class PlayerModel(BaseModel):
    currentRoomId: str
    localX: float
    localY: float


class SaveStateModel(BaseModel):
    player: PlayerModel
    energy: float
    phonePiecesCollected: List[str]
    candyCount: int
    collectedPickups: List[str]
    status: str


class SaveEventModel(BaseModel):
    userId: str
    savedAt: str
    trigger: str
    state: SaveStateModel