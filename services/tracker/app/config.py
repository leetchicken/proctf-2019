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
        with open(CONFIG_FILE) as f:
            raw_data = yaml.safe_load(f)
        _CONFIG = Config(raw_data)
        _CONFIG.validate()
    return _CONFIG
