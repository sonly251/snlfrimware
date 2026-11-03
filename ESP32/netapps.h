#ifndef _netapps_h
#define _netapps_h

#include <WiFi.h>
#include <time.h>
#include <string.h>
#include <Preferences.h>

// объявления функций основного скетча
void changeState(int state);

// 26x26 иконки для меню приложений/настроек
const unsigned char iconClock[] PROGMEM = {
 0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x7f, 0x80, 0x40, 
 0x81, 0x80, 0x60, 0x40, 0x82, 0x00, 0x10, 0x40, 0x84, 0x00, 0x08, 0x40, 0x88, 0x00, 0x24, 0x40, 
 0x88, 0x00, 0x44, 0x40, 0x90, 0x00, 0x82, 0x40, 0x90, 0x01, 0x02, 0x40, 0x90, 0x02, 0x02, 0x40, 
 0x90, 0x0c, 0x02, 0x40, 0x90, 0x0c, 0x02, 0x40, 0x90, 0x02, 0x02, 0x40, 0x90, 0x01, 0x02, 0x40, 
 0x90, 0x00, 0x82, 0x40, 0x88, 0x00, 0x44, 0x40, 0x88, 0x00, 0x04, 0x40, 0x84, 0x00, 0x08, 0x40, 
 0x82, 0x00, 0x10, 0x40, 0x81, 0x80, 0x60, 0x40, 0x80, 0x7f, 0x80, 0x40, 0x40, 0x00, 0x00, 0x80, 
 0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00
};
const unsigned char iconWifi[] PROGMEM = {
 0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x40, 
 0x80, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x80, 0xff, 0xc0, 0x40, 0x83, 0xff, 0xf0, 0x40, 
 0x8f, 0xff, 0xfc, 0x40, 0x9f, 0xc0, 0xfe, 0x40, 0xbf, 0x3f, 0x3f, 0x40, 0xbc, 0xff, 0xcf, 0x40, 
 0x9b, 0xff, 0xf6, 0x40, 0x87, 0xff, 0xf8, 0x40, 0x87, 0x80, 0x78, 0x40, 0x87, 0x3f, 0x38, 0x40, 
 0x80, 0x7f, 0x80, 0x40, 0x80, 0xff, 0xc0, 0x40, 0x80, 0xe1, 0xc0, 0x40, 0x80, 0x0c, 0x00, 0x40, 
 0x80, 0x1e, 0x00, 0x40, 0x80, 0x1e, 0x00, 0x40, 0x80, 0x0c, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 
 0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00
};

// ===================== ЧАСЫ =====================
#define CLOCK_CITY_COUNT 8

const char* const CLOCK_CITY_NAMES[CLOCK_CITY_COUNT] = {
  "NOVOSIBIRSK",
  "MOSCOW",
  "LONDON",
  "NEW YORK",
  "TOKYO",
  "DUBAI",
  "BEIJING",
  "LOS ANGELES"
};

const char* const CLOCK_TIMEZONES[CLOCK_CITY_COUNT] = {
  "NOVT-7",
  "MSK-3",
  "GMT0BST,M3.5.0/1,M10.5.0/2",
  "EST5EDT,M3.2.0,M11.1.0",
  "JST-9",
  "GST-4",
  "CST-8",
  "PST8PDT,M3.2.0,M11.1.0"
};

Preferences netPrefs;
bool netPrefsReady = false;
bool clockNtpSynced = false;
unsigned long clockNtpTryAt = 0;

// -1 = город ещё ни разу не выбирали.
int clockCityIndex = -1;
bool clockCityMenu = false;
int clockCityMenuIndex = -1;

void applyClockTimezone() {
  if (clockCityIndex < 0 || clockCityIndex >= CLOCK_CITY_COUNT) {
    clockCityIndex = 0;
  }

  setenv("TZ", CLOCK_TIMEZONES[clockCityIndex], 1);
  tzset();
  clockNtpSynced = false;
  clockNtpTryAt = 0;
}

