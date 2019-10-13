import geocacher_pb2
import base64
import struct
import os
import sys
import random
import string

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, padding, hmac
from cryptography.hazmat.primitives.asymmetric.padding import OAEP, MGF1
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.serialization import load_pem_private_key, load_pem_public_key


ADMIN_SESSION_SIZE = 4


def bin_format(n):
    data = hex(n)[2:]
    if len(data) % 2:
        data = "0" + data
    return ", ".join([hex(x) for x in bytes.fromhex(data)])

def bignum_to_bytes(n, rev=False):
    data = hex(n)[2:]
    data = "0" * (128 * 2 - len(data)) + data
    if rev:
        return bytes.fromhex(data)[::-1]
    else:
        return bytes.fromhex(data)

def aes_encryption(input, key, iv):
    padder = padding.PKCS7(128).padder()
    encryptor = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend()).encryptor()

    response = padder.update(input) + padder.finalize()
    response = encryptor.update(response) + encryptor.finalize()

    return response


def aes_decryption(input, key, iv):
    unpadder = padding.PKCS7(128).unpadder()
    decryptor = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend()).decryptor()

    data = decryptor.update(input) + decryptor.finalize()
    data = unpadder.update(data) + unpadder.finalize()

    return data


def hmacsha256(input, key):
    h = hmac.HMAC(key, hashes.SHA256(), backend=default_backend())
    h.update(input)
    return h.finalize()


protobuf_messages = {
    2: "StoreSecretRequest",
    3: "StoreSecretResponse",
    4: "GetSecretRequest",
    5: "GetSecretResponse",
    6: "DiscardSecretRequest",
    7: "DiscardSecretResponse",
    8: "GetLeastBusyCellRequest",
    9: "GetLeastBusyCellResponse",
    10: "ListAllBusyCellsRequest",
    11: "ListAllBusyCellsResponse",
    12: "AdminChallenge",
    13: "AdminResponse",
    14: "UnknownMessage",
    15: "AuthRequest",
    16: "AuthResponse",
    17: "AuthResult"
}


