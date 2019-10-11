from dataclasses import dataclass
from typing import List, Dict


@dataclass
class PlayList:
    image_hashes: List[str]
    music_hashes: List[str]
    tags: List[Dict[str, str]]
    playlist_name: str
