#ifndef _filemanager_h
#define _filemanager_h

// ============================================================
// File Manager (проводник: файлы LittleFS + SD-карта + разделы флеш-памяти)
// - Просмотр папок и файлов, путь всегда виден на экране:
//     * в шапке — текущая папка (или путь к открытому файлу);
//     * внизу списка — полный путь выбранного пункта
//       (длинные пути прокручиваются бегущей строкой).
// - Открытие файлов: текст / HEX / картинка 128x64 (.bin 1024 байта,
//   т.е. boot-логотипы).
// - Удаление файлов и папок (с подтверждением).
// - Информация: путь, тип, размер, занято/всего на накопителе.
// - Возле каждого пункта списка рисуется маленькая иконка папки (8x7).
// - Видео с SD-карты (и из LittleFS): SELECT на файле .vid / .snv (или .bin / .raw
//   с размером, кратным 1024 байтам и не менее 2 кадров) запускает воспроизведение.
//   Формат: кадры 128x64, 1 бит на пиксель, по 1024 байта подряд (как boot-битмап).
//   Необязательный заголовок 8 байт: 'S','N','L','V', fps, 0, 0, 0.
//   Без заголовка fps = 15. Видео читается потоком с карты — длина ограничена только её размером.
//   Конвертер ролика в этот формат: tools/video2snv.py
//   Кнопки в видео: SELECT — пауза/пуск, LEFT/RIGHT — перемотка -/+5 с,
//   UP — повтор вкл/выкл, DOWN — показать полосу прогресса, BACK — закрыть.
//
// SD-карта (папка /sd): SPI, CS=GPIO27, SCK=GPIO18, MOSI=GPIO23, MISO=GPIO19.
//   Все действия как с /littlefs: открыть (TXT/HEX/BMP), INFO, удаление, UPLOAD по Wi-Fi.
//   Карта монтируется при входе в /sd и отключается при выходе из проводника
//   (GPIO23 общий с динамиком — в играх он должен быть свободен от SPI).
//
// Управление:
//   UP / DOWN  — выбор;          SELECT — открыть файл / войти в папку
//   RIGHT      — меню действий (OPEN / INFO / DELETE)
//   LEFT/BACK  — на уровень вверх (BACK в корне — выход в APPS)
//   Строка "+ UPLOAD" (в /littlefs) — загрузка файлов с телефона/ПК по Wi-Fi
// В просмотре: UP/DOWN — строка, LEFT/RIGHT — страница,
//   SELECT — сменить режим (TXT/HEX/BMP), BACK — закрыть.
// ============================================================

#include <LittleFS.h>
#include <SPI.h>
#include <SD.h>
#include <esp_partition.h>
#include <WiFi.h>
#include <WebServer.h>
#include <string.h>
#include <strings.h>

void changeState(int state);

#define FM_MAX_ENTRIES 64     // максимум пунктов в одной папке
#define FM_NAME_LEN    64     // максимум БАЙТ в имени (с нулём); кириллица = 2 байта на букву
#define FM_PATH_LEN    128    // максимум длины полного пути
#define FM_COLS        20     // символов в строке экрана
#define FM_ROWS        5      // строк в списке / просмотре
#define FM_BMP_BYTES   1024   // 128x64 / 8

// Иконка пункта меню (26x26)
const unsigned char iconFiles[] PROGMEM = { 0x1f, 0xff, 0xfe, 0x00,
  0x20, 0x00, 0x01, 0x00,
  0x40, 0x00, 0x00, 0x80,
  0x80, 0x00, 0x00, 0x40,
  0xbf, 0xe0, 0x00, 0x40,
  0xa0, 0x10, 0x00, 0x40,
  0xa0, 0x0f, 0xff, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xbf, 0xff, 0xff, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa7, 0xff, 0xe1, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa7, 0xfc, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa7, 0xff, 0xf1, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xa0, 0x00, 0x01, 0x40,
  0xbf, 0xff, 0xff, 0x40,
  0x80, 0x00, 0x00, 0x40,
  0x40, 0x00, 0x00, 0x80,
  0x20, 0x00, 0x01, 0x00,
  0x1f, 0xff, 0xfe, 0x00 };

// Иконка файла 26x26 — bitmap из предоставленного изображения.
// Используется в меню действий для выбранного файла.
const unsigned char iconFile26[] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x3f, 0xe0, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00, 0x20, 0x0f, 0xff, 0x00, 0x20, 0x00,
0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x20, 0x00, 0x01, 0x00, 0x20,
0x00, 0x01, 0x00, 0x27, 0xff, 0xe1, 0x00, 0x20, 0x00, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00,
0x27, 0xfc, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x27, 0xff, 0xe1,
0x00, 0x20, 0x00, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Маленькая иконка папки 8x7 — рисуется слева от каждого пункта списка.
const unsigned char iconFolderSmall[] PROGMEM = {
   0xf0, 0x8f, 0xe1, 0x9f, 0x81, 0x81, 0xff
};
#define FM_ICON_W 8
#define FM_ICON_H 7

// ---------- состояния экрана ----------

#define FM_LIST    0
#define FM_MENU    1
#define FM_CONFIRM 2
#define FM_VIEW    3
#define FM_INFO    4
#define FM_NOFS    5
#define FM_SERVER  6
#define FM_VIDEO   7

// ---------- режимы просмотра ----------
#define FM_V_TEXT 0
#define FM_V_HEX  1
#define FM_V_BMP  2

struct FmEntry {
  char     name[FM_NAME_LEN];
  bool     isDir;
  uint32_t size;
};

static FmEntry fmEntries[FM_MAX_ENTRIES];
static FmEntry fmUploadRow = { "+ UPLOAD", false, 0 };   // служебная строка: загрузка файлов по Wi-Fi
static int     fmCount = 0;
static int     fmSkipped = 0;          // пункты, которые не поместились (лимит / длинное имя)
#define FM_ROOT "/sd"                 // корень проводника — сразу SD-карта (LittleFS и flash скрыты)
static char    fmDir[FM_PATH_LEN] = FM_ROOT;

static int     fmPhase = FM_LIST;
static int     fmSel = 0, fmTop = 0;
static int     fmMenuSel = 0;
static bool    fmDirty = true;
static unsigned long fmLastDraw = 0, fmMarqStart = 0, fmMsgUntil = 0, fmBannerUntil = 0;
static bool    fmMarqActive = false, fmMsgShown = false, fmBannerShown = false;
static char    fmMsg[21] = "";

// цель действия (меню / удаление / инфо)
static char     fmTarget[FM_PATH_LEN];
static bool     fmTargetIsDir = false;
static uint32_t fmTargetSize = 0;

// просмотр файла
static char     fmViewPath[FM_PATH_LEN];
static uint32_t fmViewSize = 0;
static int      fmViewMode = FM_V_TEXT;
static int      fmViewModes = 0;       // битовая маска доступных режимов
static int      fmViewPos = 0;         // текст: номер строки, HEX: номер строки по 4 байта
static bool     fmViewMore = false;
static uint8_t* fmBmp = NULL;

// ============================================================
// Видео (состояние плеера)
// ============================================================
#define FM_VID_FRAME     1024     // 128x64 / 8
#define FM_VID_DEF_FPS   15
#define FM_VID_SEEK_SEC  5

static File     fmVid;
static uint8_t* fmVidBuf = NULL;        // текущий кадр (1024 байта)
static uint32_t fmVidHdr = 0;           // размер заголовка в файле
static uint32_t fmVidFrames = 0;        // всего кадров
static uint32_t fmVidCur = 0;           // номер показанного кадра
static uint32_t fmVidBase = 0;          // кадр, с которого отсчитывается время
static unsigned long fmVidT0 = 0;       // millis() в момент кадра fmVidBase
static int      fmVidFps = FM_VID_DEF_FPS;
static bool     fmVidPaused = false, fmVidEnded = false, fmVidLoop = false;
static bool     fmVidDirty = true, fmVidOverlayShown = false;
static unsigned long fmVidOverlayUntil = 0;
static const char* fmVidNote = "";      // короткая подпись в плашке (LOOP ON и т.п.)

static void fmCloseVideo() {
  if (fmVid) fmVid.close();
  if (fmVidBuf) { free(fmVidBuf); fmVidBuf = NULL; }
  fmVidFrames = 0;
}

// ============================================================
// SD-карта (SPI)
// ============================================================
#define FM_SD_CS    16
#define FM_SD_SCK   26
#define FM_SD_MISO  36
#define FM_SD_MOSI  25

#ifdef VSPI
  #define FM_SD_BUS VSPI
#else
  #define FM_SD_BUS FSPI
#endif

static SPIClass fmSpi(FM_SD_BUS);
bool fmSdMounted = false;
bool fmSdErr = false;            // карта пропала / не читается в открытой папке

static bool fmSdMount() {
  if (fmSdMounted) return true;
  const uint32_t freqs[2] = { 10000000UL, 4000000UL };   // на длинных проводах пробуем помедленнее
  for (int i = 0; i < 2; i++) {
    fmSpi.begin(FM_SD_SCK, FM_SD_MISO, FM_SD_MOSI, FM_SD_CS);
    if (SD.begin(FM_SD_CS, fmSpi, freqs[i]) && SD.cardType() != CARD_NONE) {
      fmSdMounted = true;
      fmSdErr = false;
      return true;
    }
    SD.end();
    fmSpi.end();
  }
  return false;
}

static void fmSdUnmount() {
  fmCloseVideo();                               // файл видео должен быть закрыт до SD.end()
  if (!fmSdMounted) return;
  SD.end();
  fmSpi.end();                                  // отпускаем GPIO23 для tone()
  pinMode(FM_SD_CS, INPUT);
  fmSdMounted = false;
}

// ============================================================
// Вспомогательные функции
// ============================================================
static inline bool fmBtn(int pin) { return digitalRead(pin) == LOW; }

static void fmSetMsg(const char* m) {
  strncpy(fmMsg, m, sizeof(fmMsg) - 1);
  fmMsg[sizeof(fmMsg) - 1] = 0;
  fmMsgUntil = millis() + 2000;
  fmDirty = true;
}

static bool fmJoin(char* out, size_t n, const char* dir, const char* name) {
  size_t dl = strlen(dir), nl = strlen(name);
  bool root = (dl == 1 && dir[0] == '/');
  size_t need = (root ? 0 : dl) + 1 + nl + 1;
  if (need > n) return false;
  if (root) snprintf(out, n, "/%s", name);
  else      snprintf(out, n, "%s/%s", dir, name);
  return true;
}

static void fmParentOf(char* path) {       // обрезает путь на месте
  char* s = strrchr(path, '/');
  if (!s || s == path) { path[0] = '/'; path[1] = 0; }
  else *s = 0;
}

static const char* fmBaseName(const char* p) {
  const char* s = strrchr(p, '/');
  return s ? s + 1 : p;
}

