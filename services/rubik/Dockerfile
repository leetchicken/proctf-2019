FROM mcr.microsoft.com/dotnet/core/aspnet:3.0-alpine
WORKDIR /app

COPY ./start.sh .
COPY ./out/ /app/

ENV DOTNET_CLI_TELEMETRY_OPTOUT=1 COMPlus_JitTelemetry=0 COMPlus_legacyCorruptedStateExceptionsPolicy=1 COMPlus_legacyUnhandledExceptionPolicy=1

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password rubik

RUN mkdir -p /app/data
VOLUME /app/data

CMD ["./start.sh"]