void initNetworkStorage();

void initClock() {
  initNetworkStorage();

  int savedCity = netPrefs.getInt("clock_city", -1);
  if (savedCity < 0 || savedCity >= CLOCK_CITY_COUNT) {
    clockCityIndex = -1;
    clockCityMenu = true;       // первый вход — обязательно выбор города
  } else {
    clockCityIndex = savedCity;
    clockCityMenu = false;      // город уже выбран — сразу показываем часы
  }

  applyClockTimezone();
}

void saveClockCity(int index) {
  if (index < 0 || index >= CLOCK_CITY_COUNT) return;
  initNetworkStorage();
  clockCityIndex = index;
  netPrefs.putInt("clock_city", clockCityIndex);
  clockCityMenu = false;
  applyClockTimezone();
}

void drawClockCityMenu() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();

  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("SELECT CITY");
  setDrawColor();

  // Показываем 5 городов одновременно и прокручиваем список.
  static int first = 0;
  if (clockCityIndex >= 0 && clockCityIndex < CLOCK_CITY_COUNT) {
    first = clockCityIndex - 2;
    if (first < 0) first = 0;
    if (first > CLOCK_CITY_COUNT - 5) first = CLOCK_CITY_COUNT - 5;
  }

  for (int row = 0; row < 5; row++) {
    int idx = first + row;
    if (idx >= CLOCK_CITY_COUNT) break;

    display.setCursor(2, 15 + row * 9);
    display.print(idx == clockCityIndex ? "> " : "  ");
    display.print(CLOCK_CITY_NAMES[idx]);
  }


  display.display();
}

void runClockCityMenu() {
  if (clockCityMenuIndex < 0 || clockCityMenuIndex >= CLOCK_CITY_COUNT) {
    clockCityMenuIndex = (clockCityIndex >= 0 && clockCityIndex < CLOCK_CITY_COUNT)
                           ? clockCityIndex
                           : 0;
  }

  int &menuIndex = clockCityMenuIndex;

  if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_UP) == LOW) {
      menuIndex = (menuIndex - 1 + CLOCK_CITY_COUNT) % CLOCK_CITY_COUNT;
      lastButtonPress = millis();
    } else if (digitalRead(BTN_DOWN) == LOW) {
      menuIndex = (menuIndex + 1) % CLOCK_CITY_COUNT;
      lastButtonPress = millis();
    } else if (digitalRead(BTN_SELECT) == LOW) {
      saveClockCity(menuIndex);
      clockCityMenuIndex = menuIndex;
      lastButtonPress = millis();
      return;
    } else if (digitalRead(BTN_BACK) == LOW) {
      // Если город уже был выбран — возвращаемся к часам.
      // При первом входе без выбора — обратно в Apps.
      if (clockCityIndex >= 0) {
        clockCityMenu = false;
      } else {
        changeState(APPS_MENU);
      }
      lastButtonPress = millis();
      return;
    }
  }

  // Локальный индекс используется для выделения; рисуем его вручную,
  // чтобы UP/DOWN действительно двигали курсор.
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();

  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setCursor(2, 2);
  display.print("SELECT CITY");
  setDrawColor();

  int first = menuIndex - 2;
  if (first < 0) first = 0;
  if (first > CLOCK_CITY_COUNT - 5) first = CLOCK_CITY_COUNT - 5;

  for (int row = 0; row < 5; row++) {
    int idx = first + row;
    if (idx >= CLOCK_CITY_COUNT) break;
    display.setCursor(2, 15 + row * 9);
    display.print(idx == menuIndex ? "> " : "  ");
    display.print(CLOCK_CITY_NAMES[idx]);
  }
  display.display();
}

#define WIFI_SAVED_SSID_MAX 32
#define WIFI_SAVED_PASS_MAX 63
#define WIFI_SAVED_NETWORKS 5

