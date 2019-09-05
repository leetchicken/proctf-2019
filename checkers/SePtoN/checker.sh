#/bin/bash

cd `dirname $0`/SePtoN_Checker/bin/Release/netcoreapp2.2/publish >/dev/null 2>&1
exec dotnet SePtoN_Checker.dll "$@"