class DepositClient(object):
    def __init__(self):
        with open("service_public.pem", "rb") as f:
            self.pub_key = load_pem_public_key(f.read(), backend=default_backend())

        with open("checker_private.pem", "rb") as f:
            temp = load_pem_private_key(f.read(), password=None, backend=default_backend())
            self.admin_d = temp.private_numbers().d
            self.admin_n = temp.private_numbers().public_numbers.n

        self.key_3 = bytes([0] * 32)
        self.hmac1 = bytes([0] * 32)
        self.sequence_id = 0

    def print_rsa_key_for_C(self):
        print(f"unsigned char D[] = {{{bin_format(self.rsa_key.private_numbers().d)}}};")
        print(f"unsigned char E[] = {{{bin_format(self.rsa_key.private_numbers().public_numbers.e)}}};")
        print(f"unsigned char N[] = {{{bin_format(self.rsa_key.private_numbers().public_numbers.n)}}};")

    def serialize_message(self, msg):
        msg_id = msg.message_type
        flags = 2
        if msg_id not in (15, 16, 17):
            flags |= 1
        serialized_data = msg.SerializeToString()
        serialized_data_len = len(serialized_data)

        sequence_id = 0
        if flags & 2:
            self.sequence_id += 1
            sequence_id = self.sequence_id

        hmac_input = struct.pack("<IIIB", msg_id, sequence_id, msg_id,
                                 flags) + serialized_data[:260]

        if flags & 1:
            aes_iv = os.urandom(16)
            serialized_data = aes_encryption(serialized_data, self.session_key, aes_iv)
            serialized_data_len = len(serialized_data)
            hmac_input += aes_iv

        if flags & 2:
            hmac_output = hmacsha256(hmac_input, self.hmac1)

        else:
            hmac_output = hmacsha256(hmac_input, self.key_3)

        result = struct.pack("<IIBI32s", msg_id, sequence_id, flags,
                             serialized_data_len, hmac_output)

        if flags & 1:
            result += aes_iv
        result += serialized_data
        return result

    def deserialize_message(self, packet):
        msg_id, seq_id, flags, data_len = struct.unpack("<IIBI", packet[:13])
        curr = 13

        if flags & 1:
            iv = packet[curr:curr + 16]
            curr += 16
            data_len = data_len - (data_len & 0xF) + 16

        proto_data = packet[curr:curr+data_len]

        assert len(proto_data) == data_len

        if flags & 1:
            proto_data = aes_decryption(proto_data, self.session_key, iv)

        protobuf_message = getattr(geocacher_pb2, protobuf_messages[msg_id])()
        protobuf_message.ParseFromString(proto_data)
        return protobuf_message

    def wrap_packet(self, packet):
        return base64.b64encode(self.serialize_message(packet))

    def unwrap_packet(self, data):
        return self.deserialize_message(base64.b64decode(data))

    def handshake(self):
        self.client_A = os.urandom(16)
        self.client_B = os.urandom(16)
        encrypted = self.pub_key.encrypt(self.client_A + self.client_B,
                                         OAEP(mgf=MGF1(algorithm=hashes.SHA1()), algorithm=hashes.SHA1(), label=None))

        req = geocacher_pb2.AuthRequest()
        req.message_type = 15
        req.data = encrypted
        return self.wrap_packet(req)

    def handshake_response(self, data):
        message = self.unwrap_packet(data)
        authkey = message.auth_key
        ciphertext = authkey[:-16]
        iv = authkey[-16:]
        if aes_encryption(self.client_B, self.client_A, iv) == ciphertext:
            req = geocacher_pb2.AuthResult()
            req.message_type = 17
            req.status = geocacher_pb2.OK
            result = self.wrap_packet(req)
            self.hmac1 = hmacsha256(b"1" * 32, self.client_A)
            self.session_key = hmacsha256(b"2" * 32, self.client_A)
            self.sequence_id = 0

        else:
            req = geocacher_pb2.AuthResult()
            req.message_type = 17
            req.status = geocacher_pb2.FAIL
            result = self.wrap_packet(req)

        return result

    def store_secret(self, flag, location):
        req = geocacher_pb2.StoreSecretRequest()
        req.message_type = 2
        req.coordinates.lat, req.coordinates.lon = location
        req.secret = flag

        return self.wrap_packet(req)

    def get_secret(self, location, key):
        req = geocacher_pb2.GetSecretRequest()
        req.message_type = 4
        req.coordinates.lat, req.coordinates.lon = location
        req.key = key

        return self.wrap_packet(req)

    def discard_secret(self):
        req = geocacher_pb2.DiscardSecretRequest()
        req.message_type = 6
        req.coordinates.lat = 1234
        req.coordinates.lon = 5678
        req.key = b"\x00\x01\x02\x03\x04\x05\x06\x07"

        return self.wrap_packet(req)

    def parse_response(self, data):
        return self.unwrap_packet(data)

    def handle_response(self, data):
        print(self.parse_response(data))

    def list_busy_cells(self):
        req = geocacher_pb2.ListAllBusyCellsRequest()
        req.message_type = 10
        return self.wrap_packet(req)

    def handle_admin_challenge(self, data):
        data = self.unwrap_packet(data)
        full_challenge = self.session_key[:ADMIN_SESSION_SIZE] + data.data
        response = int((full_challenge + os.urandom(127 - len(full_challenge))).hex(), 16)
        signature = pow(response, self.admin_d, self.admin_n)
        data = bignum_to_bytes(signature, True)

        req = geocacher_pb2.AdminResponse()
        req.message_type = 13
        req.data = data

        return self.wrap_packet(req)

    def pick_location(self, rce_flag):
        if rce_flag:
            lat = random.randint(0, 2**32)
            lon = random.randint(0, 2**32)
            while lat < 2**31 and lon < 2**31:
                lat = random.randint(0, 2**32)
                lon = random.randint(0, 2**32)
        else:
            lat = random.randint(0, 2**31 - 1)
            lon = random.randint(0, 2**31 - 1)

        return lat, lon