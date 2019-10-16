from mutagen.id3 import ID3, APIC


def patch_music(music_path: str, image_path: str):
    audio = ID3(music_path)
    with open(image_path, mode="rb") as img:
        img_data = img.read()
    audio.add(
        APIC(
            encoding=3,  # 3 is for utf-8
            mime='image/png',  # image/jpeg or image/png
            type=3,  # 3 is for the cover image
            desc=u'Cover',
            data=img_data
        )
    )
    audio.save()