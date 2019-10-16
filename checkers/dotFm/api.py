import aiohttp
from networking.masking_connector import get_agent
from aiohttp.client import ClientTimeout
from random import randint

PORT = 1012


class Api:
    def __init__(self, hostname: str):
        self.hostname = hostname
        self.session = aiohttp.ClientSession(timeout=ClientTimeout(total=10), headers={"User-Agent": get_agent()})

    async def upload_playlist(self, path: str) -> dict:
        with open(path, mode="rb") as archive_descriptor:
            archive_bytes = archive_descriptor.read()
        x, y = randint(-1 * 10 ** 20, 10 ** 20), randint(-1 * 10 ** 20, 10 ** 20)
        async with self.session.post(f"http://{self.hostname}:{PORT}/channel", data=archive_bytes, headers={"Position": f"{x},{y}"}) as resp:
            return await resp.json()

    async def download_music(self, playlist_id, track_number) -> bytes:
        async with self.session.get(f"http://{self.hostname}:{PORT}/channel?id={playlist_id}&num={track_number}") as resp:
            return await resp.content.read()

    async def download_image(self, image_name: str) -> bytes:
        async with self.session.get(f"http://{self.hostname}:{PORT}/image/{image_name}") as resp:
            return await resp.content.read()

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.session.close()
