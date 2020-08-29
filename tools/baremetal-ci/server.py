#!/usr/bin/env python3
import argparse
import os
from pymongo import MongoClient
from fastapi import FastAPI

MONGO_URI = os.environ.get("MONGO_URI", "mongodb://localhost:27017")
MONGO_DB_NAME = os.environ.get("MONGO_DB_NAME", "baremental-ci")

app = FastAPI()
db = MongoClient(MONGO_URI)[MONGO_DB_NAME]

@app.get("/api/hello")
def home():
    return {"hello": str(db.posts.insert_one({ "asd": 123 }).inserted_id) }

@app.get("/api/runs")
def runs_list():
    for x in db.posts.find():
      print(x)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", reload=True)
