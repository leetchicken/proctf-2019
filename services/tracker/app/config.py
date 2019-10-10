import os
import yaml
from schematics.models import Model
from schematics.types import StringType, BooleanType, IntType

CONFIG_FILE = "config.yaml"
_CONFIG = None


class Config(Model):
    host = StringType(required=True)
    port = IntType(required=True)
    database_uri = StringType(required=True)
    debug = BooleanType(required=True)

    
def get_config() -> Config:
    global _CONFIG
    if _CONFIG is None:
        _CONFIG = Config(dict(
            host=os.environ["LISTEN_HOST"],
            port=os.environ["LISTEN_PORT"],
            database_uri="postgresql://{}@{}/{}".format(
                os.environ["DB_USER"],
                os.environ["DB_HOST"],
                os.environ["DB_NAME"],
            ),
            debug=os.environ.get("DEBUG", False)
        ))
        _CONFIG.validate()
    return _CONFIG
