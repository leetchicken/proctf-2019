#!/usr/bin/env python3
import os,string,random,time,shutil,sys,tempfile
from struct import *
import subprocess as sp
from .pack_arith import *
from . import remote_runner
class Generator:
    def __init__(self):
        self.runner = remote_runner.RemoteVerifier("10.60.31.102",3011,"test","sfdkjfds45a")

    def Generate(self,password,message,out_path,debug=True):
        mod_dir = os.path.dirname(__file__)
        cur_dir = os.getcwd()
        if mod_dir != "":
            os.chdir(mod_dir)
        tmp_dir=tempfile.TemporaryDirectory()
        tmp_dir_name = tmp_dir.name

        shutil.copyfile("build.sh",os.path.join(tmp_dir_name,"build.sh"))
        shutil.copyfile("template_arith.S",os.path.join(tmp_dir_name,"template_arith.S"))
        shutil.copyfile("template_arith_main.S",os.path.join(tmp_dir_name,"template_arith_main.S"))
        shutil.copyfile("template_final.S",os.path.join(tmp_dir_name,"template_final.S"))
        shutil.copyfile("build.sh",os.path.join(tmp_dir_name,"build.sh"))

        os.chdir(os.path.join(mod_dir,tmp_dir_name))

        pack_count = 50
        make_flag_inc("message.inc",0xCAFEBABE,password,message)
        compile_raw_file("template_final.1.S","out1.bin")
        raw_payload = open("out1.bin","rb").read()

        for i in range(pack_count):
            make_arith_pack_unpack_encode("template_arith.S","template_arith.1.S",raw_payload)
            compile_raw_file("template_arith.1.S","prog.tmp")
            raw_payload = open("prog.tmp","rb").read()

        make_arith_pack_unpack_encode("template_arith_main.S","template_arith_main.final.S",raw_payload)
        compile_elf_file("template_arith_main.final.S","prog")
        os.chdir(cur_dir)
        shutil.copyfile(os.path.join(mod_dir,tmp_dir_name,"prog"),out_path)
        os.chmod(out_path,0o755)
        return True

    def verify2(self,password,message,file,debug=False):
        proc = sp.Popen(['timeout','-s',"KILL","7","./seccomp_test/wrapper",file,password],stdout=sp.PIPE)
        out = proc.communicate()
        if not message in out[0].decode():
            return False
        return True

    def verify(self,password,message,file,debug=False):
        output=self.runner.RunUserBinary(file,password)
        #proc = sp.Popen(['timeout','-s',"KILL","7","./seccomp_test/wrapper",file,password],stdout=sp.PIPE)
        #out = proc.communicate()
        if not message in output.decode():
            return False
        return True;

if __name__ == "__main__":
    g=Generator()
    g.Generate("qweasdzxc",'sdlkasdlkfasdf\n',"mes2",True)
