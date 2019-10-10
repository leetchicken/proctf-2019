#!/usr/bin/env python3
import subprocess as sp
import struct as st
import random
debug = False

ror = lambda val, r_bits, max_bits: \
    ((val & (2**max_bits-1)) >> r_bits%max_bits) | \
(val << (max_bits-(r_bits%max_bits)) & (2**max_bits-1))

def compile_raw_file(fname,outname):
    proc = sp.Popen(['nasm',fname,"-o",outname],stdout=sp.PIPE,stderr=sp.PIPE)
    out,err = proc.communicate()

def compile_elf_file(fname,outname):
    proc = sp.Popen(['bash',"./build.sh",fname,outname],stdout=sp.PIPE,stderr=sp.PIPE)
    out,err = proc.communicate()

def encode_arith(payload,arith):
    enc_mes_dwords = []
    while len(payload)>0:
        if len(payload)>=4:
            enc_mes_part = payload[0:4]
            payload = payload[4:]
            res = st.unpack("<L",enc_mes_part)[0]
            enc_mes_dwords.append(res)
        else:
            enc_mes_part = payload[:]
            payload = []
            while len(enc_mes_part)<4:
                enc_mes_part+=b'\x90'
            enc_mes_dwords.append(st.unpack("<L",enc_mes_part)[0])
    encoded_payload=[]
    for dw in enc_mes_dwords:
        res = dw
        for op in arith:
            if op['op'] == 'not':
                res = ~res
            elif op['op'] == 'add':
                res = (res + op['key']) & 0xFFFFFFFF
            elif op['op'] == 'sub':
                res = res - op['key']
                if res < 0:
                    res = res + 0x100000000
            elif op['op'] == 'xor':
                res = res ^ op['key']
        encoded_payload.append(res)
    return encoded_payload

def make_arith_pack_unpack_encode(in_fname,out_fname,payload):
    payload_dat = ",".join(map(lambda x: str(x),payload))
    pd_len = int(len(payload)/4)
    if len(payload)%4 !=0:
        pd_len+=1

    packer = open(in_fname,'r').read()

    packer_code = []
    unpacker_code = []
    packer_ops = []
    packer_ops_count = 20 #random.randint(1,2)
    for i in range(packer_ops_count):
        op = random.choice(["add","sub","xor","not"])
        if op != "not":
            k = random.randint(0x10000000,0xFFFFFFFF)
            packer_ops.append(dict(op=op,key=k))
        else:
            packer_ops.append(dict(op=op))

    for op in packer_ops:
        if op['op'] != "not":
            packer_code.append("%s edx, %s" %(op['op'],op['key']))
        else:
            packer_code.append("not edx")
    for op in packer_ops[::-1]:
        if op['op'] == "not":
            unpacker_code.append("not edx")
        elif op['op'] == "add":
            unpacker_code.append("sub edx, %s" %(op['key'],))
        elif op['op'] == "sub":
            unpacker_code.append("add edx, %s" %(op['key'],))
        elif op['op'] == "xor":
            unpacker_code.append("xor edx, %s" %(op['key'],))

    packer_code = "\n".join(packer_code)
    unpacker_code = "\n".join(unpacker_code)

    enc_payload=encode_arith(payload,packer_ops)

    packer = packer.replace("ENCODED_DATA_LEN",str(len(enc_payload)))
    payload_data = ",".join(map(str,enc_payload))
    packer = packer.replace("PAYLOAD_ENCODED",payload_data)
    packer=packer.replace("ENCODER_CODE",packer_code)
    packer=packer.replace("DECODER_CODE",unpacker_code)
    open(out_fname,"w+").write(packer)

def make_flag_inc(fname,start_key,password,message):
    second_key = start_key
    for p in password:
        second_key = second_key ^ ord(p)
        second_key = ror(second_key,8,32)
    enc_mes = b''
    cur_second_key = second_key
    for c in message:
        kk = cur_second_key & 0xFF
        cur_second_key = ror(cur_second_key,8,32)
        enc_mes += bytes([kk ^ ord(c)])
    enc_mes_dwords = []
    enc_mes_copy = enc_mes[:]
    while len(enc_mes_copy)>0:
        if len(enc_mes_copy)>=4:
            enc_mes_part = enc_mes_copy[0:4]
            enc_mes_copy = enc_mes_copy[4:]
            res = st.unpack("<L",enc_mes_part)[0]
            enc_mes_dwords.append(res)
        else:
            enc_mes_part = enc_mes_copy[:]
            enc_mes_copy = []
            while len(enc_mes_part)<4:
                enc_mes_part+=b'\x00'
            enc_mes_dwords.append(st.unpack("<L",enc_mes_part)[0])
    INDEX,KEY_BYTE,SECOND_KEY_REG,SECOND_KEY_REG_BYTE = random.choice([\
    ("r8","cl","ebx","bl"),("rcx","dl","ebx","bl"),("r8","cl","edx","dl"),\
    ("rbx","al","ecx","cl"),("rbx","cl","eax","al"),("rcx","al","ebx","bl"),\
    ("rax","dl","ebx","bl"),("rax","bl","ecx","cl"),("r8","al","ecx","cl"),\
    ("rdx","al","ebx","bl")])
    message="""
    %%define START_KEY 0x%08x
    %%define SECOND_KEY 0x%08x
    %%define INDEX %s
    %%define KEY_BYTE %s
    %%define SECOND_KEY_REG %s
    %%define SECOND_KEY_REG_BYTE %s
    """ % (start_key,second_key,INDEX,KEY_BYTE,SECOND_KEY_REG,SECOND_KEY_REG_BYTE)
    def_data = []
    for num in range(len(enc_mes_dwords)):
        message+= "%%define MESSAGE_%d 0x%08x\n" %(num,enc_mes_dwords[num])
        def_data.append("%%ifdef MESSAGE_%d" % num);
        def_data.append("    mov dword [rbp+%d],MESSAGE_%d" % (num*4,num));
        def_data.append("%endif");
    message += "%%define MESSAGE_LEN %d\n" % len(enc_mes)
    open(fname,'w+').write(message)

    cont = open("template_final.S",'r').read()
    message_init = "\n".join(def_data)
    cont=cont.replace("MESSAGE_PUT_ON_STACK",message_init)
    open("template_final.1.S",'w+').write(cont)


if __name__ == "__main__":

    pack_count = 30
    make_flag_inc("message.inc",0xCAFEBABE,"qweqw=eqwe","012345678901234567890123456789012345678901234567890123456789\n")
    compile_raw_file("template_final.1.S","out1.bin")
    raw_payload = open("out1.bin","rb").read()

    for i in range(pack_count):
        print(i)
        make_arith_pack_unpack_encode("template_arith.S","template_arith.1.S",raw_payload)
        compile_raw_file("template_arith.1.S","prog.tmp")
        raw_payload = open("prog.tmp","rb").read()

    make_arith_pack_unpack_encode("template_arith_main.S","template_arith_main.final.S",raw_payload)
    compile_elf_file("template_arith_main.final.S","prog")
