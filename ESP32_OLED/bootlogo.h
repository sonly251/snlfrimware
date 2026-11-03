#ifndef _bootlogo_h
#define _bootlogo_h

// ============================================================
// Boot Logo Manager
// - Загрузочный экран выбирается ТОЛЬКО из файлов SD-карты, папка /pictures
//   (те самые рисунки, которые сохраняет PAINT: *.bin, 1024 байта, 128x64, 1 бит).
// - SD-карта делит GPIO23 с динамиком и при старте не смонтирована, поэтому
//   выбранный файл копируется во внутреннюю память (LittleFS) как /bootlogo_sd.bin,
//   а имя файла запоминается в Preferences. Логотип работает и без карты.
// - Загрузки по Wi-Fi больше нет.
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>

void changeState(int state);
void startSavedWifi();

// из filemanager.h (подключается позже)
static bool fmSdMount();
static void fmSdUnmount();

#define BOOT_LOGO_BYTES  1024
#define BOOT_LOGO_DIR    "/pictures"
#define BOOT_LOGO_CACHE  "/bootlogo_sd.bin"

// Точка доступа — используется также проводником и PAINT
const char BL_AP_SSID[] = "SNL-GAMING";
const char BL_AP_PASS[] = "12345678";

// Иконка пункта меню (26x26)
const unsigned char iconBootLogo[] PROGMEM = { 0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x87, 0xff, 0xf8, 0x40, 0x84, 0x00, 0x08, 0x40, 0x84, 0x00, 0x08, 0x40, 0x84, 0x00, 0xc8, 0x40, 0x84, 0x01, 0xe8, 0x40, 0x84, 0x00, 0xc8, 0x40, 0x84, 0x20, 0x08, 0x40, 0x84, 0x50, 0x08, 0x40, 0x84, 0x49, 0x08, 0x40, 0x84, 0x8a, 0x88, 0x40, 0x85, 0x04, 0x48, 0x40, 0x85, 0x00, 0x28, 0x40, 0x86, 0x00, 0x18, 0x40, 0x87, 0xff, 0xf8, 0x40, 0x80, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00 };


uint8_t customBootLogo[BOOT_LOGO_BYTES];
bool customBootLogoLoaded = false;
bool blFsReady = false;

Preferences bootLogoPrefs;
bool bootLogoPrefsReady = false;
char bootLogoName[32] = "";     // имя файла с SD, из которого взят текущий логотип

void initBootLogoPrefs() {
  if (!bootLogoPrefsReady) {
    bootLogoPrefs.begin("bootlogo", false);
    bootLogoPrefsReady = true;
  }
}

bool blMountFs(bool formatIfNeeded) {
  if (blFsReady) return true;
  blFsReady = LittleFS.begin(formatIfNeeded);
  return blFsReady;
}

// Загружает сохранённый логотип (копию во flash). Вызывается при старте.
void loadCustomBootLogo() {
  initBootLogoPrefs();

  customBootLogoLoaded = false;
  bootLogoName[0] = 0;

  String nm = bootLogoPrefs.getString("name", "");
  if (nm.length() == 0 || nm.length() >= sizeof(bootLogoName)) return;
  if (!blMountFs(false)) return;

  File f = LittleFS.open(BOOT_LOGO_CACHE, "r");
  if (!f) return;

  if (f.size() == BOOT_LOGO_BYTES && f.read(customBootLogo, BOOT_LOGO_BYTES) == BOOT_LOGO_BYTES) {
    customBootLogoLoaded = true;
    strcpy(bootLogoName, nm.c_str());
  }
  f.close();
}

void resetCustomBootLogo() {
  customBootLogoLoaded = false;
  bootLogoName[0] = 0;
  initBootLogoPrefs();
  bootLogoPrefs.remove("name");
  if (blMountFs(false)) LittleFS.remove(BOOT_LOGO_CACHE);
}

// Рисует текущий boot-логотип (выбранный или встроенный)
void drawBootLogo(uint16_t color) {
  if (customBootLogoLoaded) display.drawBitmap(0, 0, customBootLogo, 128, 64, color);
  else                      display.drawBitmap(0, 0, bootLogo, 128, 64, color);
}

// ---------- работа с папкой /pictures на SD ----------
#define BL_ROWS   4      // строк на экране
#define BL_CACHE  16     // сколько имён держим в памяти (остальные подгружаются при прокрутке)

