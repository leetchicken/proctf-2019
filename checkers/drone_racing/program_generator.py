import math
import textwrap

import checklib.random
import random
import string
import pprint
from dataclasses import dataclass


DIRECTION_TO_FUNCTION = dict(U="up", D="down", R="right", L="left")
ID_CHARS = string.ascii_letters
INDENT = "   "


@dataclass
class Function:
    name: str
    arguments: map
    return_type: str
    body: str


def generate_moves_sequence(maze):
    path = find_path(maze)
    moves = []
    prev = path[0]
    for next in path[1:]:
        moves.append(get_move(*prev, *next))
        prev = next
    return ''.join(moves)


def find_path(maze):
    size = int(math.sqrt(len(maze)))
    dx = [-1, 0, 1, 0]
    dy = [0, -1, 0, 1]

    def dfs(x, y):
        for i in range(4):
            nx = x + dx[i]
            ny = y + dy[i]
            if 0 <= nx < size and 0 <= ny < size and maze[size * nx + ny] == '.' and (nx, ny) not in was:
                was.add((nx, ny))
                parents[(nx, ny)] = (x, y)
                dfs(x + dx[i], y + dy[i])

    parents = {}
    was = set()
    dfs(0, 0)

    if (size - 1, size - 1) not in was:
        raise Exception("Can't find path :(")

    path = []
    x, y = size - 1, size - 1
    while x != 0 or y != 0:
        path.append((x, y))
        x, y = parents[(x, y)]
    path.append((0, 0))
    return list(reversed(path))


def get_move(x1, y1, x2, y2):
    if x1 == x2 and y1 + 1 == y2:
        return 'R'
    if x1 == x2 and y1 - 1 == y2:
        return 'L'
    if x1 + 1 == x2 and y1 == y2:
        return 'D'
    if x1 - 1 == x2 and y1 == y2:
        return 'U'
    raise ValueError("Unknown move from (%d, %d) to (%d, %d)" % (x1, y1, x2, y2))


def generate_program(moves, flag=None):
    params = generate_params()
    functions = generate_function('main', moves, params, flag)
    source_code = ''
    for function in functions:
        if function.name == "main":
            source_code += function.body + "\n"
        else:
            source_code += render_function(function)
    return source_code, params


def render_function(function: Function):
    function_return_type = ""
    if function.return_type != "":
        function_return_type = ": " + function.return_type
    return "fun %s()%s\nbegin\n" % (function.name, function_return_type) + textwrap.indent(function.body, INDENT) + "end\n\n"


