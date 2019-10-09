import track_creator
import image_creator
import random
from uuid import uuid4
import os
import shutil

PLAYLIST_DESCRIPTOR_HEADER = "#EXTM3U"
PLAYLIST_INFO_HEADER = "#EXTINF"


def create_playlist_file(flag=""):
    amount_of_tracks = 2
    flags = [flag[:16], flag[16:]]
    tags = []
    for i in range(amount_of_tracks):
        tags.append({
            "album": f"0x{uuid4().hex[:8]}",
            "artist": f"CPU_0x{uuid4().hex[:8]}",
            "comment": f"{flags[i] if flag else '(c)' + str(random.randint(1900, 2019))}",
            "title": f"0x_0x{uuid4().hex[:8]}"
        })

    playlist_dir = uuid4()
    os.mkdir(str(playlist_dir))
    images = [image_creator.generate_image(f"{playlist_dir}/{uuid4()}.png") for _ in range(amount_of_tracks)]
    music = [track_creator.create_track(f"{playlist_dir}/track{i + 1}.mp3", images[i], tags[i])
             for i in range(amount_of_tracks)]

    for image in images:
        os.remove(image)

    with open(f"{playlist_dir}/playlist.m3u", mode="w") as f:
        f.write(
            f"{PLAYLIST_DESCRIPTOR_HEADER}\n\n")

        for i, tag in enumerate(tags):
            f.write(f"{PLAYLIST_INFO_HEADER}:42, {tag['artist']} – {tag['album']}\n")
            f.write(music[i].split('/')[1] + "\n\n")

    shutil.make_archive(f"{playlist_dir}", "zip", str(playlist_dir))
    shutil.rmtree(str(playlist_dir))
    return f"{playlist_dir}.zip"
