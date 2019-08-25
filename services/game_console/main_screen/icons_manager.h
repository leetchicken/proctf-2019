#pragma once


struct IconsManager
{
    struct GameIcon
    {
        uint8_t* addr;
    };

    API* api;
    GameIcon gameIcons[kGameIconCacheSize];
    uint32_t freeGameIconIndices[kGameIconCacheSize];
    uint32_t freeGameIconIndicesNum;
    uint8_t* background;
    uint8_t* emptyGameIcon;
    uint8_t* loadingIcon;
    uint8_t* networkOffIcon;
    uint8_t* networkOnIcon;
    uint8_t* refreshButton;
    uint8_t* changePasswordButton;
    uint8_t* keyButton;
    uint8_t* capslockButton;
    uint8_t* backspaceButton;
    uint8_t* cancelButton;
    uint8_t* acceptButton;

    IconsManager()
    {
    }

    uint8_t* Init(API* api, uint8_t* sdram)
    {
        this->api = api;
        uint8_t* curSdram = sdram;
        for(uint32_t i = 0; i < kGameIconCacheSize; i++)
        {
            gameIcons[i].addr = curSdram;
            freeGameIconIndices[i] = i;
            curSdram += kGameIconSize;
        }
        freeGameIconIndicesNum = kGameIconCacheSize;

        background = curSdram;
        uint32_t backgroundSize = kBackgroundWidth * kBackgroundHeight * 4;
        curSdram = ReadIcon("/fs/background.bmp", background, backgroundSize);

        emptyGameIcon = curSdram;
        curSdram = ReadIcon("/fs/empty_icon.bmp", emptyGameIcon, kGameIconSize);

        uint32_t infoIconSize = kInfoIconsWidth * kInfoIconsHeight * 4;

        loadingIcon = curSdram;
        curSdram = ReadIcon("/fs/loading.bmp", loadingIcon, infoIconSize);

        networkOffIcon = curSdram;
        curSdram = ReadIcon("/fs/network_off.bmp", networkOffIcon, infoIconSize);

        networkOnIcon = curSdram;
        curSdram = ReadIcon("/fs/network_on.bmp", networkOnIcon, infoIconSize);

        refreshButton = curSdram;
        uint32_t buttonSize = kButtonWidth * kButtonHeight * 4;
        curSdram = ReadIcon("/fs/refresh.bmp", refreshButton, buttonSize);

        changePasswordButton = curSdram;
        curSdram = ReadIcon("/fs/account.bmp", changePasswordButton, buttonSize);

        keyButton = curSdram;
        uint32_t keyButtonSize = kKeyButtonWidth * kKeyButtonHeight * 4;
        curSdram = ReadIcon("/fs/button.bmp", keyButton, keyButtonSize);

        capslockButton = curSdram;
        curSdram = ReadIcon("/fs/capslock.bmp", capslockButton, keyButtonSize);

        backspaceButton = curSdram;
        curSdram = ReadIcon("/fs/backspace.bmp", backspaceButton, keyButtonSize);

        cancelButton = curSdram;
        uint32_t acceptCancelSize = kAcceptCancelButtonWidth * kAcceptCancelButtonHeight * 4;
        curSdram = ReadIcon("/fs/cancel.bmp", cancelButton, acceptCancelSize);

        acceptButton = curSdram;
        curSdram = ReadIcon("/fs/accept.bmp", acceptButton, acceptCancelSize);

        return curSdram;
    }

    uint32_t AllocateGameIcon()
    {
        if(!freeGameIconIndicesNum)
            return ~0u;
        
        uint32_t ret = freeGameIconIndices[freeGameIconIndicesNum - 1];
        freeGameIconIndicesNum--;
        return ret;
    }

    void FreeGameIcon(uint32_t idx)
    {
        freeGameIconIndices[freeGameIconIndicesNum] = idx;
        freeGameIconIndicesNum++;
    }

    void ClearGameIcons()
    {
        for(uint32_t i = 0; i < kGameIconCacheSize; i++)
            freeGameIconIndices[i] = i;
        freeGameIconIndicesNum = kGameIconCacheSize;
    }

private:
    uint8_t* ReadIcon(const char* fileName, uint8_t* dst, uint32_t size)
    {
        void* f = api->fopen(fileName, "r");
        api->fread(dst, size, f);
        api->fclose(f);
        return dst + size;
    }
};
