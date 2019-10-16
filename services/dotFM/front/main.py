from sanic import Sanic
from sanic.request import Request
from sanic.response import json, text, file_stream
import uuid
import glob
import random
import aiohttp


app = Sanic(__name__)
app_root = "/app/storage"
data = {}


async def upload(session: aiohttp.ClientSession, body_data):
    async with session.post("http://uploader/playlist", data=body_data) as result:
        if result.status != 200:
            return {"created": None}
        else:
            return await result.json(content_type="")


async def find(session: aiohttp.ClientSession, guid):
    async with session.get(f"http://uploader/playlist?playlist_id={guid}") as result:
        if result.status != 200:
            return {"tracks": []}
        else:
            return await result.json(content_type="")


@app.listener('before_server_start')
async def init(application, loop):
    application.aio_http_session = aiohttp.ClientSession(loop=loop)


@app.listener('after_server_stop')
async def finish(application, loop):
    loop.run_until_complete(application.aio_http_session.close())
    loop.close()


# noinspection PyUnresolvedReferences
@app.post("/channel")
async def create_channel(request: Request):
    x, y = map(int, request.headers.get("Position", "0,0").split(","))
    some_data = request.body

    result = await upload(app.aio_http_session, some_data)
    if "created" in result:
        data[(x, y)] = result["created"]
    return json(result)


@app.get("/image/<image_name:string>")
async def get_music_image(_, image_name: str):
    try:
        return await file_stream(get_files_lookup("images", f"/{image_name}.png")[0])
    except:
        return text("Not found!", 404)


@app.get("/lookup")
async def lookup_for_points(request: Request):
    rad = 40
    x, y = map(int, request.headers.get("Position", "0,0").split(","))
    found = [data[(x1, y1)] for x1, y1 in data.keys() if ((x - x1) ** 2 + (y - y1) ** 2) <= rad ** 2]
    return json(found, 200)


@app.get("/images")
async def get_random_images(request):
    return json((x.split("/")[-1] for x in random.choices(get_files_lookup("images", ".png"), k=10)))


# noinspection PyUnresolvedReferences
@app.get("/channel")
async def get_channel(request: Request):
    args = dict(request.query_args)
    given_id = args.get("id")
    if given_id is None:
        return text("No playlists found", 404)

    try:
        playlist_id = uuid.UUID(given_id, version=4)
    except ValueError:
        return text("Invalid UUID provided!")

    track_number = abs(int(args.get("num", 0)))

    result: dict = await find(app.aio_http_session, playlist_id)
    if len(result["tracks"]) == 0:
        return text("No tracks were found!", 404)

    circuited_number = track_number % len(result["tracks"])

    try:
        return await file_stream(get_files_lookup("music", f"{result['tracks'][circuited_number]}.mp3")[0])
    except:
        return text("Track has been deleted!", 404)


def get_files_lookup(folder: str, ending_rule: str):
    return [x for x in glob.glob(f"{app_root}/{folder}/**")
            if x.endswith(ending_rule)]


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
