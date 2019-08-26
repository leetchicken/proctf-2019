#/bin/bash

cd `dirname $0`/src >/dev/null 2>&1
exec dotnet run -c Release "$@"
