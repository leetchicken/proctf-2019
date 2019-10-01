import sys
import pathlib
import json
import PIL.Image
import gallery
import uuid
from cgi import parse_qs

STATIC_DIR = pathlib.Path("static")
PAINTINGS_DIR = pathlib.Path("data/paintings/")
PREVIEWS_DIR = pathlib.Path("data/previews/")

DYNAMIC_HEADERS = [
    ("Content-Type", "application/json; charset=utf8"),
    ("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate"),
    ("Expires", "0"),
    ("Pragma", "no-cache"),
]

JSON_HEADERS = [
    ("Content-Type", "application/json; charset=utf8"),
]

HTML_HEADERS = [
    ("Content-Type", "text/html; charset=utf8"),
]

IMG_HEADERS = [
    ("Content-Type", "image/png"),
    ("Cache-Control", "max-age=86400"),
]


def gen_json_ans(obj):
    return "200 OK", JSON_HEADERS, json.dumps(obj)


def parse_png(body):
    try:
        painting = PIL.Image.fromarray(body)
    except:
        raise ValueError(gen_json_ans({"error": "can't parse image bytes"}))
    
    if painting.format != "PNG" or painting.width != 128 or painting.height != 128:
        raise ValueError(gen_json_ans({"error": "image should be in .png format and be 128x128px"}))

    return painting

def post_replica(query, body):
    try:
        replica = parse_png(body)
    except ValueError as e:
        return str(e)    
    try:
        painting_id = str(uuid.UUID(query["id"]))
    except:
        return gen_json_ans({"error": "painting 'id' param not specified"})

    try:
        painting = PIL.Image.fromarray((PAINTINGS_DIR / painting_id+".png").read_bytes())
    except:
        return gen_json_ans({"error": "painting 'id' param not specified"})

    dist = gallery.calc_dist(painting, replica)
    if dist > 0.05:
        return gen_json_ans({"dist": dist})
    else:
        return "HERE IS YOUR FLAG"

def put_painting(query, body):
    try:
        painting = parse_png(body)
    except ValueError as e:
        return str(e)

    painting_id = str(uuid.uuid4())
    try:
        with open(PAINTINGS_DIR / painting_id, "x") as file:
            file.write(body)
    except OSError:
        return gen_json_ans({"error": "can't save painting"})

    return gen_json_ans({"id": painting_id})


def get_paintings(query, body):
    paintings_list = []

    for x in PAINTINGS_DIR.iterdir():
        if(len(paintings_list) >= 10_000):
            break
        if x.is_file() and x.suffix == '.png':
            paintings_list.append(x.stem)

    return gen_json_ans(paintings_list)


URLS = {
    ("GET", "/"): "index.html",
    ("GET", "/index.html"): "index.html",
    ("GET", "/favicon.ico"): "favicon.ico",
    ("PUT", "/painting"): put_painting,
    ("GET", "/paintings"): get_paintings,
    ("POST", "/replica"): post_replica,
}

def get_request_query(environ):
    try:
        query = parse_qs(environ["QUERY_STRING"])
    except:
        return {}
    if not query:
        return {}
    return query

def get_request_body(environ):
    try:
        request_body_size = int(environ.get('CONTENT_LENGTH', 0))
    except ValueError:
        request_body_size = 0
    request_body = environ['wsgi.input'].read(request_body_size)
    if not request_body:
        return b""
    return request_body


def application(environ, start_response):
    method = environ["REQUEST_METHOD"]
    url = environ["PATH_INFO"]
    query = parse_qs(environ["QUERY_STRING"])

    if (method, url) not in URLS:
        start_response("404 NOT FOUND", HTML_HEADERS)
        return [b"404"]

    handler = URLS[(method, url)]
    if not callable(handler):
        if url.endswith((".png", ".ico")):
            start_response("200 OK", IMG_HEADERS)
        else:
            start_response("200 OK", HTML_HEADERS)
        return [(STATIC_DIR / handler).read_bytes()]

    status, headers, data = handler(get_request_query(environ), get_request_body(environ))
    start_response(status, headers)
    return [data.encode()]
