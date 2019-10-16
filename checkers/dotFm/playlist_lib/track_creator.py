from pydub import AudioSegment
from random import randint, choice, sample
import glob
import os.path

SEGMENTS_PATH = None
drums_factor = "Beats"


def __get_samples_paths(bpm=None):
    paths = glob.glob(f"{SEGMENTS_PATH}/**/*.wav", recursive=True)
    max_bpm = max({int(x.split("Kit0")[1][0]) for x in paths})
    track_bpm = bpm if bpm is not None else randint(1, max_bpm)
    samples = {}
    for category, track in [x.split("/")[-2:] for x in paths if f"Kit0{track_bpm}" in x]:
        samples.setdefault(category, []).append([x for x in paths if x.endswith(track)][0])

    drums_track_path = choice([x for x in paths if drums_factor in x and f"Kit0{track_bpm}" in x])

    return drums_track_path, sample([choice(samples[k]) for k in samples if drums_factor not in k], 3), bpm


def create_track(track_name="music.mp3", image_path="image.png", tags=None, track_length=15):
    drums, others, bpm = __get_samples_paths()
    drums_segment = AudioSegment.from_wav(drums)
    multiplier = int(track_length / drums_segment.duration_seconds)
    repeated = drums_segment * max(multiplier, 1)

    for instrument in others:
        rand_instrument = AudioSegment.from_wav(instrument)
        repeated = repeated.overlay(rand_instrument, loop=True)

    _, mixes, _, = __get_samples_paths(bpm)
    instrument = choice(mixes)
    mix = AudioSegment.from_wav(instrument)
    repeated = repeated.overlay(mix, (len(drums_segment) - 1) * randint(5, track_length // 2), times=randint(3, 5))

    repeated = repeated.fade_out(len(drums_segment))

    repeated.export(os.path.curdir + f"/{track_name}", format="mp3", tags=tags)#,
                    #parameters=["-i", os.path.abspath(os.path.curdir + "/" + image_path)]),
                    #cover=os.path.curdir + "/" + image_path) #forcefully converts to jpg, meh

    return track_name
