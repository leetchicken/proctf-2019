FROM mcr.microsoft.com/dotnet/core/aspnet:3.0-alpine
WORKDIR /app

COPY ./start.sh .
COPY ./out/ /app/

ENV DOTNET_CLI_TELEMETRY_OPTOUT=1 ASPNETCORE_ENVIRONMENT=Production DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=false
RUN apk add --no-cache icu-libs
ENV COMPlus_JitTelemetry=0 COMPlus_legacyCorruptedStateExceptionsPolicy=1 COMPlus_legacyUnhandledExceptionPolicy=1 LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password notepool

RUN mkdir -p /app/data
VOLUME /app/data

CMD ["./start.sh"]
