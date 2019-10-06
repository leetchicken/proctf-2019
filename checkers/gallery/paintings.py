import os
import pathlib
import random

TEAMS_COUNT = 30

PAINTINGS_DIR = pathlib.Path("paintings")

def next_painting(team_num):
    files = []
    team_dir = PAINTINGS_DIR / team_num
    for x in team_dir.iterdir():        
        if x.suffix == ".jpg" and not x.stem.endswith("_replica"):
            files.append(x)
    file = random.choice(files)
    new_name = file.stem + ".bak"    
    file.rename(team_dir / new_name)

    with open(team_dir / new_name, "rb") as file:
        return (new_name, file.read())

def get(team_num, file_name):
    team_dir = PAINTINGS_DIR / team_num
    with open(team_dir / new_name, "rb") as file:
        return file.read()

def get_replica(team_num, file_name):
    team_dir = PAINTINGS_DIR / team_num
    replica_name = pathlib.Path((team_dir / file_name).stem + "_replica.jpg")

    with open(team_dir / replica_name, "rb") as file:
        return (replica_name, file.read())

def get_random_painting(team_num, file_name):    
    team_dir = PAINTINGS_DIR / team_num
    # for x in team_dir.iterdir():
    #     if x.name != file_name and ((x.suffix == ".jpg" and not x.stem.endswith("_replica")) or (x.suffix == ".bak")):
    #         with open(team_dir / x.name, "rb") as file:
    #             return (x.name, file.read())
    return None
