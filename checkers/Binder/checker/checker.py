#!/usr/bin/env python3
import requests, traceback
import string,random,re,sys,json,os,shutil,stat
import subprocess as sp

import pow
import message_packers.simple_1.gen as simple_1
import message_packers.middle_1.gen as middle_1
def id_gen(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

generators = [simple_1.Generator(),middle_1.Generator()]
binary_dir = "files"
tmp_dir = "tmp"
if not os.path.exists(tmp_dir):
    os.mkdir(tmp_dir)

USER_AGENTS = ["Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.80 Safari/537.36",
"Mozilla/5.0 (Linux; Android 8.0.0; SM-G930F Build/R16NW; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/74.0.3729.157 Mobile Safari/537.36",
"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36",
"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) HeadlessChrome/74.0.3729.157 Safari/537.36",
"Mozilla/5.0 (Linux; Android 9; SM-G960F Build/PPR1.180610.011; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/74.0.3729.157 Mobile Safari/537.36",
"Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.157 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36",
"Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0",
"Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0",
"Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0",
"Mozilla/5.0 (Windows NT 10.0; WOW64; rv:66.0) Gecko/20100101 Firefox/66.0",
"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:66.0) Gecko/20100101 Firefox/66.0",
"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0",
"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/6.0)",
"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)",
"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; Trident/6.0)",
"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0; MASMJS)",
"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0; MDDCJS)",
"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)"]
ua = random.choice(USER_AGENTS)
cmd = sys.argv[1]
if cmd == "info":
    print("vulns: 1:1")
    exit(101)
ip=sys.argv[2]
url = "http://"+ip+":3010/"
def generate_elf(password,message,vuln):
    tmp_file_name =  id_gen(7)+".elf"
    tmp_file_path = os.path.join(os.getcwd(),binary_dir,tmp_file_name)
    #idx = random.choice(range(len(generators)))
    idx=int(vuln)
    gen = generators[idx-1]
    if not gen.Generate(password,message,tmp_file_path):
        return None
    return tmp_file_path,idx

try:
    if cmd == "check":
        r1 = requests.get(url+"list")
        if not "Obfuscated messages" in r1.text:
            exit(102)
        exit(101)
    elif cmd == "put":
        flag_id = sys.argv[3]
        flag = sys.argv[4]
        vuln = sys.argv[5]
        sess = requests.Session()
        r=sess.get(url+"list")

        challange = re.findall(r'md5\("([^"]+)"\+',r.text)
        if len(challange) == 0:
            sys.stderr.write("No challange found \n")
            exit(102)
        challange=challange[0]
        pow_res = pow.generate_pow(challange)

        password = id_gen(12)
        fp,idx_gen = generate_elf(password,flag,vuln)
        r1=sess.post(url+"upload",data=dict(challange=pow_res),files={'message':("data",open(fp,'rb'),'application/octet-stream')})
        os.unlink(fp)
        fid = re.findall(r'with id ([A-Z0-9]+)',r1.text)
        if len(fid) == 0:
            sys.stderr.write("Cannot upload file\n")
            exit(102)
        message_id = fid[0]
        r2=requests.get(url+"list")
        if not message_id in r2.text:
            sys.stderr.write("Cannot find id on main page")
            exit(102)
        print ("%s,%s,%d" %(password,message_id,idx_gen))
        exit(101)
    elif cmd == "get":
        password,message_id,gennum=sys.argv[3].split(",")
        gennum = int(gennum)-1
        flag = sys.argv[4]
        sess = requests.Session()
        r1 = sess.get(url+"list")
        if not message_id in r1.text:
            exit(102)

        r2 = sess.get(url+"run?message_id="+message_id+"&password="+password)

        if not flag in r2.text:
            sys.stderr.write("Error 1 %s %s\n" %(flag,r2.text))
            exit(102)
        r = sess.get(url+"get?message_id="+message_id, stream=True)
        if not r.status_code == 200:
            sys.stderr.write("Error 2 %d\n"%(r.status_code))
            exit(102)
        tmp_fname = id_gen(12)+".elf"
        tmp_path = os.path.join(tmp_dir,tmp_fname)
        f = open(tmp_path, 'wb+')
        r.raw.decode_content = True
        shutil.copyfileobj(r.raw, f)
        f.close()
        os.chmod(tmp_path,stat.S_IXUSR | stat.S_IRUSR)
        if not generators[gennum].verify(password,flag,tmp_path):
            os.unlink(tmp_path)
            sys.stderr.write("Error 3 %s %s %s\n"%(password,flag,tmp_path))
            exit(102)
        os.unlink(tmp_path)
        exit(101)
    else:
        print("No command")
except Exception as err:
    traceback.print_tb(err.__traceback__)
    exit(104)
