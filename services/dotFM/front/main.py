from sanic import Sanic
from sanic.request import Request
from sanic.response import json, text
import glob
import uuid
import aiohttp

app = Sanic(__name__)
data = {}


@app.post("/channel")
async def create_channel(request: Request):
    x = request.args.get("x", 0)
    y = request.args.get("y", 0)
    some_data = request.body
    print(len(some_data))
    data[(x, y)] = str(uuid.uuid4())
    return text(data)


@app.get("/channel")
async def get_channel(request):
    # todo get channel guid from db (by freq + name)
    return json({"status": glob.glob("/var/app", recursive=True)}, 404)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
