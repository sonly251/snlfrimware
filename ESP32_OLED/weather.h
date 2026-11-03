#ifndef _weather_h
#define _weather_h

#include <WiFi.h>

// 1 — при редиректе http->https делать запрос по TLS (WiFiClientSecure).
// Если линкер ругается на IRAM (iram0_0_seg overflowed) — поставь 0.
#define WX_USE_TLS 1
#if WX_USE_TLS
#include <WiFiClientSecure.h>
#endif
#include <time.h>
#include <math.h>

// объявления из основного скетча / netapps.h
void changeState(int state);

// ---------- битмапы погоды (32x24) и иконка приложения (26x26) ----------
// Sun 32x24
const unsigned char wxBmpSun[] PROGMEM = {
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x1f, 0xf8, 0x00,
 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00,
 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x1f, 0xf8, 0x00,
 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Moon 32x24
const unsigned char wxBmpMoon[] PROGMEM = {
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x0f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00,
 0x00, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00,
 0x01, 0xfc, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
 0x00, 0xff, 0xc0, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x0f, 0xc0, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// PartD 32x24
const unsigned char wxBmpPartD[] PROGMEM = {
 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x40, 0x08, 0x00, 0x00, 0x27, 0x90,
 0x00, 0x00, 0x1f, 0xe0, 0x00, 0x00, 0x1f, 0xe0, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x3e, 0x3f, 0xf0,
 0x00, 0x7f, 0x9f, 0xf2, 0x00, 0xff, 0xc0, 0xf0, 0x01, 0xff, 0xfe, 0x60, 0x01, 0xff, 0xff, 0x60,
 0x03, 0xff, 0xff, 0x10, 0x07, 0xff, 0xff, 0x88, 0x0f, 0xff, 0xff, 0x84, 0x1f, 0xff, 0xff, 0x80,
 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0x80,
 0x0f, 0xff, 0xff, 0x80, 0x07, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00
};

// PartN 32x24
const unsigned char wxBmpPartN[] PROGMEM = {
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0c, 0x00,
 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x3e, 0x38, 0x00,
 0x00, 0x7f, 0x9c, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x01, 0xff, 0xfe, 0x10, 0x01, 0xff, 0xff, 0x60,
 0x03, 0xff, 0xff, 0x00, 0x07, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x1f, 0xff, 0xff, 0x80,
 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0x80,
 0x0f, 0xff, 0xff, 0x80, 0x07, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Cloud 32x24
const unsigned char wxBmpCloud[] PROGMEM = {
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x1f, 0xe0, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0xff, 0xff, 0xc0,
 0x00, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8,
 0x07, 0xff, 0xff, 0xf8, 0x0f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfc,
 0x0f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xf8,
 0x03, 0xff, 0xff, 0xe0, 0x00, 0x7f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Clouds 32x24
const unsigned char wxBmpClouds[] PROGMEM = {
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00,
 0x00, 0x00, 0xff, 0xe0, 0x00, 0x0e, 0x3f, 0xf0, 0x00, 0x3f, 0x9f, 0xf8, 0x00, 0x7f, 0xc0, 0xf8,
 0x00, 0xff, 0xfe, 0x38, 0x01, 0xff, 0xff, 0xbc, 0x01, 0xff, 0xff, 0x9c, 0x01, 0xff, 0xff, 0xdc,
 0x07, 0xff, 0xff, 0xc8, 0x0f, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xe0,
 0x1f, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xc0,
 0x07, 0xff, 0xff, 0x00, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// app icon 26x26
const unsigned char iconWeather[] PROGMEM = {
 0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x40,
 0x80, 0x00, 0x80, 0x40, 0x80, 0x08, 0x04, 0x40, 0x80, 0x04, 0x08, 0x40, 0x80, 0x01, 0xe0, 0x40,
 0x80, 0x03, 0xf0, 0x40, 0x80, 0x01, 0xf0, 0x40, 0x80, 0x0c, 0x7b, 0x40, 0x80, 0x3f, 0x30, 0x40,
 0x80, 0x7f, 0x90, 0x40, 0x80, 0xff, 0xc0, 0x40, 0x81, 0xff, 0xe0, 0x40, 0x87, 0xff, 0xf8, 0x40,
 0x87, 0xff, 0xf8, 0x40, 0x8f, 0xff, 0xfc, 0x40, 0x8f, 0xff, 0xfc, 0x40, 0x87, 0xff, 0xf8, 0x40,
 0x87, 0xff, 0xf8, 0x40, 0x81, 0xff, 0xe0, 0x40, 0x80, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80,
 0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00
};

// ============================================================
//  ПОГОДА (Open-Meteo, ключ не нужен)
//  * город берётся тот же, что выбран в часах (clockCityIndex)
//  * запрос идёт в фоновой задаче FreeRTOS — интерфейс не подвисает
//  * LEFT/RIGHT — страница «сейчас» / «3 дня», SELECT — обновить, BACK — выход
// ============================================================

// координаты городов (в том же порядке, что CLOCK_CITY_NAMES)
static const float WX_LAT[CLOCK_CITY_COUNT] = { 55.03f, 55.75f, 51.51f, 40.71f, 35.68f, 25.20f, 39.90f, 34.05f };
static const float WX_LON[CLOCK_CITY_COUNT] = { 82.92f, 37.62f, -0.13f, -74.01f, 139.69f, 55.27f, 116.41f, -118.24f };

enum { WI_SUN, WI_MOON, WI_PART_D, WI_PART_N, WI_CLOUDS, WI_FOG, WI_DRIZZLE, WI_RAIN, WI_HEAVY, WI_SNOW, WI_HSNOW, WI_STORM };
enum { WS_IDLE, WS_LOADING, WS_OK, WS_ERR };

struct WeatherData {
  bool valid;
  int  city;
  int  temp, feels, hum, code, isDay, wind;
  int  dCode[3], dMax[3], dMin[3];
};

static WeatherData wxData = {false, -1, 0, 0, 0, 0, 1, 0, {0,0,0}, {0,0,0}, {0,0,0}};
static WeatherData wxNew;                      // результат фоновой задачи (в wxData копируется из основного потока)
static volatile bool wxNewReady = false;
static bool wxCityMenu = false;                // выбор города кнопкой ВВЕРХ
static int  wxMenuIdx = 0;
static volatile int  wxStatus = WS_IDLE;
static volatile bool wxBusy = false;
static unsigned long wxFetchedAt = 0;
static unsigned long wxErrAt = 0;
static int   wxReqCity = 0;
static int   wxPage = 0;            // 0 — сейчас, 1 — 3 дня
static float wxPageOff = 0;         // плавный сдвиг страниц (пиксели)
static unsigned long wxLastFrame = 0;

#define WX_REFRESH_MS  (15UL * 60UL * 1000UL)
#define WX_RETRY_MS    (30UL * 1000UL)

// WMO weather code -> вид иконки
static int wxKind(int code, bool day) {
  switch (code) {
    case 0: case 1: return day ? WI_SUN : WI_MOON;
    case 2: return day ? WI_PART_D : WI_PART_N;
    case 3: return WI_CLOUDS;
    case 45: case 48: return WI_FOG;
    case 51: case 53: case 55: case 56: case 57: return WI_DRIZZLE;
    case 61: case 63: case 66: case 67: case 80: case 81: return WI_RAIN;
    case 65: case 82: return WI_HEAVY;
    case 71: case 73: case 77: case 85: return WI_SNOW;
    case 75: case 86: return WI_HSNOW;
    case 95: case 96: case 99: return WI_STORM;
  }
  return day ? WI_PART_D : WI_PART_N;
}

static const char* wxDesc(int code) {
  bool r = langRU;
  switch (code) {
    case 0: return r ? "ЯСНО" : "CLEAR";
    case 1: return r ? "ПОЧТИ ЯСНО" : "MOSTLY CLEAR";
    case 2: return r ? "ПЕРЕМ.ОБЛАЧНО" : "PARTLY CLOUDY";
    case 3: return r ? "ОБЛАЧНО" : "CLOUDY";
    case 45: case 48: return r ? "ТУМАН" : "FOG";
    case 51: case 53: case 55: case 56: case 57: return r ? "МОРОСЬ" : "DRIZZLE";
    case 61: case 63: case 80: case 81: return r ? "ДОЖДЬ" : "RAIN";
    case 65: case 82: return r ? "ЛИВЕНЬ" : "HEAVY RAIN";
    case 66: case 67: return r ? "ЛЕДЯНОЙ ДОЖДЬ" : "ICE RAIN";
    case 71: case 73: case 77: case 85: return r ? "СНЕГ" : "SNOW";
    case 75: case 86: return r ? "СНЕГОПАД" : "HEAVY SNOW";
    case 95: case 96: case 99: return r ? "ГРОЗА" : "THUNDERSTORM";
  }
  return r ? "ОБЛАЧНО" : "CLOUDY";
}

static const unsigned char* wxBaseBmp(int kind) {
  switch (kind) {
    case WI_SUN:    return wxBmpSun;
    case WI_MOON:   return wxBmpMoon;
    case WI_PART_D: return wxBmpPartD;
    case WI_PART_N: return wxBmpPartN;
    case WI_CLOUDS: return wxBmpClouds;
    default:        return wxBmpCloud;   // туман, дождь, снег, гроза — облако + анимация
  }
}

// ---------- разбор JSON (без библиотек) ----------
static bool wxNum(const char* from, const char* key, float &v) {
  const char* p = strstr(from, key);
  if (!p) return false;
  p += strlen(key);
  while (*p == ' ') p++;
  v = atof(p);
  return true;
}

static bool wxArr(const char* from, const char* key, float* arr, int n) {
  const char* p = strstr(from, key);   // key заканчивается на '['
  if (!p) return false;
  p += strlen(key);
  for (int i = 0; i < n; i++) {
    while (*p == ' ') p++;
    arr[i] = atof(p);
    while (*p && *p != ',' && *p != ']') p++;
    if (*p == ',') p++;
  }
  return true;
}

static bool wxParse(const String& body, WeatherData &d) {
  const char* js = body.c_str();
  const char* cur = strstr(js, "\"current\":{");
  const char* dly = strstr(js, "\"daily\":{");
  if (!cur || !dly) return false;

  float t, f, h, c, w, day;
  if (!wxNum(cur, "\"temperature_2m\":", t)) return false;
  if (!wxNum(cur, "\"apparent_temperature\":", f)) f = t;
  if (!wxNum(cur, "\"relative_humidity_2m\":", h)) h = 0;
  if (!wxNum(cur, "\"weather_code\":", c)) c = 0;
  if (!wxNum(cur, "\"wind_speed_10m\":", w)) w = 0;
  if (!wxNum(cur, "\"is_day\":", day)) day = 1;

  float dc[3] = {0,0,0}, dx[3] = {0,0,0}, dn[3] = {0,0,0};
  wxArr(dly, "\"weather_code\":[", dc, 3);
  wxArr(dly, "\"temperature_2m_max\":[", dx, 3);
  wxArr(dly, "\"temperature_2m_min\":[", dn, 3);

  d.temp  = (int)lroundf(t);
  d.feels = (int)lroundf(f);
  d.hum   = (int)lroundf(h);
  d.code  = (int)lroundf(c);
  d.wind  = (int)lroundf(w);
  d.isDay = day > 0.5f ? 1 : 0;
  for (int i = 0; i < 3; i++) {
    d.dCode[i] = (int)lroundf(dc[i]);
    d.dMax[i]  = (int)lroundf(dx[i]);
    d.dMin[i]  = (int)lroundf(dn[i]);
  }
  d.valid = true;
  return true;
}

// ---------- сеть ----------
static char wxErr[24] = "";   // причина последней ошибки (показывается на экране)

// Чтение ответа с уже подключённого клиента. Возвращает HTTP-статус (200 = ок) или -1, если ответ не HTTP.
static int wxReadResp(Client &client, const String& path, const char* host, String &out, size_t cap = 4000, const char* ua = "esp32") {
  client.print(String("GET ") + path + " HTTP/1.0\r\nHost: " + host +
               "\r\nUser-Agent: " + String(ua) + "\r\nConnection: close\r\n\r\n");

  String resp;
  resp.reserve(cap > 4000 ? 20000 : 1800);
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < 9000) {
    while (client.available()) {
      resp += (char)client.read();
      if (resp.length() > cap) break;
    }
    if (resp.length() > cap) break;
    delay(5);
  }
  client.stop();

  if (!resp.startsWith("HTTP/1.")) return -1;
  int sp = resp.indexOf(' ');
  int status = (sp > 0) ? resp.substring(sp + 1, sp + 4).toInt() : -1;
  int bodyAt = resp.indexOf("\r\n\r\n");
  if (status == 200 && bodyAt > 0) { resp.remove(0, bodyAt + 4); out = std::move(resp); }
  return status;
}

#define WX_HOST "api.open-meteo.com"

static bool wxHttpGet(const String& path, String &out) {
  wxErr[0] = 0;

  // 0) проверка DNS (часто «TLS CONNECT» на самом деле = нет DNS/интернета)
  IPAddress ip;
  if (!WiFi.hostByName(WX_HOST, ip)) {
    snprintf(wxErr, sizeof(wxErr), "DNS FAIL");
    return false;
  }

  int st = 0;

#if WX_USE_TLS
  // 1) Open-Meteo всё равно редиректит http -> https, поэтому сразу идём по TLS.
  //    Две попытки: при нехватке памяти / нестабильном Wi-Fi вторая часто проходит.
  int tlsErr = 0;
  bool tcpDead = false;
  for (int attempt = 0; attempt < 2; attempt++) {
    WiFiClientSecure sc;
    sc.setInsecure();               // без проверки сертификата (часы могут быть не синхронизированы)
    sc.setHandshakeTimeout(15);
    sc.setTimeout(10);
    if (sc.connect(WX_HOST, 443)) {
      st = wxReadResp(sc, path, WX_HOST, out);
      if (st == 200 && out.length() > 0) return true;
      break;
    }
    char eb[80] = "";
    tlsErr = sc.lastError(eb, sizeof(eb));
    sc.stop();
    // различаем: не открывается сам TCP (сеть/провайдер блокирует IP) или не получился TLS
    {
      WiFiClient t;
      if (!t.connect(ip, 443, 5000)) { tcpDead = true; break; }
      t.stop();
    }
    delay(600);
  }
  if (st == 0) {
    if (tcpDead) snprintf(wxErr, sizeof(wxErr), "TCP %s", ip.toString().c_str());
    else         snprintf(wxErr, sizeof(wxErr), "TLS %d M%dK", tlsErr, (int)(ESP.getMaxAllocHeap() / 1024));
  }
#endif

  // 2) запасной вариант — обычный http на 80 порту (если 443 не открывается совсем — не тратим время)
#if WX_USE_TLS
  if (!tcpDead)
#endif
  {
    WiFiClient c;
    c.setTimeout(6);
    if (c.connect(WX_HOST, 80)) {
      int st2 = wxReadResp(c, path, WX_HOST, out);
      if (st2 == 200 && out.length() > 0) return true;
      if (st2 != 0 && st <= 0) st = st2;
    }
  }

  if (wxErr[0]) return false;     // уже есть причина от TLS
  if (st == 0)       snprintf(wxErr, sizeof(wxErr), "CONNECT FAIL");
  else if (st < 0)   snprintf(wxErr, sizeof(wxErr), "BAD REPLY");
  else               snprintf(wxErr, sizeof(wxErr), "HTTP %d", st);
  return false;
}


// ============================================================
//  Запасной источник: wttr.in (обычный http, порт 80, без ключа и без TLS).
//  Русских погодных API без ключа не существует (Яндекс/Гисметео — только по ключу и через https),
//  поэтому берём wttr.in; если Open-Meteo недоступен — переключаемся автоматически.
// ============================================================
static char wxErr2[24] = "";
static int  wxSrcPref  = 0;     // 0 — Open-Meteo, 1 — wttr.in (запоминаем, какой источник сработал)

// коды WorldWeatherOnline -> коды WMO (их понимают wxKind / wxDesc)
static int wxWwoToWmo(int c) {
  switch (c) {
    case 113: return 0;
    case 116: return 2;
    case 119: case 122: return 3;
    case 143: case 248: case 260: return 45;
    case 176: case 263: case 266: case 293: case 296: return 51;
    case 179: case 227: case 323: case 326: return 71;
    case 182: case 185: case 281: case 284: return 56;
    case 200: case 386: case 389: case 392: case 395: return 95;
    case 230: case 335: case 338: case 371: case 368: return 75;
    case 299: case 302: case 353: return 61;
    case 305: case 308: case 356: return 65;
    case 311: case 314: return 66;
    case 317: case 320: return 67;
    case 329: case 332: return 73;
    case 350: case 374: case 377: return 77;
    case 359: return 82;
    case 362: case 365: return 85;
  }
  return 3;
}

// число после ключа; пропускает пробелы и кавычки ("temp_C": "-3")
static bool wxNumQ(const char* from, const char* key, float &v) {
  const char* p = strstr(from, key);
  if (!p) return false;
  p += strlen(key);
  while (*p == ' ' || *p == '"') p++;
  v = atof(p);
  return true;
}

static bool wxParseWttr(const String& body, WeatherData &d) {
  const char* js  = body.c_str();
  const char* cur = strstr(js, "\"current_condition\"");
  const char* wk  = strstr(js, "\"weather\":");
  if (!cur || !wk) return false;

  float t, f, h = 0, c = 0, w = 0;
  if (!wxNumQ(cur, "\"temp_C\":", t)) return false;
  if (!wxNumQ(cur, "\"FeelsLikeC\":", f)) f = t;
  wxNumQ(cur, "\"humidity\":", h);
  wxNumQ(cur, "\"weatherCode\":", c);
  wxNumQ(cur, "\"windspeedKmph\":", w);

  d.temp  = (int)lroundf(t);
  d.feels = (int)lroundf(f);
  d.hum   = (int)lroundf(h);
  d.code  = wxWwoToWmo((int)lroundf(c));
  d.wind  = (int)lroundf(w / 3.6f);          // км/ч -> м/с
  struct tm ti;
  d.isDay = 1;
  if (getLocalTime(&ti, 0) && ti.tm_year > 120) d.isDay = (ti.tm_hour >= 6 && ti.tm_hour < 21) ? 1 : 0;

  const char* p = wk;
  int days = 0;
  for (int i = 0; i < 3; i++) {
    const char* mx = strstr(p, "\"maxtempC\":");
    if (!mx) break;
    float a = 0, b = 0, cd = c;
    wxNumQ(mx, "\"maxtempC\":", a);
    wxNumQ(mx, "\"mintempC\":", b);
    const char* noon = strstr(mx, "\"1200\"");
    const char* cw = noon ? strstr(noon, "\"weatherCode\":") : nullptr;
    if (cw) wxNumQ(cw, "\"weatherCode\":", cd);
    d.dMax[i]  = (int)lroundf(a);
    d.dMin[i]  = (int)lroundf(b);
    d.dCode[i] = wxWwoToWmo((int)lroundf(cd));
    p = cw ? cw + 1 : mx + 1;
    days++;
  }
  if (days < 1) return false;
  for (int i = days; i < 3; i++) { d.dMax[i] = d.dMax[days-1]; d.dMin[i] = d.dMin[days-1]; d.dCode[i] = d.dCode[days-1]; }
  d.valid = true;
  return true;
}

static bool wxGetWttr(int city, String &out) {
  wxErr[0] = 0;
  IPAddress ip;
  if (!WiFi.hostByName("wttr.in", ip)) { snprintf(wxErr, sizeof(wxErr), "DNS FAIL"); return false; }
  char path[64];
  snprintf(path, sizeof(path), "/%.2f,%.2f?format=j1", WX_LAT[city], WX_LON[city]);
  WiFiClient c;
  c.setTimeout(8);
  if (!c.connect(ip, 80, 8000)) { snprintf(wxErr, sizeof(wxErr), "TCP %s", ip.toString().c_str()); return false; }
  int st = wxReadResp(c, String(path), "wttr.in", out, 40000, "curl/8.0");
  if (st == 200 && out.length() > 0) return true;
  if (st < 0)  snprintf(wxErr, sizeof(wxErr), "BAD REPLY");
  else if (st == 0) snprintf(wxErr, sizeof(wxErr), "NO ANSWER");
  else snprintf(wxErr, sizeof(wxErr), "HTTP %d", st);
  return false;
}

static void wxDoFetch() {
  int city = wxReqCity;
  char path[300];
  snprintf(path, sizeof(path),
    "/v1/forecast?latitude=%.2f&longitude=%.2f"
    "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m,is_day"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min"
    "&forecast_days=3&timezone=auto&wind_speed_unit=ms",
    WX_LAT[city], WX_LON[city]);

  WeatherData nd;
  bool ok = false;
  char errs[2][24] = {"", ""};

  for (int k = 0; k < 2 && !ok; k++) {
    int src = (wxSrcPref + k) & 1;          // сначала тот источник, что работал в прошлый раз
    String body;
    memset(&nd, 0, sizeof(nd));
    nd.city = city;
    bool got = (src == 0) ? wxHttpGet(String(path), body) : wxGetWttr(city, body);
    ok = got && ((src == 0) ? wxParse(body, nd) : wxParseWttr(body, nd));
    if (got && !ok) snprintf(wxErr, sizeof(wxErr), "PARSE FAIL");
    if (ok) wxSrcPref = src;
    else snprintf(errs[k], sizeof(errs[k]), "%s", wxErr);
  }
  snprintf(wxErr,  sizeof(wxErr),  "%s", errs[0]);
  snprintf(wxErr2, sizeof(wxErr2), "%s", errs[1]);

  if (ok) {
    wxNew = nd;
    wxNewReady = true;
    wxStatus = WS_OK;
  } else {
    wxErrAt = millis();
    wxStatus = WS_ERR;
  }
  wxBusy = false;
}

static int wxCurrentCity() {
  return (clockCityIndex >= 0 && clockCityIndex < CLOCK_CITY_COUNT) ? clockCityIndex : 0;
}

static void wxTask(void*) {
  wxDoFetch();
  vTaskDelete(NULL);
}

// Запрос идёт в фоновой задаче, чтобы анимация загрузки не замирала.
static void wxStartFetch() {
  if (wxBusy) return;
  wxReqCity = wxCurrentCity();
  wxBusy = true;
  if (!wxData.valid || wxData.city != wxReqCity) wxStatus = WS_LOADING;


  if (xTaskCreatePinnedToCore(wxTask, "weather", 12288, nullptr, 1, nullptr, 0) != pdPASS) {
    wxBusy = false;
    wxErrAt = millis();
    wxStatus = WS_ERR;
  }
}

void initWeather() {
  wxPage = 0;
  wxPageOff = 0;
  wxCityMenu = false;
  // сменили город в часах — старые данные больше не подходят
  if (wxData.valid && wxData.city != wxCurrentCity()) wxData.valid = false;
  // при каждом входе даём шанс обновиться сразу, а не ждать таймер повтора после старой ошибки
  if (!wxBusy) { wxStatus = WS_IDLE; wxErrAt = 0; }
}

// ---------- рисование ----------
static inline uint16_t wxFg()  { return whiteTheme ? BLACK : WHITE; }
static inline uint16_t wxBg()  { return whiteTheme ? WHITE : BLACK; }

static const int8_t WX_DROP_X[10] = {8, 22, 13, 26, 10, 18, 5, 29, 15, 24};

// иконка 32x24 + анимированные «эффекты» ниже/вокруг неё
static void wxDrawIcon(int kind, int x, int y, uint32_t t) {
  uint16_t fg = wxFg();

  // облака слегка «дышат»
  int bob = 0;
  if (kind != WI_SUN && kind != WI_MOON) bob = ((t / 900) & 1) ? 1 : 0;
  display.drawBitmap(x, y + bob, wxBaseBmp(kind), 32, 24, fg);

  switch (kind) {
    case WI_SUN: {
      float a0 = t * 0.0007f;
      for (int i = 0; i < 8; i++) {
        float a = a0 + i * (PI / 4.0f);
        float r1 = 9.5f, r2 = (i & 1) ? 11.5f : 12.5f;
        int cx = x + 16, cy = y + 12;
        display.drawLine(cx + (int)roundf(cosf(a) * r1), cy + (int)roundf(sinf(a) * r1),
                         cx + (int)roundf(cosf(a) * r2), cy + (int)roundf(sinf(a) * r2), fg);
      }
      break;
    }
    case WI_MOON: {
      static const int8_t sx[4] = {4, 27, 26, 5};
      static const int8_t sy[4] = {4, 6, 19, 19};
      for (int i = 0; i < 4; i++) {
        if (((t / 350) + i * 3) % 5 == 0) continue;   // мерцание
        int px = x + sx[i], py = y + sy[i];
        display.drawPixel(px, py, fg);
        if (i == 1 || i == 3) {
          display.drawPixel(px - 1, py, fg); display.drawPixel(px + 1, py, fg);
          display.drawPixel(px, py - 1, fg); display.drawPixel(px, py + 1, fg);
        }
      }
      break;
    }
    case WI_FOG: {
      for (int r = 0; r < 3; r++) {
        int yy = y + 23 + r * 3;
        int shift = (int)((t / 140) % 10);
        if (r & 1) shift = 9 - shift;
        for (int xx = x + 2 - 10 + shift; xx < x + 30; xx += 10) {
          int a = xx < x + 2 ? x + 2 : xx;
          int b = xx + 6; if (b > x + 30) b = x + 30;
          if (b > a) display.drawFastHLine(a, yy, b - a, fg);
        }
      }
      break;
    }
    case WI_DRIZZLE: case WI_RAIN: case WI_HEAVY: case WI_STORM: {
      int n = (kind == WI_DRIZZLE) ? 4 : (kind == WI_RAIN) ? 6 : (kind == WI_HEAVY) ? 10 : 5;
      uint32_t spd = (kind == WI_HEAVY || kind == WI_STORM) ? 38 : 55;
      for (int i = 0; i < n; i++) {
        int dx = x + WX_DROP_X[i];
        int dy = y + 24 + (int)(((t / spd) + i * 5) % 14);
        if (kind == WI_DRIZZLE) display.drawPixel(dx, dy, fg);
        else display.drawLine(dx, dy, dx - 1, dy + 2, fg);
      }
      if (kind == WI_STORM) {
        int ph = (int)((t / 110) % 14);
        if (ph < 3 || (ph >= 5 && ph < 7)) {
          int bx = x + 17, by = y + 21;
          for (int k = 0; k < 2; k++) {
            display.drawLine(bx + k, by,      bx - 5 + k, by + 7, fg);
            display.drawLine(bx - 5 + k, by + 7, bx - 1 + k, by + 7, fg);
            display.drawLine(bx - 1 + k, by + 7, bx - 5 + k, by + 15, fg);
          }
        }
      }
      break;
    }
    case WI_SNOW: case WI_HSNOW: {
      int n = (kind == WI_SNOW) ? 6 : 10;
      for (int i = 0; i < n; i++) {
        int fy = y + 24 + (int)(((t / 95) + i * 4) % 15);
        int fx = x + WX_DROP_X[i] + (int)roundf(sinf(t / 420.0f + i) * 2.0f);
        if (i % 3 == 0) {
          display.drawPixel(fx, fy, fg);
          display.drawPixel(fx - 1, fy, fg); display.drawPixel(fx + 1, fy, fg);
          display.drawPixel(fx, fy - 1, fg); display.drawPixel(fx, fy + 1, fg);
        } else {
          display.drawPixel(fx, fy, fg);
        }
      }
      break;
    }
  }
}

// маленькая версия иконки 16x16 для прогноза на 3 дня (день, без анимации)
static void wxDrawMini(int kind, int x, int y) {
  uint16_t fg = wxFg();
  if (kind == WI_SUN) {
    display.fillCircle(x + 8, y + 6, 3, fg);
    for (int i = 0; i < 8; i++) {
      float a = i * (PI / 4.0f);
      display.drawPixel(x + 8 + (int)roundf(cosf(a) * 6.0f), y + 6 + (int)roundf(sinf(a) * 6.0f), fg);
    }
    return;
  }
  const unsigned char* bmp = wxBaseBmp(kind);
  for (int yy = 0; yy < 12; yy++) {
    for (int xx = 0; xx < 16; xx++) {
      int cnt = 0;
      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          int sx = xx * 2 + dx, sy = yy * 2 + dy;
          uint8_t b = pgm_read_byte(&bmp[sy * 4 + (sx >> 3)]);
          if (b & (0x80 >> (sx & 7))) cnt++;
        }
      }
      if (cnt >= 2) display.drawPixel(x + xx, y + yy, fg);
    }
  }
  // осадки под облаком
  switch (kind) {
    case WI_FOG:
      display.drawFastHLine(x + 2, y + 12, 12, fg);
      display.drawFastHLine(x + 4, y + 14, 10, fg);
      break;
    case WI_DRIZZLE:
      display.drawPixel(x + 5, y + 13, fg); display.drawPixel(x + 9, y + 14, fg); display.drawPixel(x + 12, y + 13, fg);
      break;
    case WI_RAIN: case WI_HEAVY:
      for (int i = 0; i < 3; i++) display.drawLine(x + 4 + i * 4, y + 12, x + 3 + i * 4, y + 14, fg);
      if (kind == WI_HEAVY) display.drawLine(x + 6, y + 14, x + 5, y + 15, fg);
      break;
    case WI_SNOW: case WI_HSNOW:
      display.drawPixel(x + 4, y + 12, fg); display.drawPixel(x + 8, y + 14, fg);
      display.drawPixel(x + 12, y + 12, fg); display.drawPixel(x + 6, y + 15, fg);
      break;
    case WI_STORM:
      display.drawLine(x + 9, y + 11, x + 6, y + 14, fg);
      display.drawLine(x + 6, y + 14, x + 9, y + 14, fg);
      display.drawLine(x + 9, y + 14, x + 6, y + 15, fg);
      break;
  }
}

// число + кружок градуса + C. size — размер шрифта
static void wxPrintTemp(int v, int size, int x, int y) {
  uint16_t fg = wxFg();
  display.setTextSize(size);
  display.setTextColor(fg);
  display.setCursor(x, y);
  display.print(v);
  int cx = display.getCursorX();
  display.drawCircle(cx + 1 + size, y + size, size > 1 ? 2 : 1, fg);
  display.setCursor(cx + 3 + size * 2, y);
  display.print("C");
}

static void wxDropIcon(int x, int y) {          // капля 5x7
  uint16_t fg = wxFg();
  display.drawPixel(x + 2, y, fg);
  display.drawFastHLine(x + 1, y + 1, 3, fg);
  display.drawFastHLine(x + 1, y + 2, 3, fg);
  display.drawFastHLine(x,     y + 3, 5, fg);
  display.drawFastHLine(x,     y + 4, 5, fg);
  display.drawFastHLine(x,     y + 5, 5, fg);
  display.drawFastHLine(x + 1, y + 6, 3, fg);
}

static void wxWindIcon(int x, int y) {          // ветер 8x7
  uint16_t fg = wxFg();
  display.drawFastHLine(x,     y + 1, 6, fg);
  display.drawPixel(x + 6, y,     fg);
  display.drawPixel(x + 7, y + 1, fg);
  display.drawPixel(x + 6, y + 2, fg);
  display.drawFastHLine(x + 1, y + 3, 7, fg);
  display.drawFastHLine(x,     y + 5, 5, fg);
  display.drawPixel(x + 5, y + 6, fg);
  display.drawPixel(x + 6, y + 5, fg);
}

static void wxArrow(int x, int y, bool up) {    // стрелочка 5x3
  uint16_t fg = wxFg();
  if (up) {
    display.drawPixel(x + 2, y, fg);
    display.drawFastHLine(x + 1, y + 1, 3, fg);
    display.drawFastHLine(x, y + 2, 5, fg);
  } else {
    display.drawFastHLine(x, y, 5, fg);
    display.drawFastHLine(x + 1, y + 1, 3, fg);
    display.drawPixel(x + 2, y + 2, fg);
  }
}

static void wxCenterText(const char* s, int cx, int y) {
  int w = (int)trLen(trs(s)) * 6;
  display.setCursor(cx - w / 2, y);
  display.print(s);
}

// страница «Сейчас»: xo — горизонтальный сдвиг (для анимации листания)
static void wxDrawNow(int xo, uint32_t t) {
  uint16_t fg = wxFg();
  const WeatherData &d = wxData;
  bool day = d.isDay != 0;
  int kind = wxKind(d.code, day);

  // рамка с иконкой
  display.drawRoundRect(xo + 1, 13, 44, 50, 4, fg);
  wxDrawIcon(kind, xo + 7, 19, t);

  // температура
  wxPrintTemp(d.temp, 2, xo + 50, 15);

  // описание
  display.setTextSize(1);
  display.setTextColor(fg);
  display.setCursor(xo + 50, 34);
  display.print(wxDesc(d.code));

  // ощущается как
  display.setCursor(xo + 50, 44);
  display.print("FEELS ");
  display.print(d.feels);
  {
    int ex = display.getCursorX();
    display.drawCircle(ex + 2, 45, 1, fg);
    display.setCursor(ex + 5, 44);
    display.print("C");
  }

  // влажность и ветер
  wxDropIcon(xo + 50, 54);
  display.setCursor(xo + 57, 54);
  display.print(d.hum);
  display.print("%");
  wxWindIcon(xo + 86, 54);
  display.setCursor(xo + 96, 54);
  display.print(d.wind);
  display.print(langRU ? "м/с" : "m/s");
}

// страница «3 дня»
static void wxDrawForecast(int xo, uint32_t t) {
  (void)t;
  uint16_t fg = wxFg();
  uint16_t bg = wxBg();
  static const char* const DAYS_EN[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* const DAYS_RU[7] = {"ВС", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"};

  struct tm ti;
  bool haveTime = getLocalTime(&ti, 0) && ti.tm_year > 120;

  for (int i = 0; i < 3; i++) {
    int x0 = xo + 1 + i * 42;
    int cx = x0 + 20;
    if (i > 0) display.drawFastVLine(x0 - 1, 14, 49, fg);

    // название дня: сегодняшний — в инверсной плашке
    char lbl[12];   // кириллица в UTF-8: 2 байта на букву, нужен запас
    if (haveTime) {
      int wd = (ti.tm_wday + i) % 7;
      snprintf(lbl, sizeof(lbl), "%s", langRU ? DAYS_RU[wd] : DAYS_EN[wd]);
    } else {
      snprintf(lbl, sizeof(lbl), "D%d", i + 1);
    }
    int lw = (int)trLen(lbl) * 6;
    if (i == 0) {
      display.fillRect(cx - lw / 2 - 2, 13, lw + 4, 10, fg);
      display.setTextColor(bg);
    } else {
      display.setTextColor(fg);
    }
    display.setTextSize(1);
    display.setCursor(cx - lw / 2, 14);
    display.print(lbl);
    display.setTextColor(fg);

    // иконка
    wxDrawMini(wxKind(wxData.dCode[i], true), cx - 8, 26);

    // максимум / минимум
    char b[8];
    snprintf(b, sizeof(b), "%d", wxData.dMax[i]);
    int w1 = (int)strlen(b) * 6;
    wxArrow(cx - (w1 + 8) / 2, 45, true);
    display.setCursor(cx - (w1 + 8) / 2 + 8, 43);
    display.print(b);

    snprintf(b, sizeof(b), "%d", wxData.dMin[i]);
    int w2 = (int)strlen(b) * 6;
    wxArrow(cx - (w2 + 8) / 2, 55, false);
    display.setCursor(cx - (w2 + 8) / 2 + 8, 53);
    display.print(b);
  }
}

static void wxDrawSpinner(int cx, int cy, uint32_t t) {
  uint16_t fg = wxFg();
  int head = (int)((t / 90) % 8);
  for (int i = 0; i < 8; i++) {
    float a = i * (PI / 4.0f) - PI / 2.0f;
    int px = cx + (int)roundf(cosf(a) * 9.0f);
    int py = cy + (int)roundf(sinf(a) * 9.0f);
    int age = (head - i + 8) % 8;
    if (age == 0)      display.fillCircle(px, py, 2, fg);
    else if (age < 4)  display.fillCircle(px, py, 1, fg);
    else               display.drawPixel(px, py, fg);
  }
}

static void wxDrawHeader(uint32_t t) {
  uint16_t fg = wxFg();
  uint16_t bg = wxBg();
  display.fillRect(0, 0, 128, 11, fg);
  display.setTextSize(1);
  display.setTextColor(bg);
  display.setCursor(2, 2);
  display.print(CLOCK_CITY_NAMES[wxCurrentCity()]);

  // локальное время выбранного города (последнее удачное значение запоминаем — не пропадает на кадр)
  static char tb[8] = "";
  struct tm ti;
  if (getLocalTime(&ti, 0) && ti.tm_year > 120) snprintf(tb, sizeof(tb), "%02d:%02d", ti.tm_hour, ti.tm_min);
  if (tb[0]) {
    display.setCursor(77, 2);
    display.print(tb);
  }

  // точки-индикаторы страниц
  for (int p = 0; p < 2; p++) {
    int px = 113 + p * 7;
    if (p == wxPage) display.fillRect(px, 4, 4, 4, bg);
    else             display.drawRect(px, 4, 4, 4, bg);
  }

  // мигающая точка — идёт обновление данных
  if (wxBusy && wxData.valid && ((t / 300) & 1)) display.fillRect(109, 4, 2, 2, bg);
  display.drawFastHLine(0, 11, 128, fg);
  display.setTextColor(fg);
}

// выбор города (кнопка ВВЕРХ): тот же список и то же сохранение, что и в часах
static void wxRunCityMenu(uint32_t t) {
  if (t - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_UP) == LOW) {
      wxMenuIdx = (wxMenuIdx - 1 + CLOCK_CITY_COUNT) % CLOCK_CITY_COUNT;
      lastButtonPress = t;
    } else if (digitalRead(BTN_DOWN) == LOW) {
      wxMenuIdx = (wxMenuIdx + 1) % CLOCK_CITY_COUNT;
      lastButtonPress = t;
    } else if (digitalRead(BTN_SELECT) == LOW) {
      if (wxMenuIdx != clockCityIndex) {
        saveClockCity(wxMenuIdx);            // сохраняет город и применяет часовой пояс
        wxPage = 0;
        wxPageOff = 0;
        wxErrAt = 0;
        wxStatus = WS_IDLE;
      }
      wxCityMenu = false;
      lastButtonPress = t;
    } else if (digitalRead(BTN_BACK) == LOW) {
      wxCityMenu = false;
      lastButtonPress = t;
    }
  }

  if (t - wxLastFrame < 40) return;
  wxLastFrame = t;

  uint16_t fg = wxFg();
  uint16_t bg = wxBg();
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  display.setTextWrap(false);
  display.fillRect(0, 0, 128, 11, fg);
  display.setTextSize(1);
  display.setTextColor(bg);
  display.setCursor(2, 2);
  display.print("SELECT CITY");
  display.setTextColor(fg);

  int first = wxMenuIdx - 2;
  if (first < 0) first = 0;
  if (first > CLOCK_CITY_COUNT - 5) first = CLOCK_CITY_COUNT - 5;
  for (int row = 0; row < 5; row++) {
    int idx = first + row;
    if (idx >= CLOCK_CITY_COUNT) break;
    display.setCursor(2, 14 + row * 10);
    display.print(idx == wxMenuIdx ? "> " : "  ");
    display.print(CLOCK_CITY_NAMES[idx]);
    if (idx == wxCurrentCity()) {            // отметка текущего города
      display.setCursor(120, 14 + row * 10);
      display.print("*");
    }
  }
  display.display();
  display.setTextWrap(true);
}

void runWeather() {
  uint32_t t = millis();

  if (wxCityMenu) { wxRunCityMenu(t); return; }

  // результат фоновой загрузки забираем здесь, в основном потоке (без гонки с отрисовкой)
  bool busy = wxBusy;
  if (!busy && wxNewReady) {
    wxData = wxNew;
    wxFetchedAt = millis();
    wxNewReady = false;
  }

  // ---- управление ----
  if (t - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_LEFT) == LOW && wxData.valid) {
      if (wxPage > 0) { wxPage--; lastButtonPress = t; }
    } else if (digitalRead(BTN_RIGHT) == LOW && wxData.valid) {
      if (wxPage < 1) { wxPage++; lastButtonPress = t; }
    } else if (digitalRead(BTN_UP) == LOW) {
      wxCityMenu = true;
      wxMenuIdx = wxCurrentCity();
      lastButtonPress = t;
      return;
    } else if (digitalRead(BTN_SELECT) == LOW) {
      if (WiFi.status() == WL_CONNECTED && !busy) wxStartFetch();
      lastButtonPress = t;
    } else if (digitalRead(BTN_BACK) == LOW) {
      changeState(APPS_MENU);
      lastButtonPress = t;
      return;
    }
  }

  // ---- автообновление ----
  bool online = (WiFi.status() == WL_CONNECTED);
  if (online && !wxBusy) {
    int city = wxCurrentCity();
    bool needFetch = false;
    bool retryOk = (wxStatus != WS_ERR || t - wxErrAt > WX_RETRY_MS);   // после ошибки — не чаще раза в 30 с
    if (!wxData.valid || wxData.city != city) {
      needFetch = retryOk;
    } else if (t - wxFetchedAt > WX_REFRESH_MS) {
      needFetch = retryOk;
    }
    if (needFetch) wxStartFetch();
  }

  // ~25 кадров/с
  if (t - wxLastFrame < 40) return;
  wxLastFrame = t;

  // плавное листание страниц
  float target = -128.0f * wxPage;
  wxPageOff += (target - wxPageOff) * 0.38f;
  if (fabsf(target - wxPageOff) < 0.6f) wxPageOff = target;

  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  display.setTextWrap(false);   // иначе текст за правым краем «заворачивается» на левый край экрана

  bool haveData = wxData.valid && wxData.city == wxCurrentCity();

  if (haveData) {
    int xo = (int)roundf(wxPageOff);
    if (xo > -128) wxDrawNow(xo, t);
    if (xo < 0)    wxDrawForecast(xo + 128, t);
    wxDrawHeader(t);
  } else {
    wxDrawHeader(t);
    uint16_t fg = wxFg();
    display.setTextSize(1);
    display.setTextColor(fg);
    if (!online) {
      display.setCursor(7, 24); display.print("WiFi: OFFLINE");
      display.setCursor(7, 38); display.print("CONNECT WiFi");
      display.setCursor(7, 52); display.print("BACK: EXIT");
    } else if (wxStatus == WS_ERR && !wxBusy) {
      wxCenterText("NO DATA", 64, 15);
      if (wxErr[0])  wxCenterText(wxErr, 64, 26);
      if (wxErr2[0]) wxCenterText(wxErr2, 64, 35);
      wxCenterText("SELECT: RETRY", 64, 45);
      wxCenterText("BACK:   EXIT", 64, 55);
    } else {
      wxDrawSpinner(64, 31, t);
      wxCenterText("LOADING...", 64, 50);
    }
  }
  display.display();
  display.setTextWrap(true);
}

#endif
