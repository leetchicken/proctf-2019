#!/usr/bin/env python3
import base64

import Crypto.PublicKey.RSA
import Crypto.Cipher.PKCS1_v1_5
import re
import requests

TEAMS_COUNT = 20
PORTS_COUNT = 52
DEFAULT_VLAN = 1
ROUTER_PORTS = {47, 48}

DEFAULT_PASSWORD = "admin"
DEFAULT_IP = "10.90.90.90"

PASSWORD = "wsfy6g1remd49aug"
# IP = "10.90.90.91"


def ports():
    return range(1, 53)


def update_ports_membership(data, ports_u, ports_t):
    for port in ports():
        value = "0"
        if port in ports_u:
            value = "U"
        if port in ports_t:
            value = "T"
        data["C{}".format(port)] = value


class DGS1210Manager:
    def __init__(self, ip, password):
        self.ip = ip
        self.gambit = self.login(password)

    def _build_url(self, path):
        return "http://{}{}".format(self.ip, path)

    def _get(self, path):
        r = requests.get(self._build_url(path))
        assert r.status_code == 200
        return r.text

    def _post(self, path, data):
        r = requests.post(self._build_url(path), data=data)
        print(r.status_code, r.reason)
        assert r.status_code == 200
        return r.text

    def login(self, password, user="admin"):
        print("Login ... ", end="")

        en_data_js = self._get("/Encrypt.js")
        m = re.search("EN_DATA = '([^']+)'", en_data_js)
        assert m, "EN_DATA not found"

        public_key_str = base64.b64decode(m.group(1))
        public_key = Crypto.PublicKey.RSA.importKey(public_key_str)
        cipher = Crypto.Cipher.PKCS1_v1_5.new(public_key)

        encrypted_user = base64.b64encode(cipher.encrypt(user.encode())).decode()
        encrypted_pass = base64.b64encode(cipher.encrypt(password.encode())).decode()

        data = dict(
            pelican_ecryp=encrypted_user,
            pinkpanther_ecryp=encrypted_pass,
            BrowsingPage="index_redirect.htm",
            currlang="0",
            changlang="0"
        )
        gambit_html = self._post("/homepage.htm", data)

        m = re.search('Gambit=([^"]+)', gambit_html)
        assert m, "Gambit not found"
        return m.group(1)

    def change_password(self, old_pass, new_pass):
        print("Change password ... ", end="")
        data = dict(
            Gambit=self.gambit,
            FormName="pass_set",
            old_pass=old_pass,
            new_pass=new_pass,
            renew_pass=new_pass
        )
        self._post("/iss/specific/Password.js", data)

    def create_vlan(self, vlan_id, vlan_name, ports_u, ports_t):
        print("Creating VLAN {!r} ({!r}), U={!r}, T={!r} ... ".format(vlan_id, vlan_name, ports_u, ports_t), end="")
        data = dict(
            Gambit=self.gambit,
            FormName="formAddVLAN",
            # tag_id="1",
            VID=vlan_id,
            VlanName=vlan_name,
            over=""
        )
        update_ports_membership(data, ports_u, ports_t)
        self._post("/iss/specific/QVLAN.js", data)

    def configure_vlan(self, vlan_id, tag_id, vlan_name, ports_u, ports_t):
        print("Configuring VLAN {!r} ({!r}), U={!r}, T={!r} ... ".format(vlan_id, vlan_name, ports_u, ports_t), end="")
        data = dict(
            Gambit=self.gambit,
            FormName="formSetVID",
            # tag_id="1",
            VID=vlan_id,
            VlanName=vlan_name,
            over=""
        )
        update_ports_membership(data, ports_u, ports_t)
        self._post("/iss/specific/QVLAN.js", data)

    def delete_vlan(self, vlan_id):
        print("Deleting VLAN {!r} ... ".format(vlan_id), end="")
        data = dict(
            Gambit=self.gambit,
            FormName="formVLAN",
            VLAN_action="Delete",
            VID=vlan_id,
            vname="",
            over=""
        )
        self._post("/iss/specific/QVLAN.js", data)

    def set_pvid(self, pvid_list):
        print("Setting PVID for ports {!r}... ".format(pvid_list), end="")
        data = dict(
            Gambit=self.gambit,
            FormName="formSetPVID",
            Pvid=pvid_list
        )
        self._post("/iss/specific/QVLAN.js", data)

    def save(self):
        print("Saving configuration ... ", end="")
        self._post("/iss/specific/SaveConfig.js", dict(Gambit=self.gambit, SaveConfigID=1))


def main():
    manager = DGS1210Manager(DEFAULT_IP, DEFAULT_PASSWORD)
    manager.change_password(DEFAULT_PASSWORD, PASSWORD)

    manager = DGS1210Manager(DEFAULT_IP, PASSWORD)

    for n in list(range(100, 150)) + [200, 300, 400]:
        manager.delete_vlan(n)

    manager.configure_vlan(1, 0, "default", {49, 50, 51, 52}, set())

    for n in range(1, TEAMS_COUNT + 1):
        manager.create_vlan(vlan_id=100 + n, vlan_name="TEAM {}".format(n),
                            ports_u={n * 2 - 1, n * 2}, ports_t=ROUTER_PORTS)

    manager.create_vlan(vlan_id=131, vlan_name="TEAM 31 FAKE", ports_u={41, 42}, ports_t=ROUTER_PORTS)
    manager.create_vlan(vlan_id=200, vlan_name="ORGS", ports_u={43, 44}, ports_t=ROUTER_PORTS)
    manager.create_vlan(vlan_id=300, vlan_name="INTERNET", ports_u={45, 46}, ports_t=ROUTER_PORTS)

    # NOTE: Keep 'pvid_list' in sync with 'ports_u'!
    # PVID of port must be Untagged for that port
    pvid_list = []
    for team_id in range(1, TEAMS_COUNT + 1):  # Ports: 1..40
        pvid_list.append(100 + team_id)
        pvid_list.append(100 + team_id)
    pvid_list.append(131)   # Port 41
    pvid_list.append(131)   # Port 42
    pvid_list.append(200)   # Port 43
    pvid_list.append(200)   # Port 44
    pvid_list.append(300)   # Port 45
    pvid_list.append(300)   # Port 46
    while len(pvid_list) < PORTS_COUNT:
        pvid_list.append(DEFAULT_VLAN)

    manager.set_pvid(pvid_list)

    manager.save()


if __name__ == "__main__":
    main()
