#!/usr/bin/env python3
import argparse
from datetime import datetime
import os
from typing import Optional
from pymongo import MongoClient
from bson.objectid import ObjectId
from gridfs import GridFS
from fastapi import FastAPI, Depends, WebSocket, WebSocketDisconnect, \
    HTTPException, status, File, UploadFile, Form
from fastapi.requests import Request
from fastapi.responses import Response
from fastapi.encoders import jsonable_encoder
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from pydantic import BaseModel
from hmac import compare_digest
from logging import getLogger

API_KEY = os.environ["API_KEY"]
MONGO_URI = os.environ.get("MONGO_URI", "mongodb://localhost:27017")
MONGO_DB_NAME = os.environ.get("MONGO_DB_NAME", "baremental-ci")

app = FastAPI()
logger = getLogger("baremetal-ci")
db = MongoClient(MONGO_URI)[MONGO_DB_NAME]
fs = GridFS(db)

def authenticate(cred: HTTPAuthorizationCredentials = Depends(HTTPBearer())):
    if not compare_digest(API_KEY, cred.credentials):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid API Key",
            headers={ "WWW-Authenticate": "Bearer" },
        )

def raise_404_if_none(value):
    if value is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="No such a record"
        )
    return value

def encode(obj, keys, date_keys):
    return {
        "id": str(obj["_id"]),
        **{k: obj[k] for k in keys},
        **{k: obj[k].timestamp() for k in date_keys},
    }

def encode_build(obj):
    keys = ["title", "machine", "created_by"]
    date_keys = ["created_at"]
    return encode(obj, keys, date_keys)

def encode_run(obj):
    keys = ["status", "runner_name", "build_id", "duration"]
    date_keys = ["created_at"]
    return encode(obj, keys, date_keys)

def encode_runner(obj):
    keys = ["name", "machine"]
    date_keys = ["updated_at"]
    return encode(obj, keys, date_keys)

#
#  API Endpoints
#

@app.get("/api/builds")
def list_builds(newer_than: Optional[float] = None):
    if newer_than:
        query = { "created_at": { "$gt": datetime.fromtimestamp(newer_than) } }
    else:
        query = {}
    return list(map(encode_build, db.builds.find(query)))

@app.post("/api/builds")
def create_build(
    cred = Depends(authenticate),
    title = Form(...), machine = Form(...), created_by = Form(...),
    image: UploadFile = File("image")
):
    file_id = fs.put(image.file)
    result = db.builds.insert_one({
        "title": title,
        "machine": machine,
        "created_by": created_by,
        "image_file_id": file_id,
        "created_at": datetime.utcnow(),
    })
    return encode_build(db.builds.find_one({ "_id": ObjectId(result.inserted_id) }))

@app.get("/api/builds/{id}")
def get_build(id: str):
    return encode_build(raise_404_if_none(db.builds.find_one({ "_id": ObjectId(id) })))

@app.get("/api/builds/{id}/runs")
def list_runs_for_build(id: str):
    return list(map(encode_run, db.runs.find({ "build_id": id })))

@app.get("/api/builds/{id}/image")
def get_image(id: str):
    build = raise_404_if_none(db.builds.find_one({ "_id": ObjectId(id) }))
    file = fs.get(build["image_file_id"])
    return Response(file.read(), media_type="application/octet-stream")

class Run(BaseModel):
    status: str
    runner_name: str
    build_id: str
    duration: Optional[int]

@app.get("/api/runs")
def list_runs():
    return list(map(encode_run, db.runs.find()))

@app.post("/api/runs")
def create_run(run: Run, cred = Depends(authenticate)):
    result = db.runs.insert_one({
        **dict(run),
        **{ "created_at": datetime.utcnow() },
    })
    db.logs.insert_one({
        "run_id": str(result.inserted_id),
        "text": "",
        "updated_at": datetime.utcnow(),
    })
    return encode_run(db.runs.find_one({ "_id": ObjectId(result.inserted_id) }))

@app.put("/api/runs/{id}")
async def update_run(id: str, request: Request, cred = Depends(authenticate)):
    run = await request.json()
    db.runs.update_one({ "_id": ObjectId(id) }, { "$set": run })

@app.get("/api/runs/{id}")
def get_run(id: str):
    return encode_run(raise_404_if_none(db.runs.find_one({ "_id": ObjectId(id) })))

@app.get("/api/runs/{id}/log")
def get_run_log(id: str):
    log = raise_404_if_none(db.logs.find_one({ "run_id": id }))
    return Response(log["text"], media_type="plain/text")

class Log(BaseModel):
    text: str

@app.put("/api/runs/{id}/log")
def update_run_log(id: str, log: Log, cred = Depends(authenticate)):
    db.logs.update_one({ "run_id": id }, {
        "$set": {
            **dict(log),
            **{ "updated_at": datetime.utcnow() },
        }
    }, upsert=True)

@app.get("/api/runners")
def list_runners():
    return list(map(encode_runner, db.runners.find()))

class NewRunner(BaseModel):
    name: str
    machine: str

@app.put("/api/runners/{name}")
def register_or_update_runner(name: str, runner: NewRunner, cred = Depends(authenticate)):
    db.runners.update_one({ "name": name }, {
        "$set": {
            **dict(runner),
            **{ "updated_at": datetime.utcnow() },
        }
    }, upsert=True)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", reload=True)
