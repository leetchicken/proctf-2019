#!/usr/bin/env python3
from flask import Flask,logging

from flask import Flask,request,render_template,make_response,send_from_directory,session,redirect,logging,jsonify,abort
import dataset,threading,datetime,string,random,os,stat
from functools import wraps, update_wrapper
from logging.handlers import RotatingFileHandler
import subprocess as sp
from stat import S_ISREG, ST_CTIME, ST_MODE
import os, sys, time, re
from flask_session import Session
import hashlib,datetime,threading
from struct import *

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024
app.config['SESSION_TYPE'] = 'filesystem'
app.config['PERMANENT_SESSION_LIFETIME'] = datetime.timedelta(hours=15)
app.config['SESSION_FILE_THRESHOLD'] = 10000
sess = Session()
sess.init_app(app)
MAX_UPLOADED_COUNT = 1000
def CleanOld():
    dirpath="messages"
    entries = (os.path.join(dirpath, fn) for fn in os.listdir(dirpath))
    entries = ((os.stat(path), path) for path in entries)
    entries = ((stat[ST_CTIME], path) for stat, path in entries if S_ISREG(stat[ST_MODE]))
    all_file_names = []
    for cdate, path in sorted(entries):
        all_file_names.append(path)
    if len(all_file_names) >= MAX_UPLOADED_COUNT:
        delete_files = all_file_names[:-1*MAX_UPLOADED_COUNT]
        print("Removing %d messages\n" %(len(delete_files)))
        for fl in delete_files:
            os.unlink(fl)

def CleanOldLoop():
    while 1:
        CleanOld()
        time.sleep(60)

def RunCleaner():
    t=threading.Thread(target=CleanOldLoop)
    t.daemon=True
    t.start()

def id_gen(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

@app.route('/')
def index():
    return redirect("/list")

def verify_challange(chal,ans):
    sum = hashlib.md5()
    sum.update((chal + ans).encode('utf-8'))
    hash = sum.digest()
    val = unpack("<L",hash[0:4])[0]
    print(val,chal,ans,sum.hexdigest())
    if val > 0xFFFFFaa0:
        return True
    return False

@app.route('/upload',methods=['POST'])
def upload():
    if not 'message' in request.files:
        return render_template("error.html",message="No message field in request")
    if not "challange" in request.values:
        return render_template("error.html",message="No challange provided")
    if not verify_challange(session['challange'],\
                            request.values['challange']):
        session['challange'] = id_gen(8)
        return render_template("error.html",message="Invalid answer")
    session['challange'] = id_gen(8)
    f=request.files['message']
    message_id = id_gen(12)
    fname = message_id + ".elf"
    fpath = os.path.join("messages",fname)
    f.save(fpath)
    os.chmod(fpath,stat.S_IXUSR | stat.S_IRUSR)
    return render_template("uploaded.html",message="Successfuly added message with id "+message_id)

@app.route('/list')
def list_mes():
    session['challange'] = id_gen(8)
    dirpath = "messages"
    entries = (os.path.join(dirpath, fn) for fn in os.listdir(dirpath))
    entries = ((os.stat(path), path) for path in entries)
    entries = ((stat[ST_CTIME], path) for stat, path in entries if S_ISREG(stat[ST_MODE]))
    messages = []
    for cdate, path in sorted(entries):
        #print(time.ctime(cdate), os.path.basename(path))
        bn=os.path.basename(path)
        fn = os.path.splitext(bn)
        messages.append(fn[0])
    return render_template("list.html",messages=messages,challange=session['challange'])

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
    if 'message_id' not in request.values or 'password' not in request.values:
        return render_template("error.html",message="No fields")
    message_id = request.values['message_id']
    password = request.values['password']

    fname = os.path.join("messages",message_id+".elf")
    if not os.path.isfile(fname):
        return abort(404)

    proc = sp.Popen(['timeout','2',"./seccomp_test/wrapper",fname,password],stdout=sp.PIPE)
    out = proc.communicate()
    print("OUTPUT",out)
    return render_template("message.html",content=out[0].decode(),message_id=message_id)

if __name__ == "__main__":
    RunCleaner()
    app.run(host='0.0.0.0', port=3010,threaded=True,debug=False)
