#!/usr/bin/env python3
import argparse
import pymongo
from fastapi import FastAPI

app = FastAPI()

@app.get('/')
def home():
    return {'hello': 'worwld2'}

if __name__ == '__main__':
    import uvicorn
    uvicorn.run("server:app", reload=True)
