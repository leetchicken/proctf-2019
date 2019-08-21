#pragma once
#include "../hw/api.h"
#include "constants.h"
#include "icons_manager.h"

class PasswordWidget
{
public:
    enum EState
    {
        kStateHidden = 0,
        kStatePasswordEnter,
        kStateRequestSent,
        kStateDrawResult,
    };

    PasswordWidget(API* api, IconsManager& iconsMan)
        : m_api(api), m_iconsMan(iconsMan)
    {
        m_state = kStateHidden;
        m_backgroundRect = Rect(0, 0, kBackgroundWidth, kBackgroundHeight);
        m_loadingRect = Rect(220, 120, kInfoIconsWidth, kInfoIconsHeight);
        m_authKey = ~0u;
        m_upper = false;
        m_buttons = (Button*)m_api->Malloc(kButtonsCount * sizeof(Button));
        m_api->memset(m_buttons, 0, kButtonsCount * sizeof(Button));
        m_cursorTimer = 0.0f;
        m_api->memset(m_password, 0, sizeof(m_password));
        m_passwordLen = 0;
        m_request = NULL;
        InitButtons();
    }

    void InitButtons()
    {
        FontInfo fi;
        m_api->LCD_GetFontInfo(kFont24, &fi);

        uint32_t buttonIdx = 0;

        Rect rect;
        rect.width = kButtonSize;
        rect.height = kButtonSize;
        rect.x = 22;
        rect.y = 100;
        for(uint32_t i = 0; i < 10; i++)
        {
            char symbols[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
            m_buttons[buttonIdx].type = kButtonSymbol;
            m_buttons[buttonIdx].rect = rect;
            m_buttons[buttonIdx].lowerCase = symbols[i];
            m_buttons[buttonIdx].upperCase = symbols[i];
            buttonIdx++;
            rect.x += kButtonSize + kSpaceSize;
        }

        rect.x = 22;
        rect.y += kButtonSize + kSpaceSize;
        for(uint32_t i = 0; i < 10; i++)
        {
            char symbolsLower[] = {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'};
            char symbolsUpper[] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'};
            m_buttons[buttonIdx].type = kButtonSymbol;
            m_buttons[buttonIdx].rect = rect;
            m_buttons[buttonIdx].lowerCase = symbolsLower[i];
            m_buttons[buttonIdx].upperCase = symbolsUpper[i];
            buttonIdx++;
            rect.x += kButtonSize + kSpaceSize;
        }

        rect.x = 44;
        rect.y += kButtonSize + kSpaceSize;
        for(uint32_t i = 0; i < 9; i++)
        {
            char symbolsLower[] = {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l'};
            char symbolsUpper[] = {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'};
            m_buttons[buttonIdx].type = kButtonSymbol;
            m_buttons[buttonIdx].rect = rect;
            m_buttons[buttonIdx].lowerCase = symbolsLower[i];
            m_buttons[buttonIdx].upperCase = symbolsUpper[i];
            buttonIdx++;
            rect.x += kButtonSize + kSpaceSize;
        }

        // cancel button
        rect.x = 4;
        rect.width = 36;
        rect.height = 84;
        m_buttons[buttonIdx].type = kButtonCancel;
        m_buttons[buttonIdx].rect = rect;
        buttonIdx++;

        // enter button
        rect.x = 440;
        rect.width = 36;
        rect.height = 84;
        m_buttons[buttonIdx].type = kButtonEnter;
        m_buttons[buttonIdx].rect = rect;
        buttonIdx++;

        // caps lock
        rect.width = kButtonSize;
        rect.height = kButtonSize;
        rect.x = 44;
        rect.y += kButtonSize + kSpaceSize;
        m_buttons[buttonIdx].type = kButtonCapsLock;
        m_buttons[buttonIdx].rect = rect;
        buttonIdx++;
        rect.x += kButtonSize + kSpaceSize;

        for(uint32_t i = 0; i < 7; i++)
        {
            char symbolsLower[] = {'z', 'x', 'c', 'v', 'b', 'n', 'm'};
            char symbolsUpper[] = {'Z', 'X', 'C', 'V', 'B', 'N', 'M'};
            m_buttons[buttonIdx].type = kButtonSymbol;
            m_buttons[buttonIdx].rect = rect;
            m_buttons[buttonIdx].lowerCase = symbolsLower[i];
            m_buttons[buttonIdx].upperCase = symbolsUpper[i];
            buttonIdx++;
            rect.x += kButtonSize + kSpaceSize;
        }
        // backspace
        m_buttons[buttonIdx].type = kButtonBackspace;
        m_buttons[buttonIdx].rect = rect;
        buttonIdx++;

    }

    void Activate()
    {
        m_state = kStatePasswordEnter;
        m_api->memset(m_password, 0, sizeof(m_password));
        m_passwordLen = 0;
        m_result = false;
        m_cursorTimer = 0.0f;
    }

    void SetAuthKey(uint32_t authKey)
    {
        m_authKey = authKey;
    }

    void OnClick(uint16_t touchX, uint16_t touchY)
    {
        if(m_state == kStateRequestSent)
            return;

        for(uint32_t i = 0; i < kButtonsCount; i++)
        {
            Button& b = m_buttons[i];
            if(!b.rect.IsPointInside(touchX, touchY))
                continue;

            b.pressed = true;
                
            if(b.type == kButtonSymbol)
                AddSymbol(m_upper ? b.upperCase : b.lowerCase);
            else if(b.type == kButtonCapsLock)
                m_upper = !m_upper;
            else if(b.type == kButtonBackspace)                
                EraseLastSymbol();
            else if(b.type == kButtonCancel)
                m_state = kStateHidden;
            else if(b.type == kButtonEnter && m_passwordLen > 0)
            {
                RequestPasswordChange();
                if(m_request)
                    m_state = kStateRequestSent;
            }
        }        
    }

    void Update(float dt)
    {
        if(m_state == kStatePasswordEnter)
        {
            m_cursorTimer += dt;
            if(m_cursorTimer > 0.8f)
                m_cursorTimer = 0.0f;
        }
        else if(m_request && m_request->done)
        {
            m_result = m_request->succeed && m_request->statusCode == 200;

            m_api->FreeHTTPRequest(m_request);
            m_request = NULL;

            m_state = kStateDrawResult;
            m_drawResultTimer = 0.5f;
        }
        else if(m_state == kStateDrawResult)
        {
            m_drawResultTimer -= dt;
            if(m_drawResultTimer < 0.0f)
                m_state = kStateHidden;
        }
    }

    void OnRender()
    {
        m_api->LCD_DrawImage(m_backgroundRect, m_iconsMan.background, kBackgroundWidth * 4);

        m_api->LCD_SetFont(kFont24);
        m_api->LCD_SetTextColor(0xffffffff);

        FontInfo fi;
        m_api->LCD_GetFontInfo(kFont24, &fi);

        if(m_state == kStatePasswordEnter)
        {
            // head
            m_api->LCD_SetFont(kFont16);
            m_api->LCD_DisplayStringAt(8, 8, "Change password", kTextAlignNone);
            Rect line(8, 26, 180, 1);
            m_api->LCD_FillRect(line, 0xffffffff);
            m_api->LCD_SetFont(kFont24);

            // curson rendering
            uint32_t cursorOffsetX = (kBackgroundWidth - fi.charWidth * m_passwordLen) / 2;
            uint32_t cursorOffsetY = 50;

            Rect cursor;
            cursor.x = cursorOffsetX + fi.charWidth * m_passwordLen;
            cursor.y = cursorOffsetY;
            cursor.width = 2;
            cursor.height = fi.charHeight;

            if(m_cursorTimer < 0.4f)
                m_api->LCD_FillRect(cursor, 0xffffffff);

            // password rendering
            if(m_passwordLen)
                m_api->LCD_DisplayStringAt(cursorOffsetX, cursorOffsetY, m_password, kTextAlignNone);

            // buttons
            uint32_t symOffsetX = (kButtonSize - fi.charWidth) / 2;
            uint32_t symOffsetY = (kButtonSize - fi.charHeight) / 2;

            for(uint32_t i = 0; i < kButtonsCount; i++)
            {
                Button& b = m_buttons[i];
                uint32_t color = b.pressed ? 0xff303030 : 0xff7070ff;
                if(b.type == kButtonSymbol)
                {
                    m_api->LCD_FillRect(b.rect, color);
                    char symbol = m_upper ? b.upperCase : b.lowerCase;
                    m_api->LCD_DisplayChar(b.rect.x + symOffsetX, b.rect.y + symOffsetY, symbol);
                }
                else if(b.type == kButtonCapsLock)
                {
                    m_api->LCD_FillRect(b.rect, color);
                    m_api->LCD_DisplayChar(b.rect.x + symOffsetX, b.rect.y + symOffsetY, '^');
                }
                else if(b.type == kButtonBackspace)
                {
                    m_api->LCD_FillRect(b.rect, color);
                    symOffsetX = (b.rect.width - fi.charWidth * 2) / 2;
                    m_api->LCD_DisplayStringAt(b.rect.x + symOffsetX, b.rect.y + symOffsetY, "<-", kTextAlignNone);
                }
                else if(b.type == kButtonCancel)
                {
                    m_api->LCD_FillRect(b.rect, b.pressed ? 0xffff0000 : 0xffff7f7f);
                }
                else if(b.type == kButtonEnter)
                {
                    m_api->LCD_FillRect(b.rect, b.pressed ? 0xff00ff00 : 0xff7fff7f);
                }
                b.pressed = false;
            }
        }
        else if(m_state == kStateRequestSent)
        {
            m_api->LCD_DrawImageWithBlend(m_loadingRect, m_iconsMan.loadingIcon, kInfoIconsWidth * 4);
        }
        else if(m_state == kStateDrawResult)
        {
            m_api->LCD_DisplayStringAt(0, 130, m_result ? "OK" : "Fail", kTextAlignCenter);
        }
    }

    EState GetState() const
    {
        return m_state;
    }

    bool GetResult() const
    {
        return m_result;
    }

    const char* GetPassword() const
    {
        return m_password;
    }

private:

    enum EButtonType
    {
        kButtonSymbol = 0,
        kButtonCapsLock,
        kButtonBackspace,
        kButtonCancel,
        kButtonEnter
    };

    struct Button
    {
        EButtonType type;
        Rect rect;
        char lowerCase;
        char upperCase;
        bool pressed;
    };

    static const uint32_t kMaxPasswordLen = 16;
    static const uint32_t kButtonsCount = 40;
    static const uint32_t kButtonSize = 40;
    static const uint32_t kSpaceSize = 4;

    API* m_api;
    IconsManager& m_iconsMan;
    EState m_state;
    Rect m_backgroundRect;
    Rect m_loadingRect;
    uint32_t m_authKey;
    bool m_upper;
    Button* m_buttons;
    float m_cursorTimer;
    char m_password[kMaxPasswordLen + 1];
    uint32_t m_passwordLen;
    HTTPRequest* m_request;
    float m_drawResultTimer;
    bool m_result;

    void AddSymbol(char symbol)
    {
        if(m_passwordLen == kMaxPasswordLen)
            return;
        m_password[m_passwordLen] = symbol;
        m_passwordLen++;
    }

    void EraseLastSymbol()
    {
        if(m_passwordLen == 0)
            return;
        m_password[m_passwordLen - 1] = 0;
        m_passwordLen--;
    }

    void RequestPasswordChange()
    {
        m_request = m_api->AllocHTTPRequest();
        if(!m_request)
            return;
        m_request->httpMethod = kHttpMethodGet;
        m_api->sprintf(m_request->url, "http://%s:%u/change_password?auth=%x&p=%s", kServerIP, kServerPort, m_authKey, m_password);
        if(!m_api->SendHTTPRequest(m_request))
            m_api->FreeHTTPRequest(m_request);
    }
};