static bool fmEndsWithNoCase(const char* s, const char* suffix) {
  size_t sl = strlen(s), xl = strlen(suffix);
  return sl >= xl && strcasecmp(s + sl - xl, suffix) == 0;
}

static void fmFmtSize(char* out, size_t n, uint32_t b) {
  if (b < 1024UL)            snprintf(out, n, "%luB", (unsigned long)b);
  else if (b < 10240UL)      snprintf(out, n, "%lu.%luK", (unsigned long)(b / 1024), (unsigned long)((b % 1024) * 10 / 1024));
  else if (b < 1048576UL)    snprintf(out, n, "%luK", (unsigned long)(b / 1024));
  else                       snprintf(out, n, "%lu.%luM", (unsigned long)(b >> 20), (unsigned long)(((b & 0xFFFFF) * 10) >> 20));
}

static void fmFmtBig(char* out, size_t n, uint64_t b) {   // для объёма карты (может быть > 4 ГБ)
  if (b >= (1ULL << 30)) snprintf(out, n, "%lu.%luG", (unsigned long)(b >> 30), (unsigned long)(((b & 0x3FFFFFFFULL) * 10) >> 30));
  else fmFmtSize(out, n, (uint32_t)b);
}

// ============================================================
// Шрифт 5x7 для просмотра текстовых файлов: весь ASCII, UTF-8, кириллица,
// ₽ ÷ × ± ° « » … € £ ¥ • · стрелки. Латиница с диакритикой рисуется
// базовой буквой, типографские кавычки/тире — обычными. Остальное — '?'.
// Бит 0 байта-столбца = верхняя строка глифа.
// ============================================================
// Таблицы кириллицы FM_CYR_UP / FM_CYR_LO / FM_CYR_YO_* теперь лежат в lang.h
// (их же использует LangDisplay для русского интерфейса).

struct FmSym { uint16_t cp; uint8_t c[5]; };
static const FmSym FM_SYMS[] PROGMEM = {
  {0x20BD, {0x10,0x7F,0x19,0x19,0x06}},  // ₽
  {0x00F7, {0x08,0x08,0x2A,0x08,0x08}},  // ÷
  {0x00D7, {0x22,0x14,0x08,0x14,0x22}},  // ×
  {0x00B1, {0x44,0x44,0x5F,0x44,0x44}},  // ±
  {0x00B0, {0x00,0x07,0x05,0x07,0x00}},  // °
  {0x00AB, {0x08,0x14,0x2A,0x14,0x22}},  // «
  {0x00BB, {0x22,0x14,0x2A,0x14,0x08}},  // »
  {0x2026, {0x40,0x00,0x40,0x00,0x40}},  // …
  {0x20AC, {0x14,0x3E,0x55,0x55,0x41}},  // €
  {0x00A3, {0x48,0x7E,0x49,0x41,0x42}},  // £
  {0x00A5, {0x01,0x2A,0x3C,0x2A,0x01}},  // ¥
  {0x2022, {0x00,0x1C,0x1C,0x1C,0x00}},  // •
  {0x00B7, {0x00,0x00,0x08,0x00,0x00}},  // ·
  {0x2190, {0x08,0x1C,0x2A,0x08,0x08}},  // ←
  {0x2191, {0x04,0x02,0x7F,0x02,0x04}},  // ↑
  {0x2192, {0x08,0x08,0x2A,0x1C,0x08}},  // →
  {0x2193, {0x10,0x20,0x7F,0x20,0x10}},  // ↓
};
#define FM_SYMS_N (sizeof(FM_SYMS) / sizeof(FM_SYMS[0]))

static int fmSymIndex(uint32_t cp) {
  for (size_t i = 0; i < FM_SYMS_N; i++) if (pgm_read_word(&FM_SYMS[i].cp) == cp) return (int)i;
  return -1;
}

// Unicode -> код ячейки (то, что умеет рисовать fmDrawCell)
static uint16_t fmCellFor(uint32_t cp) {
  if (cp >= 32 && cp < 127) return (uint16_t)cp;
  if (fmSymIndex(cp) >= 0) return (uint16_t)cp;
  if (cp >= 0x410 && cp <= 0x44F) return (uint16_t)cp;                 // А-Яа-я
  if (cp == 0x401 || cp == 0x451) return (uint16_t)cp;                 // Ёё
  switch (cp) {
    case 0x00A0: case 0x2002: case 0x2003: case 0x2009: case 0x202F: return ' ';
    case 0x2018: case 0x2019: case 0x201A: case 0x2032: return '\'';
    case 0x201C: case 0x201D: case 0x201E: case 0x2033: return '"';
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015: case 0x2212: return '-';
    case 0x2039: return '<';   case 0x203A: return '>';
    case 0x2116: return '#';   case 0x2248: return '~';
    case 0x00A1: return '!';   case 0x00BF: return '?';
    case 0x00A2: return 'c';   case 0x00A9: return 'C';   case 0x00AE: return 'R';   case 0x2122: return 'T';
    case 0x00A7: return 'S';   case 0x00B6: return 'P';
    case 0x00B2: return '2';   case 0x00B3: return '3';   case 0x00B9: return '1';   case 0x00B5: return 'u';
    case 0x00BC: case 0x00BD: case 0x00BE: return '/';
    case 0x0406: case 0x0407: return 'I';   case 0x0456: case 0x0457: return 'i';   // І Ї і ї
    case 0x0404: return 'E';   case 0x0454: return 'e';                                // Є є
    case 0x0490: return 0x413; case 0x0491: return 0x433;                              // Ґ ґ
  }
  if (cp >= 0xC0 && cp <= 0xFF) {            // латиница с диакритикой -> базовая буква
    static const char lat[] =
      "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTB"     // C0-DF
      "aaaaaaaceeeeiiiidnooooo/ouuuuyty";    // E0-FF
    return (uint16_t)lat[cp - 0xC0];
  }
  return '?';
}

// Рисует одну ячейку 5x7 в точке (x,y); цвет — как у текущей темы.
static void fmDrawCell(int x, int y, uint16_t cell) {
  uint16_t col = whiteTheme ? BLACK : WHITE;
  if (cell >= 32 && cell < 127) { display.drawChar(x, y, (char)cell, col, col, 1); return; }
  uint8_t g[5];
  int si = fmSymIndex(cell);
  if (si >= 0)                              for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_SYMS[si].c[i]);
  else if (cell == 0x401)                   for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_YO_UP[i]);
  else if (cell == 0x451)                   for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_YO_LO[i]);
  else if (cell >= 0x410 && cell <= 0x42F)  for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_UP[cell - 0x410][i]);
  else if (cell >= 0x430 && cell <= 0x44F)  for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_LO[cell - 0x430][i]);
  else { display.drawChar(x, y, '?', col, col, 1); return; }
  for (int cx = 0; cx < 5; cx++)
    for (int ry = 0; ry < 8; ry++)
      if (g[cx] & (1 << ry)) display.drawPixel(x + cx, y + ry, col);
}

// Печать строки ячеек (завершается нулём), шаг 6 пикселей.
static void fmPrintCells(int x, int y, const uint16_t* s) {
  for (int i = 0; s[i] && i < FM_COLS; i++) fmDrawCell(x + i * 6, y, s[i]);
}

// ---------- UTF-8: имена файлов считаем в СИМВОЛАХ, а не в байтах ----------
static bool fmIsAscii(const char* s) {
  for (; *s; s++) if ((uint8_t)*s >= 0x80) return false;
  return true;
}
static size_t fmU8Len(const char* s) {            // число символов
  size_t n = 0;
  for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++;
  return n;
}
static const char* fmU8Skip(const char* s, size_t chars) {   // указатель на символ номер chars
  while (*s && chars > 0) {
    s++;
    while (((uint8_t)*s & 0xC0) == 0x80) s++;
    chars--;
  }
  return s;
}
static uint32_t fmU8Next(const char*& p) {        // декодирует один символ, сдвигает p
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80) return c;
  int need = ((c & 0xE0) == 0xC0) ? 1 : ((c & 0xF0) == 0xE0) ? 2 : ((c & 0xF8) == 0xF0) ? 3 : 0;
  uint32_t cp = need == 1 ? (c & 0x1F) : need == 2 ? (c & 0x0F) : (c & 0x07);
  if (need == 0) return '?';
  while (need > 0 && ((uint8_t)*p & 0xC0) == 0x80) { cp = (cp << 6) | ((uint8_t)*p++ & 0x3F); need--; }
  return need ? '?' : cp;
}
// копирует не более maxChars символов (не режет UTF-8 посередине)
static void fmU8Copy(char* out, const char* s, size_t maxChars) {
  const char* e = fmU8Skip(s, maxChars);
  size_t n = (size_t)(e - s);
  memcpy(out, s, n);
  out[n] = 0;
}

// печатает не более FM_COLS символов строки s, начиная с символа off.
// ASCII идёт через display.print (там работает перевод интерфейса), строки с
// не-ASCII (русские имена файлов) рисуются побуквенно — на любом языке интерфейса.
static void fmPrintAt(int x, int y, const char* s, size_t off = 0) {
  if (fmIsAscii(s)) {
    char tmp[FM_COLS + 1];
    size_t len = strlen(s);
    if (off > len) off = len;
    strncpy(tmp, s + off, FM_COLS);
    tmp[FM_COLS] = 0;
    display.setCursor(x, y);
    display.print(tmp);
    return;
  }
  const char* p = fmU8Skip(s, off);
  for (int i = 0; *p && i < FM_COLS; i++) fmDrawCell(x + i * 6, y, fmCellFor(fmU8Next(p)));
}

// конец строки с префиксом ".." если не помещается (буфер out — FM_COLS*4+1 байт)
static void fmTailText(char* out, const char* s) {
  size_t len = fmU8Len(s);
  if (len <= FM_COLS) { strcpy(out, s); return; }
  out[0] = '.'; out[1] = '.';
  strcpy(out + 2, fmU8Skip(s, len - (FM_COLS - 2)));
}

// бегущая строка для длинных путей
static void fmMarquee(int x, int y, const char* s) {
  size_t len = fmU8Len(s);
  if (len <= FM_COLS) { fmPrintAt(x, y, s); return; }
  size_t steps = len - FM_COLS;
  size_t t = (size_t)(((millis() - fmMarqStart) / 300) % (steps + 8));
  size_t off = (t < 4) ? 0 : ((t - 4 > steps) ? steps : (t - 4));
  fmPrintAt(x, y, s, off);
}

static void fmBegin() {
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  setDrawColor();
  display.setTextSize(1);
}

static void fmHeader(const char* text) {
  char tmp[FM_COLS * 4 + 1];
  fmTailText(tmp, text);
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setCursor(2, 2);
  display.print(tmp);
  setDrawColor();
}

