#!/usr/bin/env python3
  
import sys
import requests
requests.packages.urllib3.disable_warnings() 
import lxml.html as lh

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

def trace(public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)

class IdentityServerHttpClient:
    def __init__(self, host):
        self._base_url = 'https://%s:5000/' % host

    def create_new_user(self, username, password):
        try:
            data = {'username': username, 'password': password}
            r = requests.post('%sAccount/SignUp' % self._base_url, data=data, allow_redirects=True, timeout=20, verify=False)

            if r.status_code != 200:
                trace("Bad HTTP status code", "Bad HTTP status code: %d at IdentityServerHttpClient.create_new_user()" % r.status_code)
                return MUMBLE, None

            if r.url != self._base_url:
                trace("Bad redirect url", "Bad redirect url '%s' at IdentityServerHttpClient.create_new_user()" % r.url)
                return MUMBLE, None

            doc = lh.fromstring(r.text)
            feedback_queue_elements = doc.xpath('//tr[2]/td[2]/text()')
            if not feedback_queue_elements:
                trace("Can't parse feedback queue", "Can't parse feedback queue at IdentityServerHttpClient.create_new_user()")
                return MUMBLE, None

            feedback_queue = feedback_queue_elements[0]
            return OK, feedback_queue
        except requests.exceptions.ConnectionError as e:
            trace("Connection error", "Connection error at IdentityServerHttpClient.create_new_user(): %s" % e)
            return DOWN, None
        except requests.exceptions.Timeout as e:
            trace("Timeout", "Timeout at IdentityServerHttpClient.create_new_user(): %s" % e)
            return DOWN, None
        except Exception as e:
            trace("Unknown error", "Unknown request error at IdentityServerHttpClient.create_new_user(): %s" % e)
            return CHECKER_ERROR, None
