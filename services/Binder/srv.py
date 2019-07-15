#!/usr/bin/env python3
from flask import Flask,logging

from flask import Flask,request,render_template,make_response,send_from_directory,session,redirect,logging,jsonify
import dataset,threading,datetime,string,random,os,stat
from functools import wraps, update_wrapper
from logging.handlers import RotatingFileHandler
import subprocess as sp
from stat import S_ISREG, ST_CTIME, ST_MODE
import os, sys, time, re

app = Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'
formatter = logging.logging.Formatter("[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s")
handler = RotatingFileHandler("log.txt", maxBytes=10000000, backupCount=5)
handler.setFormatter(formatter)
app.logger.addHandler(handler)
app.logger.setLevel(logging.logging.INFO)
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024

def id_gen(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


@app.route('/')
def index():
    return redirect("/list")

@app.route('/upload',methods=['POST'])
def upload():
    if not 'message' in request.files:
        return render_template("error.html",message="No message field in request")
    f=request.files['message']
    message_id = id_gen(12)
    fname = message_id + ".elf"
    fpath = os.path.join("messages",fname)
    f.save(fpath)
    os.chmod(fpath,stat.S_IXUSR | stat.S_IRUSR)
    return render_template("uploaded.html",message="Successfuly added message with id "+message_id)

@app.route('/list')
def list_mes():
    dirpath = "messages"
    entries = (os.path.join(dirpath, fn) for fn in os.listdir(dirpath))
    entries = ((os.stat(path), path) for path in entries)
    entries = ((stat[ST_CTIME], path) for stat, path in entries if S_ISREG(stat[ST_MODE]))
    messages = []
    for cdate, path in sorted(entries):
        print(time.ctime(cdate), os.path.basename(path))
        bn=os.path.basename(path)
        fn = os.path.splitext(bn)
        messages.append(fn[0])
    return render_template("list.html",messages=messages)

@app.route('/get')
def get_mes():
    if not 'message_id' in request.values:
        return render_template("error.html",message="No fields")
    message_id = request.values['message_id']
    fname = message_id+".elf"
    fname_path = os.path.join("messages",message_id+".elf")
    if not os.path.isfile(fname_path):
        return abort(404)
    return send_from_directory(directory="messages",filename=fname,as_attachment=True)

@app.route('/run')
def run_mes():
    if not 'message_id' in request.values or not 'password' in request.values:
        return render_template("error.html",message="No fields")
    message_id = request.values['message_id']
    password = request.values['password']

    fname = os.path.join("messages",message_id+".elf")
    if not os.path.isfile(fname):
        return abort(404)

    proc = sp.Popen(['timeout','5',"./seccomp_test/wrapper",fname,password],stdout=sp.PIPE)
    out = proc.communicate()
    print("OUTPUT",out)
    return render_template("message.html",content=out[0].decode(),message_id=message_id)

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=3010,threaded=True,debug=True)
