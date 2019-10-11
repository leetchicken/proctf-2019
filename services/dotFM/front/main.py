from sanic import Sanic
from sanic.response import json
from response_helpers.helpers import ok, bad_request
from uuid import UUID
from db_operations.db_ops import Storage
import motor.motor_asyncio


app = Sanic(__name__)


@app.listener('before_server_start')
async def setup_db(application, loop):
    application.db = motor.motor_asyncio.AsyncIOMotorClient("localhost", 27017)


@app.route('/')
async def test(request):
    return json({'hello': 'world'})


@app.post("/channel")
async def create_channel(request):
    storage_client = Storage(request.app.db)
    # todo create channel in db
    parsed_request = request.json
    channel = "channel"
    if channel not in parsed_request:
        return bad_request(f"'{channel}' property not found in json")

    try:
        parsed_channel_uuid = UUID(parsed_request[channel])
    except ValueError as e:
        return bad_request(str(e))

    insert_id = await storage_client.add_new_channel(parsed_channel_uuid)

    # scenarios: by name, merge, by content, etc (guid + guid == new channel)
    return ok(f'Inserted document with {insert_id} id.')


@app.get("/channel")
async def get_channel(request):
    # todo get channel guid from db (by freq + name)
    return json({"status": "not found"}, 404)


@app.put("/channel")
async def put_audio_file(request):
    # todo put new audio content
    return json({"status": "too big or another shit"}, 400)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