// как в PAINT: простые имена *.bin без папок и спецсимволов
static bool blValidName(const char* n) {
  size_t l = strlen(n);
  if (l < 5 || l > 31 || n[0] == '.') return false;
  for (size_t i = 0; i < l; i++) {
    char c = n[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-';
    if (!ok) return false;
  }
  return strcasecmp(n + l - 4, ".bin") == 0;
}

static const char* blBaseName(const char* full) {
  const char* s = strrchr(full, '/');
  return s ? s + 1 : full;
}

static bool blEntryOk(File& f, const char* nm) {
  return !f.isDirectory() && f.size() == BOOT_LOGO_BYTES && blValidName(nm);
}

static bool blReadSd(const char* name, uint8_t* out) {
  char path[64];
  snprintf(path, sizeof(path), "%s/%s", BOOT_LOGO_DIR, name);
  File f = SD.open(path, "r");
  if (!f) return false;
  bool ok = !f.isDirectory() && f.size() == BOOT_LOGO_BYTES && f.read(out, BOOT_LOGO_BYTES) == BOOT_LOGO_BYTES;
  f.close();
  return ok;
}

// Копирует файл с SD во flash и делает его загрузочным экраном.
static bool blApplyFromSd(const char* name) {
  uint8_t tmp[BOOT_LOGO_BYTES];
  if (!blReadSd(name, tmp)) return false;
  if (!blMountFs(true)) return false;

  bool ok = false;
  File o = LittleFS.open(BOOT_LOGO_CACHE, "w");
  if (o) {
    ok = (o.write(tmp, BOOT_LOGO_BYTES) == BOOT_LOGO_BYTES);
    o.close();
  }

  if (!ok) {                       // старая копия уже затёрта — откатываемся на встроенный
    resetCustomBootLogo();
    return false;
  }

  memcpy(customBootLogo, tmp, BOOT_LOGO_BYTES);
  customBootLogoLoaded = true;
  strncpy(bootLogoName, name, sizeof(bootLogoName) - 1);
  bootLogoName[sizeof(bootLogoName) - 1] = 0;
  initBootLogoPrefs();
  bootLogoPrefs.putString("name", bootLogoName);
  return true;
}

// ---------- экран BOOT LOGO ----------
#define BL_MENU    0
#define BL_SELECT  1
#define BL_PREVIEW 3
#define BL_CONFIRM 4

int blPhase = BL_MENU;
int blSel = 0;                    // в меню — пункт; в списке — номер файла
unsigned long blLastDraw = 0;
bool blDirty = true;
const char* blItems[2] = { "CHOOSE FROM SD", "RESET TO DEFAULT" };

const char* blMsg = "";
unsigned long blMsgUntil = 0;
bool blMsgShown = false;

bool blSdOk = false;
int  blCount = 0;                 // сколько подходящих файлов в /pictures
int  blTop = 0;                   // номер первого видимого файла
int  blFirst = 0;                 // номер файла blNames[0]
int  blCached = 0;                // сколько имён сейчас в blNames
int  blActiveIdx = -1;            // номер файла, который сейчас стоит на загрузке
char blNames[BL_CACHE][32];

static void blSetMsg(const char* m, unsigned long ms) {
  blMsg = m;
  blMsgUntil = millis() + ms;
  blDirty = true;
}

// имя текущего логотипа для экрана (без .bin, не длиннее 16 символов)
static const char* blCurrentName() {
  static char name[18];
  if (!customBootLogoLoaded || !bootLogoName[0]) return "DEFAULT";
  size_t l = strlen(bootLogoName);
  if (l > 4 && strcasecmp(bootLogoName + l - 4, ".bin") == 0) l -= 4;
  if (l > 16) l = 16;
  memcpy(name, bootLogoName, l);
  name[l] = 0;
  return name;
}

// считаем файлы и ищем тот, что сейчас активен
static int blScanCount() {
  blActiveIdx = -1;
  File dir = SD.open(BOOT_LOGO_DIR);
  if (!dir) return 0;
  int n = 0;
  if (dir.isDirectory()) {
    for (int guard = 0; guard < 20000; guard++) {
      File f = dir.openNextFile();
      if (!f) break;
      const char* nm = blBaseName(f.name());
      if (blEntryOk(f, nm)) {
        if (customBootLogoLoaded && strcmp(nm, bootLogoName) == 0) blActiveIdx = n;
        n++;
      }
      f.close();
    }
  }
  dir.close();
  return n;
}

// загружаем в blNames до BL_CACHE имён, начиная с файла номер first
static void blLoadWindow(int first) {
  blFirst = first;
  blCached = 0;
  File dir = SD.open(BOOT_LOGO_DIR);
  if (!dir) return;
  if (dir.isDirectory()) {
    int idx = 0;
    for (int guard = 0; guard < 20000 && blCached < BL_CACHE; guard++) {
      File f = dir.openNextFile();
      if (!f) break;
      const char* nm = blBaseName(f.name());
      if (blEntryOk(f, nm)) {
        if (idx >= first) {
          strncpy(blNames[blCached], nm, 31);
          blNames[blCached][31] = 0;
          blCached++;
        }
        idx++;
      }
      f.close();
    }
  }
  dir.close();
}

// прокрутка: подгоняем blTop под выбор и при необходимости подгружаем имена
static void blEnsureWindow() {
  if (blSel < blTop) blTop = blSel;
  if (blSel >= blTop + BL_ROWS) blTop = blSel - BL_ROWS + 1;
  int maxTop = blCount - BL_ROWS;
  if (maxTop < 0) maxTop = 0;
  if (blTop > maxTop) blTop = maxTop;
  if (blTop < 0) blTop = 0;

  int needEnd = blTop + BL_ROWS;
  if (needEnd > blCount) needEnd = blCount;
  if (blTop < blFirst || needEnd > blFirst + blCached) {
    int first = blTop - (BL_CACHE - BL_ROWS) / 2;
    int maxFirst = blCount - BL_CACHE;
    if (maxFirst < 0) maxFirst = 0;
    if (first > maxFirst) first = maxFirst;
    if (first < 0) first = 0;
    blLoadWindow(first);
  }
}

static void blEnterSelect() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);
  display.setCursor(2, 28);
  display.print("READING SD...");
  display.display();

  blSdOk = fmSdMount();
  blCount = 0; blCached = 0; blFirst = 0; blTop = 0; blActiveIdx = -1;
  if (blSdOk) blCount = blScanCount();
  blSel = (blActiveIdx > 0) ? blActiveIdx : 0;
  if (blCount > 0) blEnsureWindow();
  blPhase = BL_SELECT;
  blDirty = true;
}

