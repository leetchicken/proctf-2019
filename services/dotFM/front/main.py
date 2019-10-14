from sanic import Sanic
from sanic.request import Request
from sanic.response import json, text, file_stream
import glob
import aiohttp

app = Sanic(__name__)
data = {}


async def upload(session: aiohttp.ClientSession, body_data):
    async with session.post("http://uploader:80/playlist", data=body_data) as result:
        if result.status != 200:
            return {"created": None}
        else:
            return await result.json(content_type="")


async def find(session: aiohttp.ClientSession, guid):
    async with session.get(f"http://uploader:80/playlist?playlist_id={guid}") as result:
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


@app.get("/image")
async def get_music_image(request: Request):
    args = dict(request.query_args)
    artist, album = args.get("artist", None), args.get("album", None)


# noinspection PyUnresolvedReferences
@app.get("/channel")
async def get_channel(request: Request):
    args = dict(request.query_args)
    playlist_id = args.get("id")
    if playlist_id is None:
        return text("No playlists found", 404)

    track_number = int(args.get("num", 0))

    result: dict = await find(app.aio_http_session, playlist_id)
    if len(result["tracks"]) == 0:
        return text("No tracks were found!", 404)

    files = [x for x in glob.glob("/app/storage/music/**")
             if x.endswith(result["tracks"][track_number % len(result["tracks"])])]
    if len(files) == 0:
        return text("Track has been deleted!", 404)

    return await file_stream(files[0])


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
