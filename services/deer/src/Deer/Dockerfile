FROM mcr.microsoft.com/dotnet/core/sdk:2.2 as build
WORKDIR /app

COPY *.csproj ./
RUN dotnet restore

COPY . ./
RUN dotnet publish -c Release -o out

FROM mcr.microsoft.com/dotnet/core/aspnet:2.2 as runtime
WORKDIR /app
COPY --from=build /app/out .
RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password --disabled-login deer
USER deer
ENTRYPOINT ["dotnet", "Deer.dll"]