// ============================================================
// Работа с каталогом
// ============================================================
static inline bool fmAtRoot() { return strcmp(fmDir, FM_ROOT) == 0; }
static inline bool fmHasUp()  { return !fmAtRoot(); }
static bool fmIsFs(const char* p);
static inline bool fmIsSd(const char* p);
static inline bool fmCanUpload() { return fmIsFs(fmDir); }           // загрузка — только в /littlefs

// буфер обмена: COPY / MOVE запоминают файл, "+ PASTE" в нужной папке вставляет
static bool fmClipActive = false, fmClipMove = false, fmClipIsDir = false;
static char fmClipPath[FM_PATH_LEN] = "";
static FmEntry fmPasteRow = { "+ PASTE", false, 0 };
static inline bool fmIsPasteRow(const FmEntry* e) { return e == &fmPasteRow; }
static inline bool fmCanPaste() { return fmClipActive && fmIsSd(fmDir); }

static int fmItemCount() { return fmCount + (fmHasUp() ? 1 : 0) + (fmCanPaste() ? 1 : 0) + (fmCanUpload() ? 1 : 0); }

static FmEntry* fmItem(int i) {
  if (fmHasUp()) { if (i == 0) return NULL; i--; }
  if (fmCanPaste()) { if (i == 0) return &fmPasteRow; i--; }
  if (fmCanUpload()) { if (i == 0) return &fmUploadRow; i--; }
  return (i >= 0 && i < fmCount) ? &fmEntries[i] : NULL;
}
static inline bool fmIsUploadRow(const FmEntry* e) { return e == &fmUploadRow; }

// полный путь пункта списка ("..": путь родительской папки)
static bool fmItemPath(char* out, size_t n, int i) {
  FmEntry* e = fmItem(i);
  if (!e) {
    strncpy(out, fmDir, n); out[n - 1] = 0;
    fmParentOf(out);
    return true;
  }
  if (fmIsUploadRow(e) || fmIsPasteRow(e)) { strncpy(out, fmDir, n); out[n - 1] = 0; return true; }   // папка, куда загрузим / вставим
  return fmJoin(out, n, fmDir, e->name);
}

static bool fmLess(const FmEntry& a, const FmEntry& b) {
  if (a.isDir != b.isDir) return a.isDir;     // папки сверху
  return strcasecmp(a.name, b.name) < 0;
}

static void fmSortEntries() {
  for (int i = 1; i < fmCount; i++) {
    FmEntry key = fmEntries[i];
    int j = i - 1;
    while (j >= 0 && fmLess(key, fmEntries[j])) { fmEntries[j + 1] = fmEntries[j]; j--; }
    fmEntries[j + 1] = key;
  }
}

// ============================================================
// Виртуальная файловая система проводника
//   /            — корень: три папки
//   /littlefs/…  — обычные файлы во внутренней памяти (LittleFS), можно удалять
//   /sd/…        — файлы на SD-карте, можно удалять
//   /flash/…     — разделы флеш-памяти (прошивка, NVS, ...), только чтение (HEX)
// ============================================================
#define FM_FS_PREFIX    "/littlefs"
#define FM_SD_PREFIX    "/sd"
#define FM_FLASH_PREFIX "/flash"

static bool fmUnder(const char* p, const char* prefix) {
  size_t n = strlen(prefix);
  return strncmp(p, prefix, n) == 0 && (p[n] == 0 || p[n] == '/');
}
static inline bool fmIsFlash(const char* p) { return fmUnder(p, FM_FLASH_PREFIX); }
static inline bool fmIsLfs(const char* p)   { return fmUnder(p, FM_FS_PREFIX); }
static inline bool fmIsSd(const char* p)    { return fmUnder(p, FM_SD_PREFIX); }
static bool fmIsFs(const char* p)           { return fmIsLfs(p) || fmIsSd(p); }   // любое файловое хранилище
// удалять можно только содержимое /littlefs и /sd, но не сами эти папки
static inline bool fmCanDelete(const char* p) {
  return strncmp(p, FM_FS_PREFIX "/", strlen(FM_FS_PREFIX) + 1) == 0 ||
         strncmp(p, FM_SD_PREFIX "/", strlen(FM_SD_PREFIX) + 1) == 0;
}

// какая файловая система стоит за виртуальным путём
static inline fs::FS& fmFsOf(const char* v) {
  if (fmIsSd(v)) return SD;
  return LittleFS;
}

// виртуальный путь -> реальный путь внутри своей ФС (LittleFS или SD)
static bool fmFsReal(const char* v, char* out, size_t n) {
  const char* r;
  if (fmIsLfs(v))     r = v + strlen(FM_FS_PREFIX);
  else if (fmIsSd(v)) r = v + strlen(FM_SD_PREFIX);
  else return false;
  if (*r == 0) r = "/";
  if (strlen(r) + 1 > n) return false;
  strcpy(out, r);
  return true;
}

static File fmFsOpen(const char* v, const char* mode = "r") {
  char r[FM_PATH_LEN];
  if (!fmFsReal(v, r, sizeof(r))) return File();
  if (fmIsSd(v) && !fmSdMounted) return File();
  return fmFsOf(v).open(r, mode);
}
static bool fmFsRemove(const char* v) {
  char r[FM_PATH_LEN];
  if (!fmFsReal(v, r, sizeof(r))) return false;
  if (fmIsSd(v) && !fmSdMounted) return false;
  return fmFsOf(v).remove(r);
}
static bool fmFsRmdir(const char* v) {
  char r[FM_PATH_LEN];
  if (!fmFsReal(v, r, sizeof(r))) return false;
  if (fmIsSd(v) && !fmSdMounted) return false;
  return fmFsOf(v).rmdir(r);
}

static const esp_partition_t* fmFindPart(const char* label) {
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it) {
    const esp_partition_t* p = esp_partition_get(it);
    if (p && strcmp(p->label, label) == 0) { esp_partition_iterator_release(it); return p; }
    it = esp_partition_next(it);
  }
  return NULL;
}

// чтение из файла LittleFS или из раздела флеш-памяти
static size_t fmReadAt(const char* v, uint32_t off, uint8_t* buf, size_t n) {
  if (fmIsFlash(v)) {
    const esp_partition_t* p = fmFindPart(fmBaseName(v));
    if (!p || off >= p->size) return 0;
    if (off + n > p->size) n = p->size - off;
    return (esp_partition_read(p, off, buf, n) == ESP_OK) ? n : 0;
  }
  File f = fmFsOpen(v, "r");
  if (!f) return 0;
  size_t r = 0;
  if (f.seek(off)) r = f.read(buf, n);
  f.close();
  return r;
}

static void fmAddEntry(const char* name, bool isDir, uint32_t size) {
  size_t bl = strlen(name);
  if (fmCount >= FM_MAX_ENTRIES || bl >= FM_NAME_LEN) { fmSkipped++; return; }
  FmEntry& e = fmEntries[fmCount++];
  strcpy(e.name, name);
  e.isDir = isDir;
  e.size = size;
}