void runClock() {
  // Первый вход после прошивки/сброса — выбор города.
  if (clockCityMenu) {
    runClockCityMenu();
    return;
  }

  if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
    // UP внутри Clock открывает выбор города.
    if (digitalRead(BTN_UP) == LOW) {
      clockCityMenu = true;
      clockCityMenuIndex = (clockCityIndex >= 0 && clockCityIndex < CLOCK_CITY_COUNT)
                           ? clockCityIndex
                           : 0;
      lastButtonPress = millis();
      return;
    }

    if (digitalRead(BTN_BACK) == LOW) {
      changeState(APPS_MENU);
      lastButtonPress = millis();
      return;
    }
  }

  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();

  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(CLOCK_CITY_NAMES[clockCityIndex]);
  setDrawColor();
  drawLine(0, 10, 128, 10);

  if (WiFi.status() != WL_CONNECTED) {
    display.setTextSize(1);
    display.setCursor(7, 24);
    display.print("WiFi: OFFLINE");
    display.setCursor(7, 38);
    display.print("CONNECT WiFi");
    display.setCursor(7, 52);
    display.print("UP: CITY  BACK: EXIT");
    display.display();
    return;
  }

  if (!clockNtpSynced && millis() - clockNtpTryAt > 500) {
    configTzTime(CLOCK_TIMEZONES[clockCityIndex],
                 "pool.ntp.org", "time.nist.gov", "time.google.com");
    clockNtpTryAt = millis();
    clockNtpSynced = true;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    display.setTextSize(1);
    display.setCursor(12, 25);
    display.print("SYNC...");
  } else {
    char timeStr[9];
    char dateStr[17];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);

    display.setTextSize(2);
    display.setCursor(2, 20);
    display.print(timeStr);

    display.setTextSize(1);
    display.setCursor(2, 40);
    display.print(dateStr);
  }

  display.display();
}

void initNetworkStorage() {
  if (!netPrefsReady) {
    netPrefs.begin("netapps", false);
    netPrefsReady = true;
  }
}

String getSavedWifiSSID(int index) {
  initNetworkStorage();
  if (index < 0 || index >= WIFI_SAVED_NETWORKS) return "";
  String key = "n" + String(index) + "_ssid";
  return netPrefs.getString(key.c_str(), "");
}

String getSavedWifiPass(int index) {
  initNetworkStorage();
  if (index < 0 || index >= WIFI_SAVED_NETWORKS) return "";
  String key = "n" + String(index) + "_pass";
  return netPrefs.getString(key.c_str(), "");
}

int getSavedWifiCount() {
  initNetworkStorage();
  int count = netPrefs.getInt("wifi_count", 0);
  if (count < 0) count = 0;
  if (count > WIFI_SAVED_NETWORKS) count = WIFI_SAVED_NETWORKS;

  if (count == 0) {
    String oldSsid = netPrefs.getString("ssid", "");
    if (oldSsid.length()) {
      String oldPass = netPrefs.getString("pass", "");
      netPrefs.putString("n0_ssid", oldSsid);
      netPrefs.putString("n0_pass", oldPass);
      netPrefs.putInt("wifi_count", 1);
      count = 1;
    }
  }
  return count;
}

void saveWifiCredentials(const char *ssid, const char *pass) {
  initNetworkStorage();

  String newSsid = String(ssid);
  String newPass = String(pass);
  if (!newSsid.length()) return;

  String ssids[WIFI_SAVED_NETWORKS];
  String passes[WIFI_SAVED_NETWORKS];
  int count = getSavedWifiCount();
  int out = 0;

  for (int i = 0; i < count && out < WIFI_SAVED_NETWORKS; i++) {
    String s = getSavedWifiSSID(i);
    String p = getSavedWifiPass(i);
    if (s.length() && s != newSsid) {
      ssids[out] = s;
      passes[out] = p;
      out++;
    }
  }

  netPrefs.putString("n0_ssid", newSsid);
  netPrefs.putString("n0_pass", newPass);

  int newCount = 1;
  for (int i = 0; i < out && newCount < WIFI_SAVED_NETWORKS; i++) {
    String sk = "n" + String(newCount) + "_ssid";
    String pk = "n" + String(newCount) + "_pass";
    netPrefs.putString(sk.c_str(), ssids[i]);
    netPrefs.putString(pk.c_str(), passes[i]);
    newCount++;
  }

  netPrefs.putInt("wifi_count", newCount);
  netPrefs.putBool("wifi_off", false);
}

