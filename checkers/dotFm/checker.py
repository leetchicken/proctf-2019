from api import Api
from chklib import Checker, Verdict, \
    CheckRequest, PutRequest, GetRequest
from playlist_creator import create_playlist_file

checker = Checker()


@checker.define_check
async def check_service(request: CheckRequest) -> Verdict:
    api = Api(request.hostname)
    file = create_playlist_file("nothing here")
    music_id = (await api.upload_playlist(file))["created"]
    await api.download_music(music_id, 0)

    return Verdict.OK()


@checker.define_put(vuln_num=1, vuln_rate=1)
async def put_flag_into_the_service(request: PutRequest) -> Verdict:
    api = Api(request.hostname)

    return Verdict.OK("my_new_flag_id")


@checker.define_get(vuln_num=1)
async def get_flag_from_the_service(request: GetRequest) -> Verdict:
    api = Api(request.hostname)

    return Verdict.OK()


if __name__ == '__main__':
    checker.run()