def generate_function(name, moves, params, flag=None):
    function = Function(name, {}, "", "")

    if name != "main":
        return_type_option = random.randint(0, 2)
        if return_type_option == 0:
            function.return_type = "int"
        elif return_type_option == 1:
            function.return_type = "str"

    functions = []
    parts = split_to_parts(moves)
    already_printed_flag = flag is None
    for part in parts:
        if random.randint(0, 2) <= 1 and len(part) >= 3:  # generate a function and call it
            extracted_function_name = generate_id()
            flag_for_function = None
            if not already_printed_flag and random.choice(parts) == part:
                flag_for_function = flag
                already_printed_flag = True
            extracted_functions = generate_function(extracted_function_name, part, params, flag_for_function)
            extracted_function = extracted_functions[-1]
            functions.extend(extracted_functions)
            function.body += extracted_function_name + "()\n"
        else:  # generate a sequence of moves (calls of goX() methods), maybe with loops
            if not already_printed_flag and random.choice(parts) == part:
                function.body += "write(\"%s\")\n" % flag
                already_printed_flag = True
            move_idx = 0
            while move_idx < len(part):
                if move_idx + 2 < len(part) and part[move_idx] == part[move_idx + 1] == part[move_idx + 2] and random.randint(0, 2) <= 1:
                    count = 0
                    while move_idx + count < len(part) and part[move_idx + count] == part[move_idx]:
                        count += 1
                    loop_length = random.randint(count // 2, count)
                    loop = generate_loop(DIRECTION_TO_FUNCTION[part[move_idx]] + "()", loop_length)
                    function.body += loop
                    move_idx += loop_length
                else:
                    move = part[move_idx]
                    function.body += DIRECTION_TO_FUNCTION[move] + "()\n"
                    move_idx += 1
                if random.randint(0, 4) == 0:
                    function.body += generate_nop(params)

    if not already_printed_flag:
        function.body += "write(\"%s\")\n" % flag

    if function.return_type == "str":
        function.body += 'return "%s"\n' % generate_random_string_literal()
    elif function.return_type == "int":
        function.body += 'return %d\n' % random.randint(-1000, 1000)

    return functions + [function]


def generate_loop(operation, length):
    loop_variable = generate_id()
    shift = random.randint(0, 10)
    if random.randint(0, 5) == 0:
        return "for %s = %d .. %d do begin\n%s\nend\n" % (
            loop_variable, 1 + shift, length + shift, textwrap.indent(operation, INDENT)
        )
    else:
        variable = generate_id()
        expression = generate_variable_value(variable, length + shift)
        return expression + "for %s = %d .. %s do begin\n%s\nend\n" % (
            loop_variable, 1 + shift, variable, textwrap.indent(operation, INDENT)
        )


def generate_nop(params, never_be_here=False):
    option = random.randint(0, 2 if never_be_here else 1)
    if option == 0:
        return "%s = %d\n" % (generate_id(), random.randint(-1000, -1000))
    elif option == 1 and len(params) > 0:
        name, value = random.choice(list(params.items()))
        random_value = generate_random_string_literal()
        return 'if strcmp(%s, "%s") == 0 then\n%send\n' % (
            "$" + name, random_value, textwrap.indent(generate_nop(params, True), INDENT)
        )
    else:
        return random.choice(list(DIRECTION_TO_FUNCTION.values())) + "()\n"


def generate_variable_value(variable, value):
    option = random.randint(0, 4)
    if option == 0:  # Just an assignment
        return "%s = %d\n" % (variable, value)

    result = ""
    if option == 1:  # Assign a sum of two terms
        term1 = random.randint(-100, 100)
        term2 = value - term1
        operation = "+"
    elif option == 2:  # value = term1 - term2
        term1 = random.randint(-100, 100)
        term2 = term1 - value
        operation = "-"
    elif option == 3:
        term1, term2 = get_random_multiplicators(value)
        operation = "*"
    else:
        term2 = random.randint(1, 100)
        term1 = term2 * value
        operation = "/"

    if random.randint(0, 1) == 0:
        v = generate_id()
        result += generate_variable_value(v, term1)
        term1 = v
    if random.randint(0, 1) == 0:
        v = generate_id()
        result += generate_variable_value(v, term2)
        term2 = v
    result += "%s = %s %s %s\n" % (variable, term1, operation, term2)
    return result


def get_random_multiplicators(value):
    """
    Returns tuple (x, y) such that x * y == value
    """
    if value == 0:
        return random.randint(-100, 100), 0
    dividers = []
    x = 1
    while x * x <= abs(value):
        if value % x == 0:
            dividers.append(x)
        x += 1
    mult1 = random.choice(dividers)
    mult2 = value // mult1
    if random.randint(0, 1) == 0:
        mult1 = -mult1
        mult2 = -mult2

    return mult1, mult2


def split_to_parts(moves):
    if len(moves) <= 3:
        return [moves]
    parts_count = random.randint(1, len(moves) // 3)
    sample = random.sample(range(len(moves) + parts_count), parts_count)
    sample = [0] + sorted(sample) + [len(moves) + parts_count]
    result = []
    prev = 0
    for idx in range(len(sample) - 1):
        result.append(moves[prev:prev + sample[idx + 1] - sample[idx]])
        prev += sample[idx + 1] - sample[idx]
    return list(filter(bool, result))


def generate_params():
    params_count = random.randint(1, 10)
    result = {}
    for _ in range(params_count):
        result[generate_id()] = generate_random_string_literal()
    return result


def generate_id():
    return checklib.random.string(ID_CHARS, random.randint(3, 10))


def generate_random_string_literal():
    return checklib.random.string(string.ascii_letters + " ", random.randint(3, 40))


if __name__ == "__main__":
    maze = "...*.......*....*.*.*****.*.**.*...*...*.*....*****.***.*.*..*.........*.*..*****.*******......*.......*..***.*******.*..*...*.....*...**.*****.*****....*.....*...*..***.*****.*.*..*.*.......*....*.*.**********.*............."
    print(generate_program(generate_moves_sequence(maze))[0])
    # print(split_to_parts('abacabadabacaba'))