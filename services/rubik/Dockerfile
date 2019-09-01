FROM mcr.microsoft.com/dotnet/core/sdk:3.0 as build
WORKDIR /app

COPY ./rubik ./rubik
COPY ./rubikdb ./rubikdb

WORKDIR ./rubik

ENV DOTNET_CLI_TELEMETRY_OPTOUT=1

RUN dotnet restore
RUN dotnet publish -c Release -o out
RUN rm -rf ./out/*/

FROM mcr.microsoft.com/dotnet/core/aspnet:3.0 as runtime
WORKDIR /app

COPY --from=build /app/rubik/out .

ENV DOTNET_CLI_TELEMETRY_OPTOUT=1 COMPlus_JitTelemetry=0 COMPlus_legacyCorruptedStateExceptionsPolicy=1 COMPlus_legacyUnhandledExceptionPolicy=1

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password --disabled-login rubik
USER rubik

ENTRYPOINT ["dotnet", "rubik.dll"]
