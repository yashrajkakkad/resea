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
from fastapi.responses import Response
from fastapi.encoders import jsonable_encoder
from pydantic import BaseModel
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
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
    keys = ["runner_id", "log"]
    date_keys = ["created_at"]
    return encode(obj, keys, date_keys)

#
#  API Endpoints
#

@app.get("/api/builds")
def list_builds(newer_than: Optional[float] = None):
    if newer_than:
        print(datetime.fromtimestamp(newer_than))
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
    print("Created", datetime.utcnow())
    return encode_build(db.builds.find_one({ "_id": ObjectId(result.inserted_id) }))

@app.get("/api/builds/{id}")
def get_build(id: str):
    return encode_build(raise_404_if_none(db.builds.find_one({ "_id": ObjectId(id) })))

@app.get("/api/builds/{id}/runs")
def list_runs_for_build(id: str):
    build = raise_404_if_none(db.builds.find_one({ "_id": ObjectId(id) }))
    return list(map(encode_run, db.runs.find({ "build_id": build["_id"] })))

@app.get("/api/builds/{id}/image")
def get_image(id: str):
    build = raise_404_if_none(db.builds.find_one({ "_id": ObjectId(id) }))
    file = fs.get(build["image_file_id"])
    return Response(file.read(), media_type="application/octet-stream")

@app.get("/api/runners")
def list_runners():
    for x in db.posts.find():
      print(x)

@app.put("/api/runners/{id}")
def register_or_update_runner(id: str, cred = Depends(authenticate)):
    pass # TODO:

@app.get("/api/runs")
def list_runs():
    return list(map(encode_run, db.runs.find()))

@app.put("/api/runs")
def create_run(cred = Depends(authenticate)):
    pass # TODO:

@app.put("/api/runs/{id}")
def update_run(id: str, cred = Depends(authenticate)):
    pass # TODO:

@app.get("/api/runs/{id}")
def get_run(id: str):
    pass # TODO:

@app.get("/api/runs/{id}/log")
def get_run_log(id: str):
    pass # TODO:

@app.post("/api/runs/{id}/log")
def update_run_log(id: str, cred = Depends(authenticate)):
    pass # TODO:

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", reload=True)