static void fmLoadDir() {
  fmCount = 0;
  fmSkipped = 0;

  if (!fmIsSd(fmDir)) return;                         // проводник показывает только SD-карту

  File dir = fmFsOpen(fmDir);
  if (!dir) {
    if (fmIsSd(fmDir)) { fmSdUnmount(); fmSdErr = true; }   // карту вынули — при следующем входе смонтируем заново
    return;
  }
  if (!dir.isDirectory()) { dir.close(); return; }
  if (fmIsSd(fmDir)) fmSdErr = false;

  File f = dir.openNextFile();
  while (f) {
    const char* base = fmBaseName(f.name());
    if (strlen(base) > 0) {
      bool d = f.isDirectory();
      fmAddEntry(base, d, d ? 0 : (uint32_t)f.size());
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  fmSortEntries();
}

static void fmClampSel() {
  int n = fmItemCount();
  if (n <= 0) { fmSel = 0; fmTop = 0; return; }
  if (fmSel >= n) fmSel = n - 1;
  if (fmSel < 0) fmSel = 0;
  if (fmSel < fmTop) fmTop = fmSel;
  if (fmSel >= fmTop + FM_ROWS) fmTop = fmSel - FM_ROWS + 1;
  if (fmTop < 0) fmTop = 0;
}

static void fmSelectByName(const char* name) {
  int n = fmItemCount();
  for (int i = 0; i < n; i++) {
    FmEntry* e = fmItem(i);
    if (e && strcmp(e->name, name) == 0) { fmSel = i; break; }
  }
  fmTop = 0;
  fmClampSel();
}

static void fmEnterDir(const char* path) {
  if (fmIsSd(path) && !fmSdMount()) { fmSetMsg("NO SD CARD"); return; }   // карты нет — остаёмся где были
  strncpy(fmDir, path, FM_PATH_LEN - 1);
  fmDir[FM_PATH_LEN - 1] = 0;
  fmLoadDir();
  fmSel = 0; fmTop = 0;
  fmClampSel();
  fmMarqStart = millis();
  fmDirty = true;
}

static void fmGoUp() {
  if (!fmHasUp()) return;
  char child[FM_NAME_LEN];
  strncpy(child, fmBaseName(fmDir), FM_NAME_LEN - 1);
  child[FM_NAME_LEN - 1] = 0;
  char parent[FM_PATH_LEN];
  strcpy(parent, fmDir);
  fmParentOf(parent);
  fmEnterDir(parent);
  fmSelectByName(child);          // курсор — на папку, из которой вышли
}

// ============================================================
// Удаление (рекурсивное для папок)
// ============================================================
static bool fmRemoveTree(const char* path, int depth) {
  File f = fmFsOpen(path);
  if (!f) return false;
  bool isDir = f.isDirectory();
  f.close();
  if (!isDir) return fmFsRemove(path);
  if (depth > 8) return false;

  for (int guard = 0; guard < 2000; guard++) {
    File d = fmFsOpen(path);
    if (!d) return false;
    File c = d.openNextFile();
    if (!c) { d.close(); break; }
    char child[FM_PATH_LEN];
    bool ok = fmJoin(child, sizeof(child), path, fmBaseName(c.name()));
    c.close();
    d.close();
    if (!ok) return false;
    if (!fmRemoveTree(child, depth + 1)) return false;
  }
  return fmFsRmdir(path);
}

// boot-логотип хранится копией во flash (LittleFS): удаление файла на SD его не затрагивает,
// но если удалили саму копию — откатываемся на встроенный логотип
static void fmAfterDelete() {
  if (customBootLogoLoaded && blFsReady && !LittleFS.exists(BOOT_LOGO_CACHE)) resetCustomBootLogo();
}

static void fmDoDelete() {
  bool ok = fmCanDelete(fmTarget) && fmRemoveTree(fmTarget, 0);
  if (ok && fmClipActive && fmUnder(fmClipPath, fmTarget)) fmClipActive = false;
  fmAfterDelete();
  fmLoadDir();
  fmClampSel();
  fmSetMsg(ok ? "DELETED" : "DELETE FAILED");
  fmPhase = FM_LIST;
  fmDirty = true;
}

// ============================================================
// Копирование / перенос (только SD-карта)
// ============================================================
static bool fmCancelOp = false;
static unsigned long fmOpDrawAt = 0;
static const char* fmOpTitle = "COPYING...";

static bool fmExists(const char* v) {
  char r[FM_PATH_LEN];
  if (!fmFsReal(v, r, sizeof(r))) return false;
  if (fmIsSd(v) && !fmSdMounted) return false;
  return fmFsOf(v).exists(r);
}

// экран прогресса (total == 0 — размер неизвестен, показываем счётчик байт)
static void fmOpProgress(const char* name, uint32_t done, uint32_t total) {
  uint16_t col = whiteTheme ? BLACK : WHITE;
  fmBegin();
  fmHeader(fmOpTitle);
  char nm[FM_COLS * 4 + 1];
  fmTailText(nm, name);
  fmPrintAt(2, 16, nm);
  display.drawRect(2, 30, 124, 9, col);
  if (total > 0) {
    int w = (int)((uint64_t)done * 120 / total);
    if (w > 120) w = 120;
    display.fillRect(4, 32, w, 5, col);
  }
  char sz[12];
  fmFmtSize(sz, sizeof(sz), done);
  fmPrintAt(2, 43, sz);
  fmPrintAt(2, 55, "BACK: CANCEL");
  display.display();
}

static bool fmCopyFile(const char* src, const char* dst) {
  File in = fmFsOpen(src, "r");
  if (!in || in.isDirectory()) { if (in) in.close(); return false; }
  File out = fmFsOpen(dst, "w");
  if (!out) { in.close(); return false; }
  const size_t BUF = 1024;
  uint8_t* buf = (uint8_t*)malloc(BUF);
  if (!buf) { in.close(); out.close(); fmFsRemove(dst); return false; }
  uint32_t total = (uint32_t)in.size(), done = 0;
  bool ok = true;
  while (true) {
    size_t n = in.read(buf, BUF);
    if (n == 0) break;
    if (out.write(buf, n) != n) { ok = false; break; }          // карта заполнена / ошибка записи
    done += n;
    if (millis() - fmOpDrawAt > 250) {
      fmOpDrawAt = millis();
      fmOpProgress(fmBaseName(src), done, total);
      if (fmBtn(BTN_BACK)) { fmCancelOp = true; ok = false; break; }
    }
  }
  free(buf);
  in.close();
  out.close();
  if (!ok) fmFsRemove(dst);
  return ok;
}

// рекурсивное копирование файла или папки; dst не должен существовать
static bool fmCopyTree(const char* src, const char* dst, int depth) {
  File f = fmFsOpen(src);
  if (!f) return false;
  bool isDir = f.isDirectory();
  f.close();
  if (!isDir) return fmCopyFile(src, dst);
  if (depth > 6) return false;

  char rd[FM_PATH_LEN];
  if (!fmFsReal(dst, rd, sizeof(rd)) || !fmFsOf(dst).mkdir(rd)) return false;

  // сначала собираем имена и закрываем папку (у SD мало одновременно открытых файлов)
  char (*names)[FM_NAME_LEN] = (char (*)[FM_NAME_LEN])malloc((size_t)FM_MAX_ENTRIES * FM_NAME_LEN);
  if (!names) return false;
  int cnt = 0;
  bool ok = true;
  File d = fmFsOpen(src);
  if (!d) { free(names); return false; }
  File c = d.openNextFile();
  while (c) {
    const char* b = fmBaseName(c.name());
    size_t bl = strlen(b);
    if (bl > 0) {
      if (cnt < FM_MAX_ENTRIES && bl < FM_NAME_LEN) strcpy(names[cnt++], b);
      else ok = false;                                           // не поместилось — копия была бы неполной
    }
    c.close();
    c = d.openNextFile();
  }
  d.close();

  for (int i = 0; i < cnt && ok; i++) {
    char s2[FM_PATH_LEN], d2[FM_PATH_LEN];
    if (!fmJoin(s2, sizeof(s2), src, names[i]) || !fmJoin(d2, sizeof(d2), dst, names[i])) { ok = false; break; }
    ok = fmCopyTree(s2, d2, depth + 1);
    if (fmCancelOp) ok = false;
  }
  free(names);
  return ok;
}

// свободное имя в папке: "a.txt" -> "a_1.txt", "a_2.txt" ...
static bool fmUniqueDest(const char* dir, const char* name, char* out, size_t n) {
  if (!fmJoin(out, n, dir, name)) return false;
  if (!fmExists(out)) return true;
  const char* dot = strrchr(name, '.');
  bool hasExt = dot && dot != name && strlen(dot) <= 16;
  size_t bl = hasExt ? (size_t)(dot - name) : strlen(name);
  const char* ext = hasExt ? dot : "";
  size_t maxb = FM_NAME_LEN - 1 - strlen(ext) - 3;
  if (bl > maxb) { bl = maxb; while (bl > 0 && ((uint8_t)name[bl] & 0xC0) == 0x80) bl--; }
  for (int k = 1; k < 100; k++) {
    char cand[FM_NAME_LEN + 8];
    snprintf(cand, sizeof(cand), "%.*s_%d%s", (int)bl, name, k, ext);
    if (!fmJoin(out, n, dir, cand)) return false;
    if (!fmExists(out)) return true;
  }
  return false;
}

static void fmDoPaste() {
  if (!fmClipActive) return;
  if (!fmIsSd(fmDir) || !fmSdMounted) { fmSetMsg("NO SD CARD"); return; }
  if (!fmExists(fmClipPath)) { fmClipActive = false; fmClampSel(); fmSetMsg("SOURCE GONE"); return; }

  char parent[FM_PATH_LEN];
  strcpy(parent, fmClipPath);
  fmParentOf(parent);
  if (fmClipMove && strcmp(parent, fmDir) == 0) { fmSetMsg("SAME FOLDER"); return; }
  if (fmClipIsDir && fmUnder(fmDir, fmClipPath)) { fmSetMsg("INTO ITSELF"); return; }   // папку в себя нельзя

  char dst[FM_PATH_LEN];
  if (!fmUniqueDest(fmDir, fmBaseName(fmClipPath), dst, sizeof(dst))) { fmSetMsg("NAME ERROR"); return; }

  fmCancelOp = false;
  fmOpDrawAt = 0;
  bool ok;
  if (fmClipMove) {
    char rs[FM_PATH_LEN], rd[FM_PATH_LEN];
    ok = fmFsReal(fmClipPath, rs, sizeof(rs)) && fmFsReal(dst, rd, sizeof(rd)) && fmFsOf(dst).rename(rs, rd);
    if (!ok) {                                                  // запасной путь: копия + удаление
      fmOpTitle = "MOVING...";
      ok = fmCopyTree(fmClipPath, dst, 0);
      if (ok) ok = fmRemoveTree(fmClipPath, 0);
      else if (fmExists(dst)) fmRemoveTree(dst, 0);
    }
    fmAfterDelete();
  } else {
    fmOpTitle = "COPYING...";
    ok = fmCopyTree(fmClipPath, dst, 0);
    if (!ok && fmExists(dst)) fmRemoveTree(dst, 0);             // не оставляем недокопированное
  }
  if (ok && fmClipMove) fmClipActive = false;                   // после переноса буфер пуст; после копирования — можно вставить ещё раз

  fmLoadDir();
  if (ok) fmSelectByName(fmBaseName(dst)); else fmClampSel();
  fmSetMsg(ok ? (fmClipMove ? "MOVED" : "COPIED") : (fmCancelOp ? "CANCELLED" : (fmClipMove ? "MOVE FAILED" : "COPY FAILED")));
  fmPhase = FM_LIST;
  fmDirty = true;
}

// ============================================================
// Просмотр файла
// ============================================================
// ============================================================
// Видео
// ============================================================
static bool fmLooksLikeVideo(const char* path, uint32_t size) {
  if (fmEndsWithNoCase(path, ".vid") || fmEndsWithNoCase(path, ".snv")) return true;
  if ((fmEndsWithNoCase(path, ".bin") || fmEndsWithNoCase(path, ".raw")) &&
      size >= 2 * FM_VID_FRAME && size % FM_VID_FRAME == 0) return true;
  return false;
}

static bool fmVidReadFrame(uint32_t idx) {
  if (!fmVid || !fmVidBuf || idx >= fmVidFrames) return false;
  if (!fmVid.seek(fmVidHdr + (uint32_t)idx * FM_VID_FRAME)) return false;
  if (fmVid.read(fmVidBuf, FM_VID_FRAME) != FM_VID_FRAME) return false;
  fmVidCur = idx;
  fmVidDirty = true;
  return true;
}

static void fmVidExit(const char* msg) {
  fmCloseVideo();
  fmPhase = FM_LIST;
  fmDirty = true;
  if (msg) fmSetMsg(msg);
}

static void fmVidShowOverlay(const char* note) {
  fmVidNote = note;
  fmVidOverlayUntil = millis() + 2500;
  fmVidDirty = true;
}

// перейти на кадр idx и продолжить отсчёт времени с него
static void fmVidSeekTo(uint32_t idx) {
  if (idx >= fmVidFrames) idx = fmVidFrames ? fmVidFrames - 1 : 0;
  if (!fmVidReadFrame(idx)) { fmVidExit("READ ERROR"); return; }
  fmVidBase = idx;
  fmVidT0 = millis();
  fmVidEnded = false;
}

static void fmOpenVideo(const char* path, uint32_t size) {
  fmCloseVideo();
  fmVid = fmFsOpen(path, "r");
  if (!fmVid || fmVid.isDirectory()) { fmCloseVideo(); fmSetMsg("OPEN FAILED"); return; }

  fmVidHdr = 0;
  fmVidFps = FM_VID_DEF_FPS;
  uint8_t h[8];
  if (size >= 8 && fmVid.read(h, 8) == 8 && h[0] == 'S' && h[1] == 'N' && h[2] == 'L' && h[3] == 'V') {
    fmVidHdr = 8;
    if (h[4] >= 1 && h[4] <= 60) fmVidFps = h[4];
  }
  if (size <= fmVidHdr || (size - fmVidHdr) < FM_VID_FRAME) { fmCloseVideo(); fmSetMsg("BAD VIDEO"); return; }
  fmVidFrames = (size - fmVidHdr) / FM_VID_FRAME;

  fmVidBuf = (uint8_t*)malloc(FM_VID_FRAME);
  if (!fmVidBuf) { fmCloseVideo(); fmSetMsg("NO MEMORY"); return; }

  strncpy(fmViewPath, path, FM_PATH_LEN - 1);
  fmViewPath[FM_PATH_LEN - 1] = 0;

  fmVidPaused = false; fmVidEnded = false; fmVidLoop = false;
  fmVidCur = 0; fmVidBase = 0;
  fmVidT0 = millis();
  if (!fmVidReadFrame(0)) { fmVidExit("READ ERROR"); return; }
  fmVidShowOverlay("");
  fmPhase = FM_VIDEO;
}

static void fmCloseView() {
  if (fmBmp) { free(fmBmp); fmBmp = NULL; }
}

static int fmNextModeId() {
  for (int k = 1; k <= 3; k++) {
    int m = (fmViewMode + k) % 3;
    if (fmViewModes & (1 << m)) return m;
  }
  return fmViewMode;
}

static const char* fmModeName(int m) {
  return m == FM_V_TEXT ? "TXT" : (m == FM_V_HEX ? "HEX" : "BMP");
}

static void fmOpenView(const char* path) {
  if (fmIsFlash(path)) {                              // раздел флеш-памяти — только HEX
    const esp_partition_t* part = fmFindPart(fmBaseName(path));
    if (!part) { fmSetMsg("OPEN FAILED"); return; }
    fmCloseView();
    fmViewSize = (uint32_t)part->size;
    fmViewModes = (1 << FM_V_HEX);
    fmViewMode = FM_V_HEX;
    strncpy(fmViewPath, path, FM_PATH_LEN - 1);
    fmViewPath[FM_PATH_LEN - 1] = 0;
    fmViewPos = 0; fmViewMore = false;
    fmBannerUntil = millis() + 2500;
    fmPhase = FM_VIEW; fmDirty = true;
    return;
  }
  File f = fmFsOpen(path, "r");
  if (!f || f.isDirectory()) { if (f) f.close(); fmSetMsg("OPEN FAILED"); return; }

  fmCloseView();
  fmViewSize = (uint32_t)f.size();

  if (fmLooksLikeVideo(path, fmViewSize)) {           // видео — запускаем плеер
    f.close();
    fmOpenVideo(path, fmViewSize);
    return;
  }

  // текст или бинарник — по первым 256 байтам
  bool text = true;
  uint8_t buf[256];
  size_t n = f.read(buf, sizeof(buf));
  for (size_t i = 0; i < n; i++) {
    uint8_t b = buf[i];
    if (b < 9 || (b > 13 && b < 32) || b == 127) { text = false; break; }
  }
  f.close();

  fmViewModes = 0;
  if (text || fmViewSize == 0) fmViewModes |= (1 << FM_V_TEXT);
  if (fmViewSize > 0)          fmViewModes |= (1 << FM_V_HEX);

  if (fmViewSize == FM_BMP_BYTES) {          // 128x64 монохром — можно показать картинкой
    fmBmp = (uint8_t*)malloc(FM_BMP_BYTES);
    if (fmBmp) {
      File g = fmFsOpen(path, "r");
      bool ok = g && (g.read(fmBmp, FM_BMP_BYTES) == FM_BMP_BYTES);
      if (g) g.close();
      if (ok) fmViewModes |= (1 << FM_V_BMP);
      else { free(fmBmp); fmBmp = NULL; }
    }
  }

  strncpy(fmViewPath, path, FM_PATH_LEN - 1);
  fmViewPath[FM_PATH_LEN - 1] = 0;

  if ((fmViewModes & (1 << FM_V_BMP)) && fmEndsWithNoCase(path, ".bin")) fmViewMode = FM_V_BMP;
  else if (fmViewModes & (1 << FM_V_TEXT)) fmViewMode = FM_V_TEXT;
  else fmViewMode = FM_V_HEX;

  fmViewPos = 0;
  fmViewMore = false;
  fmBannerUntil = millis() + 2500;
  fmPhase = FM_VIEW;
  fmDirty = true;
}

// Читает до rows строк текста (UTF-8), начиная со строки start (с переносом по FM_COLS).
// Каждая ячейка — код символа для fmDrawCell, строка завершается нулём.
static int fmReadText(const char* path, int start, int rows, uint16_t out[][FM_COLS + 1]) {
  File f = fmFsOpen(path, "r");
  if (!f) return 0;

  int line = 0, got = 0, col = 0;
  uint16_t cur[FM_COLS + 1];
  uint8_t buf[64];
  bool done = false;
  uint32_t cp = 0;     // накопленный код символа UTF-8
  int need = 0;        // сколько ещё продолжающих байт ждём

  #define FM_EMIT() do {     cur[col] = 0;     if (line >= start && got < rows) { memcpy(out[got], cur, (col + 1) * sizeof(uint16_t)); got++; }     line++; col = 0;     if (got >= rows) done = true;   } while (0)
  #define FM_PUT(code) do {     if (col >= FM_COLS) { FM_EMIT(); }     if (!done) cur[col++] = (code);   } while (0)

  while (!done) {
    size_t n = f.read(buf, sizeof(buf));
    if (n == 0) break;
    for (size_t i = 0; i < n && !done; i++) {
      uint8_t c = buf[i];
      if (need > 0) {                                   // ждём продолжение многобайтового символа
        if ((c & 0xC0) == 0x80) {
          cp = (cp << 6) | (c & 0x3F);
          if (--need == 0) { if (cp != 0xFEFF) FM_PUT(fmCellFor(cp)); }
          continue;
        }
        need = 0; FM_PUT('?');                           // обрыв последовательности
        if (done) break;
      }
      if (c == '\r') continue;
      if (c == '\n') { FM_EMIT(); continue; }
      if (c == '\t') c = ' ';
      if (c < 32 || c == 127) c = '.';
      if (c < 127)              { FM_PUT(c); }
      else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; need = 1; }
      else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; need = 2; }
      else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; need = 3; }
      else                      { FM_PUT('?'); }
    }
  }
  if (!done && col > 0) FM_EMIT();
  #undef FM_PUT
  #undef FM_EMIT

  f.close();
  return got;
}

