

DATABASE_NAME = "fm_channels"
CHANNELS_COLLECTION = "channels"


class Storage:
    def __init__(self, client):
        self.__client = client
        self.connection = client[DATABASE_NAME]

    async def add_new_channel(self, channel_uuid):
        document = {"channel_uid": channel_uuid}
        result = await self.connection[CHANNELS_COLLECTION].insert_one(document)
        return result.inserted_id
