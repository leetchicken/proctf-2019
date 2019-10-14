import aiohttp
from masking_connector import MaskingConnector

PORT = 8081


class Api:
    def __init__(self, hostname: str):
        self.hostname = hostname
        self.session = aiohttp.ClientSession(connector=MaskingConnector(), timeout=5)

    async def upload_playlist(self, path) -> dict:
        with open(path, mode="rb") as archive_descriptor:
            archive_bytes = archive_descriptor.read()
        async with self.session.post(f"http://{self.hostname}:{PORT}/channel", data=archive_bytes) as resp:
            return await resp.json()

    async def download_music(self, playlist_id, track_number) -> bytes:
        async with self.session.get(f"http://{self.hostname}:{PORT}/channel?id={playlist_id}&num={track_number}") as resp:
            return await resp.content.read()