// ============================================================
// Отрисовка
// ============================================================
static void fmDrawList() {
  fmBegin();
  fmHeader(fmDir);

  int n = fmItemCount();
  for (int row = 0; row < FM_ROWS; row++) {
    int i = fmTop + row;
    if (i >= n) break;
    FmEntry* e = fmItem(i);
    char nm[FM_NAME_LEN + 2];
    char sz[8];
    if (!e) {
      strcpy(nm, "..");
      sz[0] = 0;
    } else if (fmIsUploadRow(e)) {
      strcpy(nm, "+ UPLOAD");
      strcpy(sz, "WI-FI");
    } else if (fmIsPasteRow(e)) {
      strcpy(nm, "+ PASTE");
      strcpy(sz, fmClipMove ? "MOVE " : "COPY ");        // пробел в конце: без перевода, влезает в 5 символов
    } else {
      snprintf(nm, sizeof(nm), "%s%s", e->name, e->isDir ? "/" : "");
      if (e->isDir) strcpy(sz, "<DIR>");
      else fmFmtSize(sz, sizeof(sz), e->size);
    }
    if (fmU8Len(nm) > 12) { char* cut = (char*)fmU8Skip(nm, 11); cut[0] = '~'; cut[1] = 0; }   // по символам, не по байтам
    const int y = 12 + row * 9;
    if (i == fmSel) fmPrintAt(2, y, ">");
    if (!e || (!fmIsUploadRow(e) && !fmIsPasteRow(e)))          // иконка папки у каждого пункта
      display.drawBitmap(10, y, iconFolderSmall, FM_ICON_W, FM_ICON_H, whiteTheme ? BLACK : WHITE);
    fmPrintAt(21, y, nm);                                       // имя — после иконки
    fmPrintAt(128 - 2 - 5 * 6, y, sz);                          // размер — у правого края
  }

  if (fmCount == 0) {                                  // пустая папка: метка после служебных строк
    int row = n - fmTop;
    if (row >= 0 && row < FM_ROWS) fmPrintAt(2, 12 + row * 9, (fmIsSd(fmDir) && fmSdErr) ? "  (NO SD CARD)" : "  (EMPTY)");
  }

  // нижняя строка: сообщение или полный путь выбранного пункта
  display.drawFastHLine(0, 55, 128, whiteTheme ? BLACK : WHITE);
  fmMsgShown = (millis() < fmMsgUntil);
  fmMarqActive = false;
  if (fmMsgShown) {
    fmPrintAt(2, 57, fmMsg);
  } else if (n > 0) {
    char full[FM_PATH_LEN];
    if (fmItemPath(full, sizeof(full), fmSel)) {
      fmMarqActive = fmU8Len(full) > FM_COLS;
      fmMarquee(2, 57, full);
    }
  }
  if (fmSkipped > 0 && !fmMsgShown && n == 0) fmPrintAt(2, 57, "TOO MANY FILES");
  display.display();
}

