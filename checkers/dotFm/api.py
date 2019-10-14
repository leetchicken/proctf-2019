import aiohttp
from networking.masking_connector import MaskingConnector
from aiohttp.client import ClientTimeout

PORT = 1012


class Api:
    def __init__(self, hostname: str):
        self.hostname = hostname
        self.session = aiohttp.ClientSession(connector=MaskingConnector(), timeout=ClientTimeout(total=10))

    async def upload_playlist(self, path) -> dict:
        with open(path, mode="rb") as archive_descriptor:
            archive_bytes = archive_descriptor.read()
        async with self.session.post(f"http://{self.hostname}:{PORT}/channel", data=archive_bytes) as resp:
            return await resp.json()

    async def download_music(self, playlist_id, track_number) -> bytes:
        async with self.session.get(f"http://{self.hostname}:{PORT}/channel?id={playlist_id}&num={track_number}") as resp:
            return await resp.content.read()

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.session.close()