bool isWifiDisabledByUser() {
  initNetworkStorage();
  return netPrefs.getBool("wifi_off", false);
}

void setWifiDisabledByUser(bool disabled) {
  initNetworkStorage();
  netPrefs.putBool("wifi_off", disabled);
}

void startSavedWifiIndex(int index) {
  String ssid = getSavedWifiSSID(index);
  String pass = getSavedWifiPass(index);
  if (!ssid.length()) return;

  setWifiDisabledByUser(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(ssid.c_str(), pass.c_str());
}

void startSavedWifi() {
  if (isWifiDisabledByUser()) {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    return;
  }

  int count = getSavedWifiCount();
  if (count <= 0) return;
  startSavedWifiIndex(0);
}

void disableWifiCompletely() {
  setWifiDisabledByUser(true);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, false);
  delay(20);
  WiFi.mode(WIFI_OFF);
}

// ===================== WI-FI SETUP =====================
char wifiKbLower[5][8] = {
  {'1','2','3','4','5','6','7','8'},
  {'9','0','q','w','e','r','t','y'},
  {'u','i','o','p','a','s','d','f'},
  {'g','h','j','k','l','z','x','c'},
  {'v','b','n','m',' ','<','^','@'}
};
char wifiKbUpper[5][8] = {
  {'1','2','3','4','5','6','7','8'},
  {'9','0','Q','W','E','R','T','Y'},
  {'U','I','O','P','A','S','D','F'},
  {'G','H','J','K','L','Z','X','C'},
  {'V','B','N','M',' ','<','^','@'}
};
char wifiKbSymbols[5][8] = {
  {'1','2','3','4','5','6','7','8'},
  {'9','0','!','@','#','$','%','&'},
  {'*','(',')','-','_','=','+',':'},
  {';',',','.','?','/','[',']','{'},
  {'}','~',' ','\'', ' ','<','^','@'}
};
int wifiKbMode = 0;
inline char (*wifiCurrentLayout())[8] {
  if (wifiKbMode == 1) return wifiKbUpper;
  if (wifiKbMode == 2) return wifiKbSymbols;
  return wifiKbLower;
}
int wifiPhase = 0;
// 6 = сканирование/список найденных Wi-Fi
int wifiScanCount = 0;
int wifiScanIndex = 0;
float wifiScanCursorY = 0.0f;
int wifiScanTop = 0;
int wifiSavedIndex = 0;
int wifiTopMenuIndex = 0;
#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 63
char wifiSSIDInput[WIFI_SSID_MAX + 1] = {0};
char wifiPassInput[WIFI_PASS_MAX + 1] = {0};
int wifiCursorX = 0, wifiCursorY = 0;
float wifiTopCursorY = 0.0f;
float wifiSavedCursorY = 0.0f;
float wifiSmoothX = 2.0f, wifiSmoothY = 28.0f;
int wifiTargetX = 0, wifiTargetY = 0;
unsigned long wifiLastAnim = 0;
unsigned long wifiConnectStart = 0;
String wifiResultMsg = "";

void initWifiSetup() {
  wifiPhase = 4;
  wifiKbMode = 0;
  wifiSavedIndex = 0;
  wifiScanCount = 0;
  wifiScanIndex = 0;
  wifiScanCursorY = 0.0f;
  wifiScanTop = 0;
  wifiTopMenuIndex = 0;
  wifiSSIDInput[0] = '\0';
  wifiPassInput[0] = '\0';
  wifiCursorX = 0;
  wifiCursorY = 0;
  wifiTargetX = 0;
  wifiTargetY = 0;
  wifiSmoothX = 2.0f;
  wifiSmoothY = 28.0f;
  wifiTopCursorY = 0.0f;
  wifiSavedCursorY = 0.0f;
  wifiLastAnim = millis();
}

