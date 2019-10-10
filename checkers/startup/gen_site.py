#!/usr/bin/python3

import random
import os
import sys
import json
import re

N = 2048

TMPL1 = """<?php

$id=intval($_REQUEST["id"]);

$key=$_REQUEST["key"];
$flag=$_REQUEST["flag"];

if (!preg_match("/^[a-zA-Z0-9_=]+$/", $flag)) die("bad flag format");
if (!preg_match("/^[a-zA-Z0-9]+$/", $key)) die("bad key format");

$data='<?php if($_GET["key"]!="'.$key.'") die("bad key"); echo "'.$flag.'"; ?>';

file_put_contents("{}_$id.php", $data);

echo "OK";
?>"""

TMPL2 = """<?php
$files = glob('*.php');
usort($files, function($a, $b) {
    return filemtime($a) < filemtime($b);
});
foreach ($files as &$file) echo "$file<br>\n";
?>
"""

TMPL3 = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>The startup</title>

<style type="text/css">
body {
    background-color: COLOR;
}
h1 {
    color: COLOR;
    text-align: ALIGN;
}

li {
    color: COLOR,
    font-size: FONTSIZE;
}
</style>

</head>
<body>
<h1>PHRASE!</h1>

<ul>
BULLSHIT
</ul>

</body>
</html>
"""


ABC = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789"

used_randoms = set()


def bullshit_phrase_gen():
    verbs = ["aggregate", "architect", "benchmark", "brand", "cultivate", "deliver", "deploy",
             "disintermediate", "drive", "e-enable", "embrace", "empower", "enable", "engage",
             "engineer", "enhance", "envisioneer", "evolve", "expedite", "exploit", "extend",
             "facilitate", "generate", "grow", "harness", "implement", "incentivize", "incubate",
             "innovate", "integrate", "iterate", "leverage", "matrix", "maximize", "mesh",
             "monetize", "morph", "optimize", "orchestrate", "productize", "recontextualize",
             "redefine", "reintermediate", "reinvent", "repurpose", "revolutionize", "scale",
             "seize", "strategize", "streamline", "syndicate", "synergize", "synthesize", "target",
             "transform", "transition", "unleash", "utilize", "visualize", "whiteboard"]
    adj = ["24/365", "24/7", "B2B", "B2C", "back-end", "best-of-breed", "bleeding-edge",
           "bricks-and-clicks", "clicks-and-mortar", "collaborative", "compelling",
           "cross-platform", "cross-media", "customized", "cutting-edge", "distributed", "dot-com",
           "dynamic", "e-business", "efficient", "end-to-end", "enterprise", "extensible",
           "frictionless", "front-end", "global", "granular", "holistic", "impactful", "innovative",
           "integrated", "interactive", "intuitive", "killer", "leading-edge", "magnetic",
           "mission-critical", "next-generation", "one-to-one", "open-source", "out-of-the-box",
           "plug-and-play", "proactive", "real-time", "revolutionary", "rich", "robust", "scalable",
           "seamless", "sexy", "sticky", "strategic", "synergistic", "transparent", "turn-key",
           "ubiquitous", "user-centric", "value-added", "vertical", "viral", "virtual", "visionary",
           "web-enabled", "wireless", "world-class"]
    nouns = ["action-items", "applications", "architectures", "bandwidth", "channels",
             "communities", "content", "convergence", "deliverables", "e-business", "e-commerce",
             "e-markets", "e-services", "e-tailers", "experiences", "eyeballs", "functionalities",
             "infomediaries", "infrastructures", "initiatives", "interfaces", "markets",
             "methodologies", "metrics", "mindshare", "models", "networks", "niches", "paradigms",
             "partnerships", "platforms", "portals", "relationships", "ROI", "synergies",
             "web-readiness", "schemas", "solutions", "supply-chains", "systems", "technologies",
             "users", "vortals", "web services"]

    return random.choice(verbs) + " " + random.choice(adj) + " " + random.choice(nouns)


def gen_name():
    global used_randoms
    while True:
        ret = "".join(random.choice(ABC) for i in range(random.randrange(4, 8)))
        if ret not in used_randoms:
            used_randoms.add(ret)
            return ret


def gen_color(x):
    return "#%06x" % random.randrange(256**3)


def gen_fontsize(x):
    return random.choice(["large", "x-large", "xx-large"])


def gen_align(x):
    return random.choice(["left", "center"])


def gen_phrase(x):
    out = ["We"]
    verbs = ["buy", "aggregate", "enhance", "extend", "monetize", "optimize", "redefine",
             "visualize", "sell", "exchange", "got", "have", "examine"]
    nouns = ["bitcoin", "blockchain", "methodologies", "cats", "dogs", "mices", "tables",
             "content", "services", "interfaces", "infrastructure", "metrics", "networks",
             "platforms", "solutions", "systems", "users"]
    out.append(random.choice(verbs))
    out.append(random.choice(nouns))
    return " ".join(out)


def gen_bullshit(x):
    out = []
    for i in range(random.randrange(8, 16)):
        out.append("<li>" + bullshit_phrase_gen() + "</li>")
    return "\n".join(out)


def gen_html():
    html = TMPL3
    html = re.sub("COLOR", gen_color, html)
    html = re.sub("FONTSIZE", gen_fontsize, html)
    html = re.sub("PHRASE", gen_phrase, html)
    html = re.sub("ALIGN", gen_align, html)
    html = re.sub("BULLSHIT", gen_bullshit, html)

    return html


def gen_garbage():
    grb = []

    for i in range(random.randrange(1, 2000)):
        grb_type = random.choice(["random", "ids"])
        if grb_type == "random":
            grb.append(gen_name())
        else:
            keywords = ["php", "'<>>", "eval($GET[$cmd])", "')", "cmd",
                        "exec", "readdir", "SELECT '", "system('id')", "shell('"]

            grb.append(" " + random.choice(keywords) + " ")

        if random.randrange(30) == 0:
            grb.append("\n")
    return "".join(grb)


try:
    os.mkdir("site")
except FileExistsError:
    print("Remove ./site dir first")
    sys.exit(1)


sitemap = {"t1": [], "t2": [], "idx": [], "grb": []}

for i in range(N):
    php_name = gen_name() + ".php"
    check_file_prefix = gen_name()

    open("site/" + php_name, "w").write(TMPL1.format(check_file_prefix))

    sitemap["t1"].append({"name": php_name, "prefix": check_file_prefix})

for i in range(N):
    php_name = gen_name() + ".php"
    check_file_prefix = gen_name()

    open("site/" + php_name, "w").write(TMPL2)

    sitemap["t2"].append({"name": php_name})

for i in range(N):
    dirname = gen_name()
    os.mkdir("site/" + dirname)
    open("site/" + dirname + "/index.html", "w").write(gen_html())
    sitemap["idx"].append({"name": dirname + "/index.html"})

for i in range(N):
    file_name = gen_name() + "." + random.choice("txt html php".split())
    open("site/" + file_name, "w").write(gen_garbage())
    sitemap["grb"].append({"name": file_name})

json.dump(sitemap, open("sitemap.json", "w"))
