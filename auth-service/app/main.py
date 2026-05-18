import asyncio
from fastapi import FastAPI
from app.routes.saves import router
from app.consumer import consume_save_events

app = FastAPI()

app.include_router(router)

@app.get("/")
async def root():

    return {
        "message": "Auth Save Service Running"
    }

@app.on_event("startup")
async def startup_event():

    asyncio.create_task(
        consume_save_events()
    )