static void blLeaveSelect() {
  fmSdUnmount();                  // освобождаем GPIO23 для динамика
  blSdOk = false;
  blPhase = BL_MENU;
  blSel = 0;
  blDirty = true;
}

void initBootLogoMenu() {
  loadCustomBootLogo();
  blPhase = BL_MENU;
  blSel = 0;
  blDirty = true;
  blMsg = "";
  blMsgUntil = 0;
  blMsgShown = false;
}

static void blHeader(const char* title, const char* right = NULL) {
  setDrawColor();
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(title);
  if (right) {
    display.setCursor(126 - 6 * (int)strlen(right), 2);
    display.print(right);
  }
  setDrawColor();
}

static void blDrawMenu() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);
  blHeader("BOOT LOGO");

  display.setCursor(2, 14);
  display.print("NOW: ");
  display.print(blCurrentName());

  for (int i = 0; i < 2; i++) {
    display.setCursor(2, 28 + i * 10);
    display.print(i == blSel ? "> " : "  ");
    display.print(blItems[i]);
  }

  display.setCursor(2, 56);
  if (millis() < blMsgUntil) {
    display.print(blMsg);
    blMsgShown = true;
  } else {
    display.print("FILES: SD /pictures");
  }

  display.display();
}

static void blDrawSelect() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);

  char cnt[14] = "";
  if (blSdOk && blCount > 0) snprintf(cnt, sizeof(cnt), "%d/%d", blSel + 1, blCount);
  blHeader("SD PICTURES", cnt[0] ? cnt : NULL);

  if (!blSdOk) {
    display.setCursor(2, 24);
    display.print("NO SD CARD");
    display.setCursor(2, 36);
    display.print("BACK: EXIT");
  } else if (blCount == 0) {
    display.setCursor(2, 20);
    display.print("NO PICTURES IN");
    display.setCursor(2, 30);
    display.print(BOOT_LOGO_DIR);
    display.setCursor(2, 44);
    display.print("DRAW SOME IN PAINT");
  } else {
    for (int row = 0; row < BL_ROWS; row++) {
      int idx = blTop + row;
      int ci = idx - blFirst;
      if (idx >= blCount || ci < 0 || ci >= blCached) break;

      const char* full = blNames[ci];
      size_t l = strlen(full) - 4;                 // без ".bin"
      if (l > 16) l = 16;
      char nm[20];
      memcpy(nm, full, l);
      nm[l] = 0;

      bool active = customBootLogoLoaded && strcmp(full, bootLogoName) == 0;
      char line[26];
      snprintf(line, sizeof(line), "%s%s%s", idx == blSel ? "> " : "  ", nm, active ? " *" : "");
      display.setCursor(2, 14 + row * 9);
      display.print(line);
    }
  }

  display.setCursor(2, 56);
  if (millis() < blMsgUntil) {
    display.print(blMsg);
    blMsgShown = true;
  } else if (blSdOk && blCount > 0) {
    display.print("SEL:SET  RIGHT:VIEW");
  }

  display.display();
}