static void fmFmtTime(char* out, size_t n, uint32_t sec) {
  snprintf(out, n, "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

static void fmDrawVideo() {
  fmBegin();
  if (fmVidBuf) display.drawBitmap(0, 0, fmVidBuf, 128, 64, whiteTheme ? BLACK : WHITE);

  fmVidOverlayShown = (millis() < fmVidOverlayUntil) || fmVidPaused;
  if (fmVidOverlayShown) {                          // плашка: время, путь/состояние и полоса прогресса
    drawRect(0, 52, 128, 12, true);
    setInverseColor();
    char a[12], b[12], line[24];
    fmFmtTime(a, sizeof(a), fmVidCur / fmVidFps);
    fmFmtTime(b, sizeof(b), fmVidFrames / fmVidFps);
    const char* st = fmVidEnded ? "END" : (fmVidPaused ? "PAUSE" : (fmVidNote[0] ? fmVidNote : (fmVidLoop ? "LOOP" : "PLAY")));
    snprintf(line, sizeof(line), "%s/%s %s", a, b, st);
    display.setCursor(2, 53);
    display.print(line);
    uint32_t w = fmVidFrames > 1 ? (uint32_t)(((uint64_t)fmVidCur * 124) / (fmVidFrames - 1)) : 124;
    display.fillRect(2, 61, (int)w, 3, INVERSE);                         // пройденная часть
    if (w < 124) display.drawFastHLine(2 + (int)w, 62, 124 - (int)w, INVERSE);   // линия до конца
    setDrawColor();
  }
  display.display();
}

#define FM_MENU_N 5
static void fmDrawMenu() {
  fmBegin();
  fmHeader(fmTarget);

  const char* items[FM_MENU_N] = { "OPEN", "INFO", "COPY", "MOVE", "DELETE" };
  for (int i = 0; i < FM_MENU_N; i++) {
    display.setCursor(2, 13 + i * 8);
    display.print(i == fmMenuSel ? "> " : "  ");
    display.print(items[i]);
  }
  display.drawBitmap(101, 32, iconFile26, 26, 26,
                     whiteTheme ? BLACK : WHITE);
  display.display();
}

static void fmDrawConfirm() {
  fmBegin();
  fmHeader(fmTarget);
  fmPrintAt(2, 14, fmTargetIsDir ? "DELETE FOLDER?" : "DELETE FILE?");

  char nm[FM_COLS * 4 + 1];
  fmTailText(nm, fmBaseName(fmTarget));
  fmPrintAt(2, 24, nm);

  if (!fmTargetIsDir && !fmIsFlash(fmTarget)) {
    display.drawBitmap(100, 12, iconFile26, 26, 26,
                       whiteTheme ? BLACK : WHITE);
  }

  if (fmTargetIsDir) fmPrintAt(2, 34, "WITH ALL CONTENTS!");
  fmPrintAt(2, 45, "SELECT: YES");
  fmPrintAt(2, 55, "BACK:   NO");
  display.display();
}

static void fmDrawInfo() {
  fmBegin();
  fmHeader("INFO");
  for (int i = 0; i < 3; i++) {
    if (fmU8Len(fmTarget) > (size_t)(i * FM_COLS)) fmPrintAt(2, 13 + i * 9, fmTarget, i * FM_COLS);
  }
  char sz[10], line[48];
  if (fmTargetIsDir) snprintf(line, sizeof(line), "FOLDER");
  else { fmFmtSize(sz, sizeof(sz), fmTargetSize); snprintf(line, sizeof(line), "%s  %s", fmIsFlash(fmTarget) ? "PART" : "FILE", sz); }
  fmPrintAt(2, 41, line);

  if (fmIsFlash(fmTarget) && !fmTargetIsDir) {
    const esp_partition_t* part = fmFindPart(fmBaseName(fmTarget));
    if (part) snprintf(line, sizeof(line), "OFS: 0x%06lX", (unsigned long)part->address);
    else      snprintf(line, sizeof(line), "READ-ONLY");
    fmPrintAt(2, 50, line);
  } else if (fmIsSd(fmTarget)) {
    char used[10], total[10];
    fmFmtBig(used, sizeof(used), fmSdMounted ? (uint64_t)SD.usedBytes() : 0);
    fmFmtBig(total, sizeof(total), fmSdMounted ? (uint64_t)SD.totalBytes() : 0);
    snprintf(line, sizeof(line), "SD: %s/%s", used, total);
    fmPrintAt(2, 50, line);
  } else if (fmIsLfs(fmTarget)) {
    char used[10], total[10];
    fmFmtSize(used, sizeof(used), (uint32_t)LittleFS.usedBytes());
    fmFmtSize(total, sizeof(total), (uint32_t)LittleFS.totalBytes());
    snprintf(line, sizeof(line), "FS: %s/%s", used, total);
    fmPrintAt(2, 50, line);
  }
  display.display();
}

static void fmDrawView() {
  fmBegin();

  // ----- картинка 128x64 -----
  if (fmViewMode == FM_V_BMP && fmBmp) {
    display.drawBitmap(0, 0, fmBmp, 128, 64, whiteTheme ? BLACK : WHITE);
    fmBannerShown = (millis() < fmBannerUntil);
    if (fmBannerShown) {                       // плашка с путём — на пару секунд (UP/DOWN — показать снова)
      drawRect(0, 54, 128, 10, true);
      setInverseColor();
      char tmp[FM_COLS * 4 + 1];
      fmTailText(tmp, fmViewPath);
      display.setCursor(2, 55);
      display.print(tmp);
      setDrawColor();
    }
    display.display();
    return;
  }
  fmBannerShown = false;

  fmHeader(fmViewPath);

  char foot[48];
  char left[24];

  if (fmViewMode == FM_V_TEXT) {
    uint16_t lines[FM_ROWS + 1][FM_COLS + 1];
    int n = fmReadText(fmViewPath, fmViewPos, FM_ROWS + 1, lines);
    fmViewMore = (n > FM_ROWS);
    if (fmViewSize == 0) fmPrintAt(2, 12, "(EMPTY FILE)");
    for (int i = 0; i < n && i < FM_ROWS; i++) fmPrintCells(2, 12 + i * 9, lines[i]);
    snprintf(left, sizeof(left), "TXT L%d", fmViewPos + 1);
  } else {
    uint8_t b[4 * FM_ROWS];
    size_t n = 0;
    n = fmReadAt(fmViewPath, (uint32_t)fmViewPos * 4, b, sizeof(b));
    for (size_t r = 0; r * 4 < n; r++) {
      char line[FM_COLS + 4];
      char asc[5] = "    ";
      size_t cnt = n - r * 4; if (cnt > 4) cnt = 4;
      char hex[9] = "        ";
      for (size_t k = 0; k < cnt; k++) {
        uint8_t v = b[r * 4 + k];
        snprintf(hex + k * 2, 3, "%02X", v);
        asc[k] = (v >= 32 && v < 127) ? (char)v : '.';
      }
      // snprintf дописал завершающий ноль — пробелами добиваем недостающее
      for (size_t k = cnt * 2; k < 8; k++) hex[k] = ' ';
      hex[8] = 0;
      asc[cnt] = 0;
      snprintf(line, sizeof(line), "%06lX %s %s", (unsigned long)(((uint32_t)fmViewPos + r) * 4), hex, asc);
      fmPrintAt(2, 12 + r * 9, line);
    }
    fmViewMore = ((uint32_t)(fmViewPos + FM_ROWS) * 4 < fmViewSize);
    unsigned pct = fmViewSize ? (unsigned)(((uint64_t)fmViewPos * 4 * 100) / fmViewSize) : 0;
    if (pct > 100) pct = 100;
    snprintf(left, sizeof(left), "HEX %u%%", pct);
  }

  int nxt = fmNextModeId();
  if (nxt != fmViewMode) snprintf(foot, sizeof(foot), "%-9s SEL:%s", left, fmModeName(nxt));
  else                   snprintf(foot, sizeof(foot), "%s", left);
  fmPrintAt(2, 57, foot);
  display.display();
}

static void fmDrawNoFs() {
  fmBegin();
  fmHeader("FILES");
  fmPrintAt(2, 18, "NO SD CARD");
  fmPrintAt(2, 32, "SELECT: RETRY");
  fmPrintAt(2, 42, "BACK:   EXIT");
  if (millis() < fmMsgUntil) fmPrintAt(2, 56, fmMsg);
  display.display();
}

// ============================================================
// Wi-Fi загрузка файлов (веб-сервер)
// Строка "+ UPLOAD" в любой папке /littlefs поднимает точку доступа
// SNL-GAMING и страницу загрузки. Файлы (.h .txt .ino .json ... любые)
// сохраняются в выбранную папку, потом их можно открыть в проводнике.
// ============================================================
const char FM_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SNL Files</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;max-width:480px;margin:0 auto;padding:12px}
h2{margin:6px 0}label{display:block;margin:12px 0 2px;font-size:14px;color:#bbb}
input[type=text]{width:100%;box-sizing:border-box;font-size:16px;padding:10px}
#dz{border:2px dashed #666;border-radius:8px;padding:30px 10px;text-align:center;margin:12px 0;color:#bbb}
#dz.on{border-color:#2a6;color:#fff}
#log div{border-bottom:1px solid #333;padding:6px 0;font-size:14px;word-break:break-all}
.small{font-size:13px;color:#aaa}
</style></head><body>
<h2>SNL GAMING - Файлы</h2>
<label>Папка на SD-карте: /sd/... (создаётся автоматически)</label>
<input type="text" id="dir" value="/">
<div id="dz">Нажмите или перетащите файлы сюда<br><span class="small">.h .txt .ino .json и любые другие</span></div>
<input type="file" id="f" multiple hidden>
<div id="log"></div>
<script>
const $=i=>document.getElementById(i);
fetch('/cur').then(r=>r.text()).then(t=>{$('dir').value=t}).catch(()=>{});
function add(t){const d=document.createElement('div');d.textContent=t;$('log').prepend(d);return d}
function up(file){return new Promise(res=>{
const d=add(file.name+' ...'),x=new XMLHttpRequest();
x.open('POST','/upload?dir='+encodeURIComponent($('dir').value));
x.upload.onprogress=e=>{if(e.lengthComputable)d.textContent=file.name+' '+Math.round(e.loaded*100/e.total)+'%'};
x.onload=()=>{d.textContent=file.name+': '+x.responseText;res()};
x.onerror=()=>{d.textContent=file.name+': ошибка связи';res()};
const fd=new FormData();fd.append('file',file,file.name);x.send(fd)})}
async function go(fs){for(const f of fs)await up(f)}
$('dz').onclick=()=>$('f').click();
$('f').onchange=e=>{go(e.target.files)};
['dragover','dragleave','drop'].forEach(n=>$('dz').addEventListener(n,e=>{e.preventDefault();
$('dz').className=n=='dragover'?'on':'';if(n=='drop')go(e.dataTransfer.files)}));
</script></body></html>)rawliteral";

static WebServer fmServer(80);
static bool fmSrvRoutes = false, fmSrvRunning = false, fmSrvAp = false;
static File fmUpFile;
static fs::FS* fmUpFs = &SD;           // пишем только на SD-карту
static bool fmUpActive = false, fmUpFail = false;
static char fmUpName[FM_NAME_LEN] = "";
static char fmUpPath[FM_PATH_LEN] = "";
static char fmUpErr[24] = "";
static uint32_t fmUpBytes = 0;
static int fmUpCount = 0, fmUpErrors = 0;
static char fmUpLast[64] = "";

static bool fmSanitizeName(const char* in, char* out, size_t n) {   // n = FM_NAME_LEN
  const char* b = in;
  for (const char* q = in; *q; q++) if (*q == '/' || *q == '\\') b = q + 1;   // только имя, без пути
  char tmp[96];
  size_t k = 0;
  for (; *b && k < sizeof(tmp) - 1; b++) {
    unsigned char c = (unsigned char)*b;
    // байты UTF-8 (>=0x80) пропускаем как есть: русские имена сохраняются
    bool ok = c >= 0x80 || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-' || c == ' ' || c == '(' || c == ')';
    tmp[k++] = ok ? (char)c : '_';
  }
  tmp[k] = 0;
  if (k == 0 || strcmp(tmp, ".") == 0 || strcmp(tmp, "..") == 0) return false;
  if (k >= n) {                                   // слишком длинное — режем, расширение сохраняем
    const char* dot = strrchr(tmp, '.');
    size_t el = (dot && strlen(dot) <= 8) ? strlen(dot) : 0;
    size_t bl = n - 1 - el;
    while (bl > 0 && ((uint8_t)tmp[bl] & 0xC0) == 0x80) bl--;   // не режем UTF-8 посередине
    memmove(tmp + bl, tmp + k - el, el + 1);
    k = bl + el;
  }
  memcpy(out, tmp, k + 1);
  return true;
}

// безопасный путь папки внутри LittleFS: "/", "/a/b" (без "..", без странных символов)
static void fmSanitizeDir(const char* in, char* out, size_t n) {
  strcpy(out, "/");
  int depth = 0;
  const char* p = in;
  while (*p && depth < 6) {
    while (*p == '/' || *p == '\\') p++;
    if (!*p) break;
    char comp[FM_NAME_LEN];
    size_t k = 0;
    while (*p && *p != '/' && *p != '\\' && k < sizeof(comp) - 1) {
      unsigned char c = (unsigned char)*p++;
      bool ok = c >= 0x80 || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-' || c == ' ' || c == '(' || c == ')';
      comp[k++] = ok ? (char)c : '_';
    }
    while (*p && *p != '/' && *p != '\\') p++;    // хвост слишком длинного имени
    comp[k] = 0;
    if (k == 0 || strcmp(comp, ".") == 0 || strcmp(comp, "..") == 0) continue;
    char next[FM_PATH_LEN];
    if (!fmJoin(next, sizeof(next), out, comp) || strlen(next) + 1 > n) break;
    strcpy(out, next);
    depth++;
  }
}

static bool fmMkdirs(fs::FS& fsx, const char* dir) {   // создаёт все папки пути
  if (strcmp(dir, "/") == 0) return true;
  char cur[FM_PATH_LEN];
  size_t len = strlen(dir);
  if (len >= sizeof(cur)) return false;
  for (size_t i = 1; i <= len; i++) {
    if (dir[i] == '/' || dir[i] == 0) {
      memcpy(cur, dir, i);
      cur[i] = 0;
      File t = fsx.open(cur);
      bool exists = (bool)t;
      bool isDir = exists && t.isDirectory();
      if (t) t.close();
      if (exists && !isDir) return false;
      if (!exists && !fsx.mkdir(cur)) return false;
    }
  }
  return true;
}

static void fmDrawServer() {
  fmBegin();
  fmHeader("WI-FI UPLOAD");
  char line[64];
  snprintf(line, sizeof(line), "WiFi: %s", BL_AP_SSID);
  fmPrintAt(2, 12, line);
  snprintf(line, sizeof(line), "Pass: %s", BL_AP_PASS);
  fmPrintAt(2, 21, line);
  String ap = WiFi.softAPIP().toString();
  snprintf(line, sizeof(line), "http://%s", ap.c_str());
  fmPrintAt(2, 30, line);

  char to[FM_PATH_LEN + 8];
  snprintf(to, sizeof(to), "TO: %s", fmDir);
  char tail[FM_COLS * 4 + 1];
  fmTailText(tail, to);
  fmPrintAt(2, 39, tail);

  if (fmUpActive) {
    char sz[10];
    fmFmtSize(sz, sizeof(sz), fmUpBytes);
    char nm[48];
    fmU8Copy(nm, fmUpName, 10);                    // имя режем по символам, чтобы влез размер
    snprintf(line, sizeof(line), "RX %s", nm);
    char full[96];
    snprintf(full, sizeof(full), "%s %s", line, sz);
    fmPrintAt(2, 48, full);
  } else if (fmUpLast[0]) {
    fmPrintAt(2, 48, fmUpLast);
  } else if (WiFi.status() == WL_CONNECTED) {
    String lan = WiFi.localIP().toString();
    snprintf(line, sizeof(line), "LAN %s", lan.c_str());
    fmPrintAt(2, 48, line);
  } else {
    fmPrintAt(2, 48, "WAITING...");
  }
  fmPrintAt(2, 57, "BACK: STOP");
  display.display();
}

static void fmUploadFail(const char* why) {
  fmUpFail = true;
  strncpy(fmUpErr, why, sizeof(fmUpErr) - 1);
  fmUpErr[sizeof(fmUpErr) - 1] = 0;
}

// приём файла (вызывается WebServer по частям)
static void fmUploadData() {
  HTTPUpload& up = fmServer.upload();

  if (up.status == UPLOAD_FILE_START) {
    fmUpActive = true; fmUpFail = false; fmUpBytes = 0; fmUpErr[0] = 0;
    fmUpName[0] = 0; fmUpPath[0] = 0;
    if (fmUpFile) fmUpFile.close();

    char name[FM_NAME_LEN], dir[FM_PATH_LEN], path[FM_PATH_LEN];
    String dirArg = fmServer.hasArg("dir") ? fmServer.arg("dir") : String("/");
    if (!fmSanitizeName(up.filename.c_str(), name, sizeof(name))) { fmUploadFail("BAD NAME"); return; }
    fmSanitizeDir(dirArg.c_str(), dir, sizeof(dir));

    // всё, что загружается, попадает на SD-карту: "/x" -> "/sd/x"
    if (!fmIsSd(dir)) {
      char t[FM_PATH_LEN];
      snprintf(t, sizeof(t), "/sd%s", strcmp(dir, "/") == 0 ? "" : dir);
      strncpy(dir, t, sizeof(dir) - 1); dir[sizeof(dir) - 1] = 0;
    }
    char realDir[FM_PATH_LEN];
    if (!fmSdMounted && !fmSdMount()) { fmUploadFail("NO SD CARD"); return; }
    fmUpFs = &SD;
    fmFsReal(dir, realDir, sizeof(realDir));
    if (!fmMkdirs(*fmUpFs, realDir)) { fmUploadFail("MKDIR FAILED"); return; }
    if (!fmJoin(path, sizeof(path), realDir, name)) { fmUploadFail("PATH TOO LONG"); return; }

    fmUpFile = fmUpFs->open(path, "w");
    if (!fmUpFile) { fmUploadFail("OPEN FAILED"); return; }
    strcpy(fmUpName, name);
    strcpy(fmUpPath, path);
    fmDirty = true;

  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (fmUpActive && !fmUpFail && fmUpFile) {
      if (fmUpFile.write(up.buf, up.currentSize) != up.currentSize) fmUploadFail("NO SPACE");
      else fmUpBytes += up.currentSize;
    }
    if (millis() - fmLastDraw > 500) { fmLastDraw = millis(); fmDrawServer(); }   // прогресс на экране

  } else if (up.status == UPLOAD_FILE_END) {
    if (fmUpFile) fmUpFile.close();
    if (fmUpFail) {
      if (fmUpPath[0]) fmUpFs->remove(fmUpPath);
      fmUpErrors++;
      snprintf(fmUpLast, sizeof(fmUpLast), "FAIL: %s", fmUpErr);
    } else {
      fmUpCount++;
      char sz[10];
      fmFmtSize(sz, sizeof(sz), fmUpBytes);
      char shortNm[48];
      fmU8Copy(shortNm, fmUpName, 10);
      snprintf(fmUpLast, sizeof(fmUpLast), "OK %s %s", shortNm, sz);
    }
    fmUpActive = false;
    fmDirty = true;

  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (fmUpFile) fmUpFile.close();
    if (fmUpPath[0]) fmUpFs->remove(fmUpPath);
    fmUpErrors++;
    strcpy(fmUpLast, "FAIL: ABORTED");
    fmUpActive = false;
    fmDirty = true;
  }
}

static void fmUploadDone() {                      // ответ браузеру после приёма файла
  if (fmUpFail || !fmUpPath[0]) {
    String m = String("ОШИБКА: ") + (fmUpErr[0] ? fmUpErr : "?");
    fmServer.send(500, "text/plain; charset=utf-8", m);
  } else {
    String m = String("OK -> ") + (fmUpFs == &SD ? "/sd" : "") + fmUpPath + " (" + String((unsigned long)fmUpBytes) + " байт)";
    fmServer.send(200, "text/plain; charset=utf-8", m);
  }
}

static void fmHandleRoot() { fmServer.send_P(200, "text/html; charset=utf-8", FM_PAGE); }

static void fmHandleCur() {                       // папка, открытая сейчас в проводнике (с префиксом /littlefs или /sd)
  fmServer.send(200, "text/plain", fmDir);
}

static void fmStartServer() {
  if (fmSrvRunning) return;
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAPdisconnect(true);
  delay(100);
  fmSrvAp = WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  if (!fmSrvAp) {
    WiFi.mode(WIFI_AP);
    delay(100);
    fmSrvAp = WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  }
  if (!fmSrvRoutes) {
    fmServer.on("/", HTTP_GET, fmHandleRoot);
    fmServer.on("/cur", HTTP_GET, fmHandleCur);
    fmServer.on("/upload", HTTP_POST, fmUploadDone, fmUploadData);
    fmSrvRoutes = true;
  }
  fmServer.begin();
  fmSrvRunning = true;
}

static void fmStopServer() {
  if (!fmSrvRunning) return;
  if (fmUpFile) fmUpFile.close();
  fmServer.stop();
  fmSrvRunning = false;
  if (fmSrvAp) {
    WiFi.softAPdisconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    startSavedWifi();
    fmSrvAp = false;
  }
}

static void fmStartUpload() {
  if (!fmCanUpload()) return;
  fmUpCount = 0; fmUpErrors = 0; fmUpLast[0] = 0; fmUpActive = false;
  fmStartServer();
  fmPhase = FM_SERVER;
  fmLastDraw = 0;
  fmDirty = true;
}

// ============================================================
// Обработка кнопок
// ============================================================
static void fmOpenMenuFor(int idx) {
  FmEntry* e = fmItem(idx);
  if (e && fmIsPasteRow(e)) {            // RIGHT на "+ PASTE" — отменить копирование/перенос
    fmClipActive = false; fmClampSel(); fmSetMsg("CLIPBOARD CLEARED"); return;
  }
  if (!e || fmIsUploadRow(e)) return;   // для ".." и строки UPLOAD меню не нужно
  if (!fmItemPath(fmTarget, sizeof(fmTarget), idx)) { fmSetMsg("PATH TOO LONG"); return; }
  fmTargetIsDir = e->isDir;
  fmTargetSize  = e->size;
  fmMenuSel = 0;
  fmPhase = FM_MENU;
  fmDirty = true;
}

static void fmActivate(int idx) {       // SELECT в списке
  FmEntry* e = fmItem(idx);
  if (!e) { fmGoUp(); return; }
  if (fmIsUploadRow(e)) { fmStartUpload(); return; }
  if (fmIsPasteRow(e)) { fmDoPaste(); return; }
  char path[FM_PATH_LEN];
  if (!fmJoin(path, sizeof(path), fmDir, e->name)) { fmSetMsg("PATH TOO LONG"); return; }
  if (e->isDir) fmEnterDir(path);
  else          fmOpenView(path);
}

static void fmRunList() {
  const int ph = fmPhase;
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    int n = fmItemCount();
    if (fmBtn(BTN_UP)) {
      lastButtonPress = now;
      if (n > 0) { fmSel = (fmSel - 1 + n) % n; fmClampSel(); fmMarqStart = now; fmDirty = true; }
    } else if (fmBtn(BTN_DOWN)) {
      lastButtonPress = now;
      if (n > 0) { fmSel = (fmSel + 1) % n; fmClampSel(); fmMarqStart = now; fmDirty = true; }
    } else if (fmBtn(BTN_SELECT)) {
      lastButtonPress = now;
      if (n > 0) fmActivate(fmSel);
    } else if (fmBtn(BTN_RIGHT)) {
      lastButtonPress = now;
      if (n > 0) fmOpenMenuFor(fmSel);
    } else if (fmBtn(BTN_LEFT)) {
      lastButtonPress = now;
      fmGoUp();
    } else if (fmBtn(BTN_BACK)) {
      lastButtonPress = now;
      if (fmHasUp()) fmGoUp();
      else { fmCloseView(); fmCloseVideo(); fmSdUnmount(); changeState(APPS_MENU); return; }
    }
  }

  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmMsgShown && millis() >= fmMsgUntil) fmDirty = true;      // сообщение погасло
  if (fmDirty || (fmMarqActive && millis() - fmLastDraw > 250)) {
    fmDirty = false;
    fmLastDraw = millis();
    fmDrawList();
  }
}

