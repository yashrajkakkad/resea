#!/usr/bin/env python3
import argparse
import os
from pymongo import MongoClient
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

MONGO_URI = os.environ.get("MONGO_URI", "mongodb://localhost:27017")
MONGO_DB_NAME = os.environ.get("MONGO_DB_NAME", "baremental-ci")

app = FastAPI()
db = MongoClient(MONGO_URI)[MONGO_DB_NAME]

@app.get("/api/builds")
def list_builds():
    for x in db.posts.find():
      print(x)

@app.websocket("/api/builds/changes")
async def watch_for_new_builds(con: WebSocket):
    pass # TODO:

@app.post("/api/builds")
def create_build():
    pass # TODO:

@app.get("/api/builds/{id}")
def get_build(id: int):
    pass # TODO:


@app.get("/api/runners")
def list_runners():
    for x in db.posts.find():
      print(x)

@app.put("/api/runners/{id}")
def register_or_update_runner(id: int):
    pass # TODO:

@app.get("/api/runs")
def list_runs():
    for x in db.posts.find():
      print(x)

@app.put("/api/runs/{id}")
def update_run(id: int):
    pass # TODO:

@app.get("/api/runs/{id}")
def get_run(id: int):
    pass # TODO:

@app.get("/api/runs/{id}/log")
def get_run_log(id: int):
    pass # TODO:

@app.post("/api/runs/{id}/log")
def update_run_log(id: int):
    pass # TODO:

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", reload=True)