// ---------- список в стиле SoundPad: шапка, "1. пункт", страницы по 4, плавная плашка ----------
static float wifiListY = 16.0f;
static int wifiListKind = -1;
static unsigned long wifiListAnimT = 0;

static void wifiListRow(int kind, int idx, int y, bool sel) {
  static const char* menuItems[3] = {"JOIN SAVED WIFI", "ADD NETWORK", "DISABLE WIFI"};
  uint16_t col = sel ? (whiteTheme ? WHITE : BLACK) : (whiteTheme ? BLACK : WHITE);
  if (sel) setInverseColor(); else setDrawColor();
  display.setCursor(10, y);
  display.print(idx + 1);
  display.print(". ");
  if (kind == 0) {
    display.print(menuItems[idx]);
  } else if (kind == 1) {
    String ssid = getSavedWifiSSID(idx);
    if (ssid.length() > 13) ssid = ssid.substring(0, 13);
    display.print(ssid);
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == getSavedWifiSSID(idx))
      display.fillCircle(116, y + 3, 2, col);          // текущая сеть
  } else {
    String ssid = WiFi.SSID(idx);
    if (!ssid.length()) ssid = "<HIDDEN>";
    if (ssid.length() > 13) ssid = ssid.substring(0, 13);
    display.print(ssid);
    if (WiFi.encryptionType(idx) != WIFI_AUTH_OPEN) {  // защищённая сеть
      display.setCursor(114, y);
      display.print("*");
    }
  }
}

static void wifiDrawList(const char* title, int count, int sel, int kind) {
  int target = 16 + (sel % 4) * 12;
  if (wifiListKind != kind) { wifiListKind = kind; wifiListY = (float)target; }
  if (millis() - wifiListAnimT > 16) {
    wifiListAnimT = millis();
    wifiListY += (target - wifiListY) * 0.35f;
  }

  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setCursor(2, 2);
  display.print(title);
  int pages = (count + 3) / 4;
  if (pages > 1) {
    String pg = String(sel / 4 + 1) + "/" + String(pages);
    display.setCursor(126 - (int)pg.length() * 6, 2);
    display.print(pg);
  }
  setDrawColor();
  drawLine(0, 10, 128, 10);

  if (count <= 0) {
    display.setCursor(10, 30);
    display.print(kind == 2 ? "NO NETWORKS" : "NO SAVED WIFI");
    display.display();
    return;
  }

  int page = sel / 4;
  for (int i = 0; i < 4; i++) {
    int idx = page * 4 + i;
    if (idx >= count) break;
    wifiListRow(kind, idx, 16 + i * 12, false);
  }
  drawRect(5, (int)wifiListY - 1, 118, 10, true);
  wifiListRow(kind, sel, 16 + (sel % 4) * 12, true);
  setDrawColor();
  display.display();
}

