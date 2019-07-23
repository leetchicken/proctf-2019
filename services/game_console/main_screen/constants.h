#pragma once

static const uint32_t kGameIconWidth = 172;
static const uint32_t kGameIconHeight = 172;
static const uint32_t kGameIconSize = kGameIconWidth * kGameIconHeight * 4;
static const uint32_t kBackgroundWidth = 480;
static const uint32_t kBackgroundHeight = 272;
static const uint32_t kInfoIconsWidth = 40;
static const uint32_t kInfoIconsHeight = 40;
static const uint32_t kRefreshButtonWidth = 153;
static const uint32_t kRefreshButtonHeight = 32;
static const uint32_t kMaxGameIconsOnScreen = 3;
static const uint32_t kGameIconCacheSize = kMaxGameIconsOnScreen + 1;
static const uint32_t kMaxGamesCount = 256;
static const uint32_t kMaxGameCodeSize = 1024;
static const uint32_t kMaxGameAssetSize = 1024 * 1024;
static const char* kServerIP = "192.168.1.1";
static const uint32_t kServerPort = 8000;
static const uint32_t kServerNotifyPort = 8001;
static const uint32_t kServerChecksystemPort = 8002;
static const float kNotificationDrawTime = 5.0f;