
import os,string,random
import subprocess as sp

class Generator:
    def __init__(self):
        pass

    def Generate(self,password,message,out_path,debug=False):

        kk = 0
        for p in password:
            kk = kk ^ ord(p)
        enc_mes = ""
        for c in message:
            enc_mes += chr(kk ^ ord(c))
        ar = []
        for c in enc_mes:
            ar.append(hex(ord(c)))
        ar.append(hex(0))
        mes = "{%s}" %(",".join(ar))
        meslen = len(message)

        mod_dir = os.path.dirname(__file__)
        cur_dir = os.getcwd()
        os.chdir(mod_dir)

        proc = sp.Popen(['./compile.sh',str(meslen),mes,out_path,str(kk)],\
                                                stdout=sp.PIPE,stderr=sp.PIPE)
        out,err = proc.communicate()
        if debug:
            print("[DEBUG] Generate",password,message,out_path,out,err)
        os.chdir(cur_dir)
        if len(err.decode()) != 0:
            return False
        return True
    def verify(self,password,message,file,debug=False):
        for i in range(3):
            c=random.choice(string.ascii_uppercase)
            password = password +c+c
            proc = sp.Popen(['timeout',"-s","KILL","7","./seccomp_test/wrapper",file,password],stdout=sp.PIPE)
            out = proc.communicate()
            if not message in out[0].decode():
                return False
        return True
if __name__ == "__main__":
    g=Generator()
    g.Generate("qweqwe",'asdasd',"mes2",True)
