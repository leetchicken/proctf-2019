import contextlib
import traceback
import shutil
import uuid
import sys
import os


@contextlib.contextmanager
def unpack():
    dirr = str(uuid.uuid4())
    os.mkdir(dirr)
    try:
        yield f"{dirr}"
    except Exception as e:
        print(e, traceback.format_exc(), file=sys.stderr)
    finally:
        shutil.rmtree(dirr)