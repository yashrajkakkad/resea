#!/usr/bin/env python3
import argparse
import os
from pymongo import MongoClient
from gridfs import GridFS
from fastapi import FastAPI, Depends, WebSocket, WebSocketDisconnect, \
    HTTPException, status, File, UploadFile, Form
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

def encode_build(obj):
    return {
        "id": str(obj["_id"]),
        **{k: obj[k] for k in ["title", "machine", "created_by"]}
    }

#
#  API Endpoints
#

@app.get("/api/builds")
def list_builds():
    return {
        "builds": list(db.builds.find()),
    }

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
    })
    return encode_build(db.builds.find_one(result.inserted_id))

@app.get("/api/builds/{id}")
def get_build(id: int):
    return raise_404_if_none(encode_build(db.builds.find_one(id)))

@app.get("/api/runners")
def list_runners():
    for x in db.posts.find():
      print(x)

@app.put("/api/runners/{id}")
def register_or_update_runner(id: int, cred = Depends(authenticate)):
    pass # TODO:

@app.get("/api/runs")
def list_runs():
    for x in db.posts.find():
      print(x)

@app.put("/api/runs")
def create_run(cred = Depends(authenticate)):
    pass # TODO:

@app.put("/api/runs/{id}")
def update_run(id: int, cred = Depends(authenticate)):
    pass # TODO:

@app.get("/api/runs/{id}")
def get_run(id: int):
    pass # TODO:

@app.get("/api/runs/{id}/log")
def get_run_log(id: int):
    pass # TODO:

@app.post("/api/runs/{id}/log")
def update_run_log(id: int, cred = Depends(authenticate)):
    pass # TODO:

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", reload=True)
