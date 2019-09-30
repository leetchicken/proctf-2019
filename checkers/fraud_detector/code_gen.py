import random
import re
import json

from random import choice, randint, randrange, sample

import fraud_detector
from word_list import WORDLIST

NAME_ABC = "qwertyuiopasdfghjklzxcvbnm_"

STR_FIELDS = ["name", "surname", "registered_at", "favorite_music"]
NUM_FIELDS = ["age", "is_fraud", "trip_count", "payment_failures_rate", "avg_trip_km"]

TYPES = ["bool", "object", "str", "int", "list", "set", "dict", "bytes"]
FUNCTIONS = ["locals", "globals", "ord", "chr", "len", "randint", "sha256"]
KEYWORDS = ["with", "while", "yield", "class", "if", "elif", "else", "from", "as"]
BAD_NAMES = TYPES + FUNCTIONS + KEYWORDS

VOWELS = set("AIUEOaiueo")
CONSONANTS = set("BDFGJKLMNPRSTVWXZbdfgjklmnprstvwxz")


def gen_rand_str(min_len=10, max_len=11, abc="1234567890qwertyuiopasdfghjklzxcvbnm_"):
    n = randint(min_len, max_len)

    abc_even = list(set(abc) - VOWELS) or abc
    abc_odd = list(set(abc) - CONSONANTS) or abc

    s = []
    while len(s) < n:
        if len(s) % 2 == 0:
            s.append(choice(abc_even))
        else:
            s.append(choice(abc_odd))

    return "".join(s)


used_names = set()

def gen_name():
    while True:
        need_fallback = len(used_names) * 2 > len(WORDLIST)
        name = random.choice(WORDLIST) if not need_fallback else gen_rand_str(4, 7, NAME_ABC)
        if name not in BAD_NAMES and name not in used_names:
            used_names.add(name)
            return name


def gen_str_literal():
    quote = choice(["'", '"'])
    return quote + " ".join(gen_name() for i in range(randint(1, 3))) + quote


def gen_iterable():
    n = randint(2, 7)

    iter_type = choice(["list", "tuple", "set", "dict"])
    if iter_type == "list":
        return "[%s]" % (", ".join(str(randint(1, 100)) for i in range(n)))
    elif iter_type == "tuple":
        return "%s" % (", ".join(str(randint(1, 100)) for i in range(n)))
    elif iter_type == "set":
        return "{%s}" % (", ".join(str(randint(1, 100)) for i in range(n)))
    else:
        return "{%s}" % (", ".join("%d: %d" % (randint(1, 100), randint(1, 100)) for i in range(n)))


def gen_function_check1(var_name):
    f_name = gen_name()
    arg1_name = gen_name()
    arg2_name = gen_name()
    sign = choice("+-*/")
    arg1 = randint(1, 100)
    arg2 = randint(1, 100)
    assign = choice(["+=", "-="])

    return """def {f_name}({arg1_name}, {arg2_name}):
    <GARBAGE>
    return {arg1_name}{sign}{arg2_name}
<GARBAGE>
{var_name}{assign}{f_name}({arg1}, {arg2})""".format(**locals())


def gen_function_check2(var_name):
    f_name = gen_name()
    arg_name = gen_name()
    operation = choice(["+", "-", "ord('%s')+" % choice(NAME_ABC)])
    arg = randint(1, 100)
    assign = choice(["+=", "-="])

    return """def {f_name}({arg_name}):
    <GARBAGE>
    return {operation}{arg_name}
{var_name}{assign}{f_name}({arg})""".format(**locals())


def gen_type_check(var_name):
    return """{0}{1}type(user["{2}"]) {3} {4}""".format(var_name, choice(["+=", "-="]),
                                                        choice(STR_FIELDS+NUM_FIELDS),
                                                        choice(["is", "is not"]), choice(TYPES))


def gen_for_check(var_name):
    return """for {1} in {2}:
    <GARBAGE>
    {0} {3} {1}
    <GARBAGE>""".format(var_name, gen_name(), gen_iterable(), choice(["+=", "-="]))


def gen_while_check(var_name):
    return """{1} = {2}
while {1} < {3}:
    <GARBAGE>
    {0} {4} {1}
    <GARBAGE>
    {1} += 1
    <GARBAGE>""".format(
        var_name, gen_name(), randint(1, 10), randint(1, 10), choice(["+=", "-="]))


def gen_int_data_check(var_name):
    return """{0} {1} user["{2}"]{3}{4}""".format(
        var_name, choice(["+=", "-="]), choice(NUM_FIELDS), choice("+-*/"), randint(2, 5))


def gen_str_data_check(var_name):
    return """{0} {1} ord(globals()["user"]["{2}"][{3}])""".format(
        var_name, choice(["+=", "-="]), choice(STR_FIELDS), randint(1, 2))


def gen_locals_check(var_name):
    f_name = gen_name()
    assign = "; ".join(["%s = %d" % (gen_name(), randint(1, 100)) for i in range(randint(1, 7))])
    assign2 = choice(["+=", "-="])

    return """def {f_name}():
    {assign}
    return len(locals())
{var_name} {assign2} {f_name}()""".format(**locals())


def gen_try_check(var_name):
    return """try:
    <GARBAGE>
    {1}
    <GARBAGE>
except:
    {0} {2} {3}""".format(var_name, choice(["1//0", gen_name(), "pass", gen_str_literal()]),
                          choice(["+=", "-=", "*="]), randint(2, 5))


