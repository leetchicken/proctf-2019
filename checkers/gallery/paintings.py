import os
import pathlib
import random

TEAMS_COUNT = 30

PAINTINGS_DIR = pathlib.Path("paintings")

def next_painting(team_num):
    files = []
    team_dir = PAINTINGS_DIR / team_num
    for x in team_dir.iterdir():        
        if x.suffix == ".jpg" and not x.stem.endswith("_replica") and not x.stem.endswith("_copy"):
            files.append(x)
    random_file = random.choice(files)
    new_name = random_file.stem + ".bak"    
    random_file.rename(team_dir / new_name)

    with open(team_dir / new_name, "rb") as file:
        return (new_name, file.read())

def get_replica(team_num, file_name):
    team_dir = PAINTINGS_DIR / team_num
    replica_name = pathlib.Path((team_dir / file_name).stem + "_replica.jpg")

    with open(team_dir / replica_name, "rb") as file:
        return (replica_name, file.read())

def get_copy(team_num, file_name):
    team_dir = PAINTINGS_DIR / team_num
    replica_name = pathlib.Path((team_dir / file_name).stem + "_copy.jpg")

    with open(team_dir / replica_name, "rb") as file:
        return (replica_name, file.read())

def get_random_painting(team_num, file_name):    
    files = []
    team_dir = PAINTINGS_DIR / team_num
    for x in team_dir.iterdir():
        if x.name != file_name and ((x.suffix == ".jpg" and not x.stem.endswith("_replica") and not x.stem.endswith("_copy")) or (x.suffix == ".bak")):
            files.append(x)
    random_file = random.choice(files)
    with open(team_dir / random_file.name, "rb") as file:
        return (random_file.name, file.read()) 