void runWifiSetup() {
  // ---------------- INPUT ----------------
  if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_BACK) == LOW) {
      if (wifiPhase == 5) {
        wifiPhase = 4;
        wifiSavedIndex = 0;
      } else if (wifiPhase == 6) {
        WiFi.scanDelete();
        wifiPhase = 4;
        wifiScanIndex = 0;
        wifiScanTop = 0;
        wifiScanCursorY = 0.0f;
      } else if (wifiPhase == 4 || wifiPhase == 3 || wifiPhase == 2) {
        changeState(SETTINGS_MENU);
      } else if (wifiPhase == 1) {
        // После выбора сети BACK возвращает к списку найденных сетей.
        wifiPhase = 6;
        wifiPassInput[0] = '\0';
        wifiKbMode = 0;
        wifiCursorX = wifiCursorY = 0;
        wifiTargetX = wifiTargetY = 0;
        wifiSmoothX = 2.0f;
        wifiSmoothY = 28.0f;
      } else {
        wifiPhase = 4;
      }
      lastButtonPress = millis();
      return;
    }

    if (wifiPhase == 3) {
      if (digitalRead(BTN_SELECT) == LOW) {
        // Возврат в меню с курсором наверху.
        wifiTopMenuIndex = 0;
        wifiTopCursorY = 0.0f;
        wifiPhase = 4;
        lastButtonPress = millis();
      }
    } else if (wifiPhase == 4) {
      bool moved = false;
      if (digitalRead(BTN_UP) == LOW) {
        wifiTopMenuIndex = (wifiTopMenuIndex > 0) ? wifiTopMenuIndex - 1 : 2; moved = true;
      }
      if (digitalRead(BTN_DOWN) == LOW) {
        wifiTopMenuIndex = (wifiTopMenuIndex < 2) ? wifiTopMenuIndex + 1 : 0; moved = true;
      }
      if (moved) lastButtonPress = millis();

      if (digitalRead(BTN_SELECT) == LOW) {
        if (wifiTopMenuIndex == 0) {
          wifiPhase = 5;
          wifiSavedIndex = 0;
          wifiSavedCursorY = 0.0f;
        } else if (wifiTopMenuIndex == 1) {
          // Сканируем сети и показываем найденные SSID.
          wifiPhase = 6;
          wifiScanIndex = 0;
          wifiScanTop = 0;
          wifiScanCursorY = 0.0f;
          wifiSSIDInput[0] = '\0';
          wifiPassInput[0] = '\0';
          setWifiDisabledByUser(false);
          WiFi.mode(WIFI_STA);
          WiFi.disconnect(false, false);
          delay(50);
          wifiScanCount = WiFi.scanNetworks(false, true);
          if (wifiScanCount < 0) wifiScanCount = 0;
        } else {
          disableWifiCompletely();
          wifiResultMsg = "WiFi disabled.";
          // После disable — курсор плавно едет наверх.
          wifiTopMenuIndex = 0;
          wifiPhase = 3;
        }
        lastButtonPress = millis();
      }
    } else if (wifiPhase == 6) {
      if (wifiScanCount <= 0) {
        wifiPhase = 4;
      } else {
        bool moved = false;
        if (digitalRead(BTN_UP) == LOW) {
          wifiScanIndex = (wifiScanIndex > 0) ? wifiScanIndex - 1 : wifiScanCount - 1; moved = true;
        }
        if (digitalRead(BTN_DOWN) == LOW) {
          wifiScanIndex = (wifiScanIndex < wifiScanCount - 1) ? wifiScanIndex + 1 : 0; moved = true;
        }
        if (moved) {
          if (wifiScanIndex < wifiScanTop) wifiScanTop = wifiScanIndex;
          if (wifiScanIndex >= wifiScanTop + 4) wifiScanTop = wifiScanIndex - 3;
          lastButtonPress = millis();
        }

        if (digitalRead(BTN_SELECT) == LOW) {
          String ssid = WiFi.SSID(wifiScanIndex);
          ssid.toCharArray(wifiSSIDInput, WIFI_SSID_MAX + 1);
          wifiPassInput[0] = '\0';
          wifiKbMode = 0;
          wifiCursorX = wifiCursorY = 0;
          wifiTargetX = wifiTargetY = 0;
          wifiSmoothX = 2.0f;
          wifiSmoothY = 28.0f;
          // Открытую сеть подключаем без пароля, защищённую — через ввод пароля.
          bool openNetwork = (WiFi.encryptionType(wifiScanIndex) == WIFI_AUTH_OPEN);
          WiFi.scanDelete();
          if (openNetwork) {
            WiFi.begin(wifiSSIDInput, "");
            wifiConnectStart = millis();
            wifiPhase = 2;
          } else {
            wifiPhase = 1;
          }
          lastButtonPress = millis();
        }
      }
    } else if (wifiPhase == 5) {
      int savedCount = getSavedWifiCount();
      if (savedCount <= 0) {
        wifiPhase = 4;
      } else {
        bool moved = false;
        if (digitalRead(BTN_UP) == LOW) {
          wifiSavedIndex = (wifiSavedIndex > 0) ? wifiSavedIndex - 1 : savedCount - 1; moved = true;
        }
        if (digitalRead(BTN_DOWN) == LOW) {
          wifiSavedIndex = (wifiSavedIndex < savedCount - 1) ? wifiSavedIndex + 1 : 0; moved = true;
        }
        if (moved) lastButtonPress = millis();

        if (digitalRead(BTN_SELECT) == LOW) {
          String ssid = getSavedWifiSSID(wifiSavedIndex);
          String pass = getSavedWifiPass(wifiSavedIndex);
          ssid.toCharArray(wifiSSIDInput, WIFI_SSID_MAX + 1);
          pass.toCharArray(wifiPassInput, WIFI_PASS_MAX + 1);
          setWifiDisabledByUser(false);
          WiFi.mode(WIFI_STA);
          WiFi.setAutoReconnect(true);
          WiFi.persistent(false);
          WiFi.begin(wifiSSIDInput, wifiPassInput);
          wifiConnectStart = millis();
          wifiPhase = 2;
          lastButtonPress = millis();
        }
      }
    } else if (wifiPhase == 0 || wifiPhase == 1) {
      bool moved = false;
      if (digitalRead(BTN_UP) == LOW && wifiTargetY > 0) { wifiTargetY--; moved = true; }
      if (digitalRead(BTN_DOWN) == LOW && wifiTargetY < 4) { wifiTargetY++; moved = true; }
      if (digitalRead(BTN_LEFT) == LOW && wifiTargetX > 0) { wifiTargetX--; moved = true; }
      if (digitalRead(BTN_RIGHT) == LOW && wifiTargetX < 7) { wifiTargetX++; moved = true; }

      if (moved) {
        wifiCursorX = wifiTargetX;
        wifiCursorY = wifiTargetY;
        lastButtonPress = millis();
      }

      if (digitalRead(BTN_SELECT) == LOW) {
        char sel = wifiCurrentLayout()[wifiTargetY][wifiTargetX];
        char *field = (wifiPhase == 0) ? wifiSSIDInput : wifiPassInput;
        size_t maxLen = (wifiPhase == 0) ? WIFI_SSID_MAX : WIFI_PASS_MAX;
        size_t len = strlen(field);

        if (sel == '<') {
          if (len > 0) field[len - 1] = '\0';
        } else if (sel == '^') {
          wifiKbMode = (wifiKbMode + 1) % 3;
        } else if (sel == '@') {
          if (wifiPhase == 0 && wifiSSIDInput[0] != '\0') {
            wifiPhase = 1;
            wifiKbMode = 0;
            wifiCursorX = wifiCursorY = 0;
            wifiTargetX = wifiTargetY = 0;
            wifiSmoothX = 2.0f;
            wifiSmoothY = 28.0f;
          } else if (wifiPhase == 1) {
            wifiSSIDInput[WIFI_SSID_MAX] = '\0';
            wifiPassInput[WIFI_PASS_MAX] = '\0';
            WiFi.mode(WIFI_STA);
            WiFi.disconnect(false, false);
            delay(20);
            WiFi.begin(wifiSSIDInput, wifiPassInput);
            wifiConnectStart = millis();
            wifiPhase = 2;
          }
        } else if (len < maxLen) {
          field[len] = sel;
          field[len + 1] = '\0';
        }
        lastButtonPress = millis();
      }
    }
  }

  // ---------------- CONNECTION STATE ----------------
  if (wifiPhase == 2) {
    if (WiFi.status() == WL_CONNECTED) {
      saveWifiCredentials(wifiSSIDInput, wifiPassInput);
      wifiResultMsg = String("Connected\nIP: ") + WiFi.localIP().toString();
      wifiPhase = 3;
      clockNtpSynced = false;
    } else if (millis() - wifiConnectStart > 15000) {
      WiFi.disconnect(true);
      wifiResultMsg = "Connection failed.\nCheck password.";
      wifiPhase = 3;
    }
  }

  // ---------------- SMOOTH CURSORS ----------------
  // Плавное движение курсора меню (обновляется и в phase==3, чтобы к моменту
  // возврата в phase==4 курсор уже подъехал наверх).
  if (millis() - wifiLastAnim > 16) {
    wifiLastAnim = millis();
    float targetPixelX = 2 + wifiTargetX * 15;
    float targetPixelY = 28 + wifiTargetY * 7;
    wifiSmoothX += (targetPixelX - wifiSmoothX) * 0.20f;
    wifiSmoothY += (targetPixelY - wifiSmoothY) * 0.20f;
  }

  // ---------------- DRAW ----------------
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();

  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("WIFI");
  setDrawColor();
  drawLine(0, 10, 128, 10);

  if (wifiPhase == 3) {
    int y = 24;
    int start = 0;
    for (int i = 0; i <= (int)wifiResultMsg.length(); i++) {
      if (i == (int)wifiResultMsg.length() || wifiResultMsg[i] == '\n') {
        display.setCursor(6, y);
        display.print(wifiResultMsg.substring(start, i));
        y += 10;
        start = i + 1;
      }
    }
    display.display();
    return;
  }

  if (wifiPhase == 2) {
    display.setCursor(10, 25);
    display.print("Connecting");
    int dots = (millis() / 350) % 4;
    for (int i = 0; i < dots; i++) display.print(".");
    display.setCursor(10, 39);
    display.print(wifiSSIDInput);
    display.display();
    return;
  }

  // ---------- СПИСКИ В СТИЛЕ SOUNDPAD ----------
  if (wifiPhase == 4) {
    wifiDrawList("WIFI", 3, wifiTopMenuIndex, 0);
    return;
  }
  if (wifiPhase == 6) {
    if (wifiScanCount <= 0) { wifiDrawList("SELECT WIFI", 0, 0, 2); return; }
    wifiDrawList("SELECT WIFI", wifiScanCount, wifiScanIndex, 2);
    return;
  }
  if (wifiPhase == 5) {
    wifiDrawList("SAVED NETWORKS", getSavedWifiCount(), wifiSavedIndex, 1);
    return;
  }

  // ---------------- KEYBOARD ----------------
  uint16_t fg = whiteTheme ? BLACK : WHITE;
  uint16_t bg = whiteTheme ? WHITE : BLACK;

  display.setTextColor(fg, bg);
  display.setCursor(2, 14);
  display.print(wifiPhase == 0 ? "SSID: " : "PASS: ");
  if (wifiPhase == 0) display.print(wifiSSIDInput);
  else for (size_t i = 0; i < strlen(wifiPassInput); i++) display.print("*");
  if ((millis() / 500) % 2 == 0) display.print("_");

  drawLine(0, 22, 128, 22);

  char (*layout)[8] = wifiCurrentLayout();

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 8; x++) {
      int btnX = 2 + x * 15;
      int btnY = 28 + y * 7;
      display.setTextColor(fg, bg);
      display.setCursor(btnX, btnY);
      display.print(layout[y][x]);
    }
  }

  int curX = (int)roundf(wifiSmoothX) - 1;
  int curY = (int)roundf(wifiSmoothY) - 1;
  display.fillRect(curX, curY, 15, 8, fg);
  display.setTextColor(bg, fg);
  display.setCursor((int)roundf(wifiSmoothX), (int)roundf(wifiSmoothY));
  display.print(layout[wifiTargetY][wifiTargetX]);

  display.setTextColor(fg, bg);
  const char* modeLabel = wifiKbMode == 0 ? "abc" : (wifiKbMode == 1 ? "ABC" : "#$%");
  display.setCursor(102, 14);
  display.print(modeLabel);

  display.display();
}

#endif