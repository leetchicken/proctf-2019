#/bin/bash

cd `dirname $0`/src/bin/Release/netcoreapp2.2/publish >/dev/null 2>&1
exec dotnet notepoolchecker.dll "$@"
