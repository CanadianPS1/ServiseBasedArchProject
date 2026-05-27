import asyncio
from fastapi import FastAPI
from app.routes.saves import router
from app.consumer import consume_save_events
from app.db import create_indexes

app = FastAPI()
app.include_router(router)

@app.get("/")
async def root():

    return {
        "message": "Auth Save Service Running"
    }

@app.on_event("startup")
async def startup_event():

    await create_indexes()

    asyncio.create_task(
        consume_save_events()
    )