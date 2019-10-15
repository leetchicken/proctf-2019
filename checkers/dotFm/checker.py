#!/usr/bin/env python3
from api import Api
from chklib import Checker, Verdict, \
    CheckRequest, PutRequest, GetRequest, utils

from playlist_lib.playlist_creator import create_playlist_file
from playlist_lib import track_creator
from unpacking_tools import unpack
import eyed3
import sys
import hashlib
import traceback
import os.path
import random


track_creator.SEGMENTS_PATH = os.path.abspath(os.curdir) + "/music_files/"

checker = Checker()


@checker.define_check
async def check_service(request: CheckRequest) -> Verdict:
    async with Api(request.hostname) as api:
        data = utils.generate_random_text()
        with create_playlist_file(data) as playlist:
            try:
                downloaded_json = (await api.upload_playlist(playlist.playlist_name))
                if "created" not in downloaded_json:
                    return Verdict.MUMBLE("Bad json", "'created' not in answer")
                music_id = downloaded_json["created"]
            except Exception as e:
                return Verdict.DOWN(str(e), traceback.format_exc())
            music_hashes = set(playlist.music_hashes)
            image_hashes = set(playlist.image_hashes)

        downloaded_music = []
        for i, _ in enumerate(music_hashes):
            try:
                music = await api.download_music(music_id, i)
            except Exception as e:
                return Verdict.DOWN(str(e), traceback.format_exc())

            if hashlib.sha1(music).hexdigest() not in music_hashes:
                return Verdict.MUMBLE("Broken format", "Digests mismatch!")
            downloaded_music.append(music)

    return Verdict.OK()


@checker.define_put(vuln_num=1, vuln_rate=1)
async def put_flag_into_the_service(request: PutRequest) -> Verdict:
    async with Api(request.hostname) as api:
        with create_playlist_file(request.flag) as playlist:
            try:
                downloaded_json = (await api.upload_playlist(playlist.playlist_name))
                if "created" not in downloaded_json:
                    return Verdict.MUMBLE("Bad json", "'created' not in answer")
                music_id = downloaded_json["created"]
            except Exception as e:
                return Verdict.DOWN(str(e), traceback.format_exc())
            music_hashes = set(playlist.music_hashes)
            image_hashes = set(playlist.image_hashes)

    return Verdict.OK(f'{music_id}:{":".join(music_hashes)}:{":".join(image_hashes)}')


@checker.define_get(vuln_num=1)
async def get_flag_from_the_service(request: GetRequest) -> Verdict:
    music_id, *hashes = request.flag_id.split(":")
    music_hashes = hashes[:2]
    images_hashes = hashes[2:4]
    async with Api(request.hostname) as api:

        downloaded_music = []
        for i in range(len(music_hashes)):
            try:
                track_id = random.choice(range(i, 100, 2))
                music = await api.download_music(music_id, track_id)
            except Exception as e:
                return Verdict.DOWN("Couldn't connect to api", traceback.format_exc())

            if hashlib.sha1(music).hexdigest() not in music_hashes:
                return Verdict.MUMBLE("Broken track", "Digests mismatch!")
            downloaded_music.append(music)

    with unpack() as session:
        for num, music in enumerate(downloaded_music):
            with open(f"{session}/{music_id}_{num}.mp3", mode="wb") as mus:
                mus.write(music)

            music = eyed3.load(f"{session}/{music_id}_{num}.mp3")

            flag = get_comment_from_music(music)
            if flag is None:
                return Verdict.CORRUPT("Couldn't extract flag from track!", "last stage worked bad")
            if flag != request.flag:
                return Verdict.CORRUPT("Bad flag found!", f"bad flags, hmm: {flag}")

    return Verdict.OK()


def get_comment_from_music(music):
    try:
        return music.tag.frame_set[b"TXXX"][0].data.split(b"\x00")[1].decode()
    except Exception as e:
        print(e, traceback.format_exc(), file=sys.stderr)
        return None


if __name__ == '__main__':
    checker.run()