def gen_if_check1(var_name):
    return """if {1} {2} {3}:
    <GARBAGE>
    {0} {4} {5}
    <GARBAGE>""".format(var_name, choice(["ord(%s[0])" % gen_str_literal(), randrange(1, 10)]),
                        choice(["<", "==", ">", "<=", ">="]), randrange(1, 10),
                        choice(["+=", "-=", "*="]), randrange(1, 10))


def gen_if_check2(var_name):
    return """if {1} {2} {3}:
    <GARBAGE>
    {0} {4} {5}
    <GARBAGE>
else:
    <GARBAGE>
    {0} {6} {7}
    <GARBAGE>""".format(var_name, choice(["ord(%s[0])" % gen_str_literal(), randrange(1, 10)]),
                        choice(["<", "==", ">", "<=", ">="]), randrange(1, 10),
                        choice(["+=", "-=", "*="]), randrange(1, 10),
                        choice(["+=", "-=", "*="]), randrange(1, 10))


GARBAGE_GENERATORS = [
    lambda: "# " + " ".join(gen_name() for i in range(randint(1, 3))),
    lambda: "",
    lambda: "randint(%d, %d)" % (randint(0, 10), randint(10, 20)),
    lambda: "%s = %s" % (gen_name(), gen_str_literal()),
    lambda: "%s = %s" % (gen_name(), gen_iterable()),
    lambda: "%s = %d" % (gen_name(), randrange(1, 10000)),
    lambda: "%s = chr(%d)" % (gen_name(), randrange(32, 128)),
    lambda: "%d" % randrange(1, 10000),
    lambda: gen_str_literal(),
    lambda: gen_iterable(),
]


def gen_garbage(align):
    lines = []
    for i in range(choice([0, 0, 1, 1, 2, 3])):
        lines.append(" " * align + random.choice(GARBAGE_GENERATORS)() + "\n")
    return "".join(lines)


def gen_finalizer(var_name):
    return """fraud_prob = {}""".format(
        choice([
            "int(%s) %% 100" % var_name,
            "sha256(str(%s))[%d] %% 100" % (var_name, random.randint(-16, 16))
        ]))


SNIPPETS = [
    gen_function_check1, gen_function_check2,
    gen_type_check, gen_type_check, gen_type_check, gen_type_check,
    gen_for_check, gen_for_check,
    gen_while_check,
    gen_int_data_check, gen_int_data_check, gen_int_data_check, gen_int_data_check,
    gen_str_data_check, gen_str_data_check, gen_str_data_check,
    gen_locals_check,
    gen_try_check, gen_try_check,
    gen_if_check1, gen_if_check2,
]


def gen_base_code(var_name, payload):
    check_len = randint(300, 800)

    check_snippets = SNIPPETS[:]
    random.shuffle(check_snippets)

    code = []
    for snippet in check_snippets:
        code_len = sum(map(len, code))
        if code_len > check_len:
            break
        code.append(snippet(var_name))

    code.insert(0, "{} = {}".format(var_name, randint(1, 100)))
    code.insert(randint(1, len(code)), payload)
    code.append(gen_finalizer(var_name))
    return code


def join_and_garbagify(code):
    code = "\n<GARBAGE>\n".join(code)
    code = re.sub(r"^( *)<GARBAGE> *\n", lambda m: gen_garbage(len(m.group(1))), code, flags=re.M)
    return code


def gen_check(var_name, payload):
    base_code = gen_base_code(var_name, payload)
    code = join_and_garbagify(base_code)
    return code


def gen_empty_check():
    var_name = gen_name()
    return gen_check(var_name, "")


def gen_rand_check():
    var_name = gen_name()
    rand_expr = "randint(%d, %d)" % (randint(0, 10), randint(20, 30))
    return gen_check(var_name, "{0} {1} {2}".format(var_name, choice(["+=", "-="]), rand_expr))


def gen_vuln1_check(flag):
    var_name = gen_name()
    payload = """{0}{1}sha256("{2}")[len(str(user["{3}"])) % 32]""".format(var_name,
                                                                           choice(["+=", "-="]),
                                                                           flag, choice(STR_FIELDS))
    return gen_check(var_name, payload)


def gen_vuln2_check(flag):
    var_name = gen_name()
    tmp_name = gen_name()
    tmp_name2 = gen_name()
    payload = """{1}="{2}" + user["{3}"]
for {4} in {1}[::len(user["{3}"]) % 4 + 2]:
    <GARBAGE>
    {0}{5}ord({4})
    <GARBAGE>""".format(var_name, tmp_name, flag, choice(STR_FIELDS), tmp_name2,
                        choice(["+=", "-="]))
    return gen_check(var_name, payload)


def gen_vuln3_check(flag):
    var_name = gen_name()
    tmp_name = gen_name()
    tmp_name2 = gen_name()
    payload = """{1}="{2}"
for {4} in {1}[::len(user["{3}"]) % 4 + 2]:
    <GARBAGE>
    {0}{5}ord({4})
    <GARBAGE>""".format(var_name, tmp_name, flag, choice(STR_FIELDS), tmp_name2,
                        choice(["+=", "-="]))
    return gen_check(var_name, payload)


if __name__ == "__main__":
    users = random.sample(json.load(open("users.json")), 3)
    n = 100

    rules = [gen_empty_check() for i in range(n)]
    # print(rules[0])

    for rule in rules:
        if len(rule) > 2000:
            print("gen_check: rule too long")
            print(rule)

    for user in users:
        try:
            print(fraud_detector.run_rules(rules, user))
        except Exception as e:
            print(user)
            raise e
