import sys
import pathlib
import json
import PIL.Image
import uuid
import dnn
import numpy as np
import pickle

from cgi import parse_qs
from io import BytesIO

STATIC_DIR = pathlib.Path("static")
PAINTINGS_DIR = pathlib.Path("data/paintings/")
EMBEDDINGS_DIR = pathlib.Path("data/embeddings/")
PREVIEWS_DIR = pathlib.Path("data/previews/")

MODEL_PATH = "models/model_big_30_30_predict.h5"

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

#MODEL_PATH = "models/model.h5"
#MODEL_PATH = "model_big_30_30.h5"

# train_model = dnn.create_and_train_model()
# model = dnn.create_model(train_model)
# model.save(MODEL_PATH)


model = dnn.load(MODEL_PATH)

def gen_json_ans(obj):
    return "200 OK", JSON_HEADERS, json.dumps(obj)

def gen_png_ans(obj):
    return "200 OK", IMG_HEADERS, obj


def parse_png_bytes(body):
    try:
        painting = PIL.Image.open(BytesIO(body))
    except Exception as e:
        raise Exception("can't parse image bytes", e)

    if painting.format != "PNG" or painting.mode != "RGB" or painting.width != 128 or painting.height != 128:
        raise Exception(f"image should be in .png format in RGB mode and be 128x128px, but is {painting.format} {painting.mode} {painting.width} {painting.height}")

    return painting

def post_replica(query, body):
    try:
        painting_id = str(uuid.UUID(query["id"][0]))
    except Exception as e:
        print("painting 'id' param not specified or is invalid:", e)
        return gen_json_ans({"error": "painting 'id' param not specified or is invalid"})

    try:
        replica = parse_png_bytes(body)
    except Exception as e:
        print("can't parse replica image:", e)
        return gen_json_ans({"error": str(e)})

    embedding_file_name = painting_id + ".emb"
    try:
        with open(EMBEDDINGS_DIR / embedding_file_name, "rb") as file:
            painting_emb = pickle.load(file)
    except Exception as e:
        print("can't remember painting:", e)
        return gen_json_ans({"error": "can't remember painting"})

    replica_emb = model.predict(np.array([np.array(replica)]))
    dist = np.linalg.norm(painting_emb - replica_emb)

    if dist >= 0.05:
        return gen_json_ans({"dist": dist})
    else:
        return gen_json_ans("HERE IS YOUR FLAG")

def put_painting(query, body):
    try:
        painting = parse_png_bytes(body)
    except Exception as e:
        print("can't parse painting image:", e)
        return gen_json_ans({"error": str(e)})

    painting_id = str(uuid.uuid4())

    embedding_file_name = painting_id + ".emb"
    try:
        emb = model.predict(np.array([np.array(painting)]))
        with open(EMBEDDINGS_DIR / embedding_file_name, "xb") as file:
            pickle.dump(emb, file)
    except Exception as e:
        print("can't memorize painting:", e)
        return gen_json_ans({"error": "can't memorize painting"})

    image_file_name = painting_id + ".png"
    try:
        with open(PAINTINGS_DIR / image_file_name, "xb") as file:
            file.write(body)
    except Exception as e:
        print("can't save painting:", e)
        return gen_json_ans({"error": "can't save painting"})

    preview = painting.resize((16, 16), resample=PIL.Image.BICUBIC)
    try:
        preview.save(PREVIEWS_DIR / image_file_name)
    except Exception as e:
        print("can't save preview:", e)
        return gen_json_ans({"error": "can't save preview"})

    return gen_json_ans({"id": painting_id})


def get_paintings(query, body):
    paintings_ids = []

    for x in PREVIEWS_DIR.iterdir():
        if(len(paintings_ids) >= 10_000):
            break
        if x.is_file() and x.suffix == '.png':
            paintings_ids.append(x.stem)

    return gen_json_ans(paintings_ids)

def get_preview(query, body):
    try:
        file_name = str(uuid.UUID(query["id"][0])) + ".png"
    except Exception as e:
        print("painting 'id' param not specified or is invalid:", e)
        return gen_json_ans({"error": "preview 'id' param not specified or is invalid"})

    try:
        with open(PREVIEWS_DIR / file_name, "rb") as file:
            return gen_png_ans(file.read())
    except Exception as e:
        print("can't read preview:", e)
        return gen_json_ans({"error": "can't read preview"})

URLS = {
    ("GET", "/"): "index.html",
    ("GET", "/index.html"): "index.html",
    ("GET", "/favicon.ico"): "favicon.ico",
    ("PUT", "/painting"): put_painting,
    ("GET", "/paintings"): get_paintings,
    ("GET", "/preview"): get_preview,
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

    if isinstance(data, str):
        return [data.encode()]
    else:
        return [data]