static void fmRunMenu() {
  const int ph = fmPhase;
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (fmBtn(BTN_UP)) {
      lastButtonPress = now; fmMenuSel = (fmMenuSel + FM_MENU_N - 1) % FM_MENU_N; fmDirty = true;
    } else if (fmBtn(BTN_DOWN)) {
      lastButtonPress = now; fmMenuSel = (fmMenuSel + 1) % FM_MENU_N; fmDirty = true;
    } else if (fmBtn(BTN_SELECT)) {
      lastButtonPress = now;
      if (fmMenuSel == 0) {                       // OPEN
        if (fmTargetIsDir) { fmPhase = FM_LIST; fmEnterDir(fmTarget); }
        else               { fmPhase = FM_LIST; fmOpenView(fmTarget); }
      } else if (fmMenuSel == 1) {                // INFO
        fmPhase = FM_INFO; fmDirty = true;
      } else if (fmMenuSel == 2 || fmMenuSel == 3) { // COPY / MOVE
        bool mv = (fmMenuSel == 3);
        if (!fmIsSd(fmTarget) || (mv && !fmCanDelete(fmTarget))) { fmSetMsg("READ-ONLY"); fmPhase = FM_LIST; }
        else {
          strcpy(fmClipPath, fmTarget);
          fmClipIsDir = fmTargetIsDir; fmClipMove = mv; fmClipActive = true;
          fmPhase = FM_LIST; fmDirty = true;
          fmSetMsg("PICK FOLDER > PASTE");
        }
      } else {                                    // DELETE
        if (fmCanDelete(fmTarget)) { fmPhase = FM_CONFIRM; fmDirty = true; }
        else { fmSetMsg("READ-ONLY"); fmPhase = FM_LIST; }
      }
    } else if (fmBtn(BTN_BACK) || fmBtn(BTN_LEFT)) {
      lastButtonPress = now; fmPhase = FM_LIST; fmDirty = true;
    }
  }
  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmDirty) { fmDirty = false; fmDrawMenu(); }
}

