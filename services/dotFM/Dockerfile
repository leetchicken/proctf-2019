FROM mcr.microsoft.com/dotnet/core/aspnet:3.0-alpine
WORKDIR /app
COPY ./out/ .
COPY start.sh .

ENV COMPlus_JitTelemetry=0 COMPlus_legacyCorruptedStateExceptionsPolicy=1 COMPlus_legacyUnhandledExceptionPolicy=1 LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password -u 10000 dotfm

ENTRYPOINT ["./start.sh"]
#ENTRYPOINT ["dotnet", "Uploader.dll"]