import png
import random


def __generate_rand_tuple_color():
    return random.randint(0, 1) * 255, random.randint(0, 1) * 255, random.randint(0, 1) * 255


def __generate_32x_line():
    return (
        *__generate_rand_tuple_color() * 8,
        *__generate_rand_tuple_color() * 8,
        *__generate_rand_tuple_color() * 8,
        *__generate_rand_tuple_color() * 8
    )


def generate_image(filename="image.png"):
    lines = []
    for i in range(0, 32, 8):
        line = __generate_32x_line()
        for k in range(8):
            lines.append(line)

    with open(filename, 'wb') as file:
        w = png.Writer(32, 32, greyscale=False)
        w.write(file, lines)

    return filename
