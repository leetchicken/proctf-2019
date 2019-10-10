#!/usr/bin/env python3

import paramiko,os

class RemoteVerifier:
    def __init__(self, ip, port,user,password):
        self.serv = (ip,port)
        self.user=user
        self.password = password
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    def init_host(self):
        self.client.connect(hostname=self.serv[0], username=self.user, \
                                password=self.password, port=self.serv[1])
        stdin, stdout, stderr = self.client.exec_command('ls')
        data = stdout.read() + stderr.read()

        self.client.close()

    def RunUserBinary(self,binpath,passwd):
        self.client.connect(hostname=self.serv[0], username=self.user, \
                                password=self.password, port=self.serv[1])

        ftp_client=self.client.open_sftp()
        fn = os.path.basename(binpath)
        ftp_client.put(binpath,"/home/test/%s" %fn)
        ftp_client.close()

        stdin, stdout, stderr = self.client.exec_command('chmod +x %s' %fn)

        stdin, stdout, stderr = self.client.exec_command('timeout -s KILL 7 ./wrapper %s %s' %(fn,passwd))
        data_run = stdout.read() + stderr.read()

        #stdin, stdout, stderr = self.client.exec_command('rm %s' %fn)

        self.client.close()
        return data_run
if __name__ == "__main__":
    verif = RemoteVerifier("localhost",3011,"test","sfdkjfds45a")
    a=verif.RunUserBinary("/home/awengar/Distr/Hacks/ORG/ProCTF/Binder/checker2/checker/message_packers/simple_1/mes2","qweqwe")
    print(a)