static void blDrawPreview() {
  uint8_t tmp[BOOT_LOGO_BYTES];
  bool ok = blReadSd(blNames[blSel - blFirst], tmp);

  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  if (ok) {
    display.drawBitmap(0, 0, tmp, 128, 64, whiteTheme ? BLACK : WHITE);
  } else {
    display.setTextSize(1);
    display.setCursor(2, 28);
    display.print("READ ERROR");
  }
  display.display();
}

static void blDrawConfirm() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);
  blHeader("BOOT LOGO");
  display.setCursor(10, 22);
  display.print("USE DEFAULT LOGO?");
  display.setCursor(10, 40);
  display.print("SELECT: YES");
  display.setCursor(10, 50);
  display.print("BACK:   NO");
  display.display();
}

void runBootLogoSettings() {
  bool canPress = (millis() - lastButtonPress > DEBOUNCE_DELAY);

  // убрать сообщение, когда оно истекло
  if (blMsgShown && millis() >= blMsgUntil) {
    blMsgShown = false;
    blDirty = true;
  }

  if (blPhase == BL_SELECT) {
    if (canPress) {
      if (blSdOk && blCount > 0 && digitalRead(BTN_UP) == LOW) {
        blSel = (blSel - 1 + blCount) % blCount;
        blEnsureWindow();
        blDirty = true;
        lastButtonPress = millis();
      } else if (blSdOk && blCount > 0 && digitalRead(BTN_DOWN) == LOW) {
        blSel = (blSel + 1) % blCount;
        blEnsureWindow();
        blDirty = true;
        lastButtonPress = millis();
      } else if (blSdOk && blCount > 0 && digitalRead(BTN_RIGHT) == LOW) {
        // предпросмотр выбранной картинки
        blPhase = BL_PREVIEW;
        blDirty = true;
        lastButtonPress = millis();
        return;
      } else if (blSdOk && blCount > 0 && digitalRead(BTN_SELECT) == LOW) {
        lastButtonPress = millis();
        if (blApplyFromSd(blNames[blSel - blFirst])) {
          blLeaveSelect();
          blSetMsg("LOGO SET", 2000);
        } else {
          blSetMsg("COPY FAILED", 2000);
        }
        return;
      } else if (digitalRead(BTN_BACK) == LOW) {
        lastButtonPress = millis();
        blLeaveSelect();
        return;
      }
    }

    if (blDirty) {
      blDirty = false;
      blDrawSelect();
    }
    return;
  }

  if (blPhase == BL_PREVIEW) {
    if (canPress) {
      if (digitalRead(BTN_SELECT) == LOW) {
        // выбрать именно эту картинку
        lastButtonPress = millis();
        if (blApplyFromSd(blNames[blSel - blFirst])) {
          blLeaveSelect();
          blSetMsg("LOGO SET", 2000);
        } else {
          blPhase = BL_SELECT;
          blSetMsg("COPY FAILED", 2000);
        }
        return;
      }
      if (digitalRead(BTN_BACK) == LOW || digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
          digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
        blPhase = BL_SELECT;
        blDirty = true;
        lastButtonPress = millis();
        return;
      }
    }

    if (blDirty) {
      blDirty = false;
      blDrawPreview();
    }
    return;
  }

  if (blPhase == BL_CONFIRM) {
    if (canPress) {
      if (digitalRead(BTN_SELECT) == LOW) {
        resetCustomBootLogo();
        blPhase = BL_MENU;
        blSetMsg("DEFAULT ACTIVE", 2000);
        lastButtonPress = millis();
        return;
      }

      if (digitalRead(BTN_BACK) == LOW) {
        blPhase = BL_MENU;
        blDirty = true;
        lastButtonPress = millis();
        return;
      }
    }

    if (blDirty) {
      blDirty = false;
      blDrawConfirm();
    }
    return;
  }

  // ----- BL_MENU -----
  if (canPress) {
    if (digitalRead(BTN_UP) == LOW) {
      blSel = (blSel + 1) % 2;
      blDirty = true;
      lastButtonPress = millis();
    } else if (digitalRead(BTN_DOWN) == LOW) {
      blSel = (blSel + 1) % 2;
      blDirty = true;
      lastButtonPress = millis();
    } else if (digitalRead(BTN_SELECT) == LOW) {
      lastButtonPress = millis();
      blDirty = true;

      if (blSel == 0) {
        blEnterSelect();
      } else {
        blPhase = BL_CONFIRM;
      }
      return;
    } else if (digitalRead(BTN_BACK) == LOW) {
      lastButtonPress = millis();
      changeState(SETTINGS_MENU);
      return;
    }
  }

  if (blDirty) {
    blDirty = false;
    blLastDraw = millis();
    blDrawMenu();
  }
}

#endif
