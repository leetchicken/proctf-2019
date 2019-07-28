from sanic import Sanic
from sanic.response import json
import motor

client = motor.MotorClient("mongo://mongo")
app = Sanic(__name__)


@app.route('/')
async def test(request):
    return json({'hello': 'world'})


@app.post("/channel")
async def create_channel():
    # todo create channel in db
    # scenarios: by name, merge, by content, etc (guid + guid == new channel)
    return json({"status": "bad content"}, 400)


@app.get("/channel")
async def get_channel():
    # todo get channel guid from db (by freq + name)
    return json({"status": "not found"}, 404)


@app.put("/channel")
async def put_audio_file():
    # todo put new audio content
    return json({"status": "too big or another shit"}, 400)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