static void fmRunConfirm() {
  const int ph = fmPhase;
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (fmBtn(BTN_SELECT)) {
      lastButtonPress = now;
      fmDoDelete();
      return;
    } else if (fmBtn(BTN_BACK) || fmBtn(BTN_LEFT)) {
      lastButtonPress = now; fmPhase = FM_LIST; fmDirty = true;
    }
  }
  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmDirty) { fmDirty = false; fmDrawConfirm(); }
}

static void fmRunInfo() {
  const int ph = fmPhase;
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (fmBtn(BTN_BACK) || fmBtn(BTN_SELECT) || fmBtn(BTN_LEFT)) {
      lastButtonPress = now; fmPhase = FM_LIST; fmDirty = true;
    }
  }
  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmDirty) { fmDirty = false; fmDrawInfo(); }
}

static void fmRunView() {
  const int ph = fmPhase;
  unsigned long now = millis();
  const bool slow = (now - lastButtonPress > DEBOUNCE_DELAY);   // BACK / SELECT
  const bool fast = (now - lastButtonPress > 110);              // прокрутка — быстрее
  if (fast) {
    if (slow && fmBtn(BTN_BACK)) {
      lastButtonPress = now;
      fmCloseView();
      fmPhase = FM_LIST; fmDirty = true;
      return;
    } else if (slow && fmBtn(BTN_SELECT)) {
      lastButtonPress = now;
      int nxt = fmNextModeId();
      if (nxt != fmViewMode) {
        fmViewMode = nxt; fmViewPos = 0;
        fmBannerUntil = millis() + 2500;
        fmDirty = true;
      }
    } else if (fmViewMode == FM_V_BMP) {
      if (fmBtn(BTN_UP) || fmBtn(BTN_DOWN)) {
        lastButtonPress = now; fmBannerUntil = millis() + 2500; fmDirty = true;
      }
    } else if (fmBtn(BTN_UP)) {
      lastButtonPress = now;
      if (fmViewPos > 0) { fmViewPos--; fmDirty = true; }
    } else if (fmBtn(BTN_DOWN)) {
      lastButtonPress = now;
      if (fmViewMore) { fmViewPos++; fmDirty = true; }
    } else if (fmBtn(BTN_LEFT)) {
      lastButtonPress = now;
      if (fmViewPos > 0) { fmViewPos -= FM_ROWS; if (fmViewPos < 0) fmViewPos = 0; fmDirty = true; }
    } else if (fmBtn(BTN_RIGHT)) {
      lastButtonPress = now;
      if (fmViewMore) { fmViewPos += FM_ROWS; fmDirty = true; }
    }
  }

  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmBannerShown && millis() >= fmBannerUntil) fmDirty = true;   // плашка с путём пропадает
  if (fmDirty || (fmBannerShown && millis() - fmLastDraw > 250)) {
    fmDirty = false;
    fmLastDraw = millis();
    fmDrawView();
  }
}

static void fmRunVideo() {
  unsigned long now = millis();
  const bool slow = (now - lastButtonPress > DEBOUNCE_DELAY);
  const bool fast = (now - lastButtonPress > 150);

  if (slow && fmBtn(BTN_BACK)) {
    lastButtonPress = now;
    fmVidExit(NULL);
    return;
  } else if (slow && fmBtn(BTN_SELECT)) {
    lastButtonPress = now;
    if (fmVidEnded) {                               // после конца — заново
      fmVidPaused = false;
      fmVidSeekTo(0);
    } else {
      fmVidPaused = !fmVidPaused;
      if (!fmVidPaused) { fmVidBase = fmVidCur; fmVidT0 = millis(); }   // продолжаем с текущего кадра
    }
    fmVidShowOverlay("");
  } else if (slow && fmBtn(BTN_UP)) {
    lastButtonPress = now;
    fmVidLoop = !fmVidLoop;
    fmVidShowOverlay(fmVidLoop ? "LOOP ON" : "LOOP OFF");
  } else if (slow && fmBtn(BTN_DOWN)) {
    lastButtonPress = now;
    fmVidShowOverlay("");
  } else if (fast && fmBtn(BTN_LEFT)) {
    lastButtonPress = now;
    uint32_t step = (uint32_t)fmVidFps * FM_VID_SEEK_SEC;
    fmVidSeekTo(fmVidCur > step ? fmVidCur - step : 0);
    if (fmPhase != FM_VIDEO) return;
    fmVidShowOverlay("<<");
  } else if (fast && fmBtn(BTN_RIGHT)) {
    lastButtonPress = now;
    fmVidSeekTo(fmVidCur + (uint32_t)fmVidFps * FM_VID_SEEK_SEC);
    if (fmPhase != FM_VIDEO) return;
    fmVidShowOverlay(">>");
  }

  // смена кадров по времени (кадры, которые не успели, пропускаются — звука нет, держим темп)
  if (!fmVidPaused && !fmVidEnded) {
    uint32_t target = fmVidBase + (uint32_t)(((uint64_t)(millis() - fmVidT0) * fmVidFps) / 1000);
    if (target >= fmVidFrames) {
      if (fmVidLoop) {
        fmVidSeekTo(0);
        if (fmPhase != FM_VIDEO) return;
      } else {
        fmVidPaused = true; fmVidEnded = true;      // остаёмся на последнем кадре
        fmVidShowOverlay("");
      }
    } else if (target > fmVidCur) {
      if (!fmVidReadFrame(target)) { fmVidExit("READ ERROR"); return; }   // карта пропала
    }
  }

  if (fmVidOverlayShown && !fmVidPaused && millis() >= fmVidOverlayUntil) fmVidDirty = true;   // плашка гаснет
  if (fmVidDirty) {
    fmVidDirty = false;
    fmDrawVideo();
  }
}

static void fmRunNoFs() {
  const int ph = fmPhase;
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (fmBtn(BTN_SELECT)) {
      lastButtonPress = now;
      if (fmSdMount()) { fmPhase = FM_LIST; fmEnterDir(FM_ROOT); }
      else fmSetMsg("NO SD CARD");
      fmDirty = true;
    } else if (fmBtn(BTN_BACK)) {
      lastButtonPress = now;
      fmSdUnmount();
      changeState(APPS_MENU);
      return;
    }
  }
  if (fmPhase != ph) { fmDirty = true; return; }   // экран сменился — рисуем его на следующем проходе
  if (fmDirty || (millis() < fmMsgUntil)) { fmDirty = false; fmDrawNoFs(); }
}

static void fmRunServer() {
  fmServer.handleClient();
  unsigned long now = millis();

  if (now - lastButtonPress > DEBOUNCE_DELAY && fmBtn(BTN_BACK)) {
    lastButtonPress = now;
    fmStopServer();
    fmLoadDir();                                  // показать только что загруженные файлы
    fmClampSel();
    fmPhase = FM_LIST;
    fmDirty = true;
    return;
  }

  if (fmDirty || now - fmLastDraw > 400) {
    fmDirty = false;
    fmLastDraw = now;
    fmDrawServer();
  }
}

// ============================================================
// Точки входа
// ============================================================
void initFileManager() {
  fmStopServer();
  fmCloseView();
  fmCloseVideo();
  fmSdUnmount();
  fmSdErr = false;
  fmPhase = FM_LIST;
  strcpy(fmDir, FM_ROOT);
  fmSel = 0; fmTop = 0; fmMenuSel = 0;
  fmClipActive = false;
  fmMsg[0] = 0; fmMsgUntil = 0; fmMsgShown = false; fmBannerShown = false;
  fmMarqStart = millis();
  fmDirty = true;
  if (!fmSdMount()) { fmPhase = FM_NOFS; return; }     // сразу открываем SD-карту
  fmLoadDir();
}

void runFileManager() {
  switch (fmPhase) {
    case FM_MENU:    fmRunMenu();    break;
    case FM_CONFIRM: fmRunConfirm(); break;
    case FM_INFO:    fmRunInfo();    break;
    case FM_VIEW:    fmRunView();    break;
    case FM_NOFS:    fmRunNoFs();    break;
    case FM_SERVER:  fmRunServer();  break;
    case FM_VIDEO:   fmRunVideo();   break;
    default:         fmRunList();    break;
  }
}

#endif
