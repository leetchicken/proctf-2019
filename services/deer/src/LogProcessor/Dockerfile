FROM mcr.microsoft.com/dotnet/core/sdk:2.2 as build
WORKDIR /app

COPY *.csproj ./
RUN dotnet restore

COPY . ./
RUN dotnet publish -c Release -o out

FROM mcr.microsoft.com/dotnet/core/runtime:2.2 as runtime
WORKDIR /app
COPY --from=build /app/out .
#COPY bin/Release/netcoreapp2.2/publish/ app/
ENTRYPOINT ["dotnet", "LogProcessor.dll"]
