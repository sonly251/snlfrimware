#ifndef _snl_lang_h
#define _snl_lang_h
// ============================================================
//  ЯЗЫК ИНТЕРФЕЙСА (EN / RU)
//
//  Как это работает:
//   * display теперь объект LangDisplay (наследник Adafruit_SSD1306).
//     Он перехватывает ЛЮБОЙ display.print()/println():
//       1) в режиме RU ищет строку в словаре TR_TABLE (английский -> русский)
//          и печатает перевод вместо оригинала;
//       2) всегда понимает UTF-8 кириллицу (А-Яа-яЁё) и рисует её шрифтом 5x7
//          (тот же, что в просмотре текстовых файлов в проводнике).
//     Поэтому весь существующий код (display.print("SCORE:") и т.д.)
//     менять не нужно: достаточно добавить пару в словарь ниже.
//   * Словарь — обычный UTF-8, пишите по-русски прямо в исходнике.
//   * Ключ должен совпадать с английским литералом в коде ПОБУКВЕННО
//     (включая пробелы и двоеточия). Если строки в словаре нет — печатается
//     оригинал, ничего не ломается.
//   * Русский текст должен помещаться на экран: 21 символ в строке при
//     размере шрифта 1 (6 px на символ), 10 символов при размере 2.
//   * trs("TEXT")  — вернуть перевод (для самостоятельной отрисовки);
//     trLen(str)   — длина строки в СИМВОЛАХ (а не байтах UTF-8).
// ============================================================

// Текущий язык: false = английский, true = русский (хранится в EEPROM, EE_LANG)
bool langRU = false;

// ---------- Кириллица 5x7 (бит 0 байта-столбца = верхняя строка глифа) ----------
static const uint8_t FM_CYR_UP[32][5] PROGMEM = {
  {0x7E,0x09,0x09,0x09,0x7E},  // А
  {0x7F,0x49,0x49,0x49,0x31},  // Б
  {0x7F,0x49,0x49,0x49,0x36},  // В
  {0x7F,0x01,0x01,0x01,0x01},  // Г
  {0x60,0x3E,0x21,0x3F,0x60},  // Д
  {0x7F,0x49,0x49,0x49,0x41},  // Е
  {0x63,0x14,0x7F,0x14,0x63},  // Ж
  {0x22,0x41,0x49,0x49,0x36},  // З
  {0x7F,0x10,0x08,0x04,0x7F},  // И
  {0x7E,0x21,0x12,0x09,0x7E},  // Й 
  {0x7F,0x08,0x14,0x22,0x41},  // К
  {0x60,0x1E,0x01,0x01,0x7F},  // Л
  {0x7F,0x02,0x0C,0x02,0x7F},  // М
  {0x7F,0x08,0x08,0x08,0x7F},  // Н
  {0x3E,0x41,0x41,0x41,0x3E},  // О
  {0x7F,0x01,0x01,0x01,0x7F},  // П
  {0x7F,0x09,0x09,0x09,0x06},  // Р
  {0x3E,0x41,0x41,0x41,0x22},  // С
  {0x01,0x01,0x7F,0x01,0x01},  // Т
  {0x27,0x48,0x48,0x48,0x3F},  // У
  {0x1C,0x22,0x7F,0x22,0x1C},  // Ф
  {0x63,0x14,0x08,0x14,0x63},  // Х
  {0x3F,0x20,0x20,0x3F,0x60},  // Ц
  {0x07,0x08,0x08,0x08,0x7F},  // Ч
  {0x7F,0x40,0x7F,0x40,0x7F},  // Ш
  {0x3F,0x20,0x3F,0x20,0x7F},  // Щ
  {0x01,0x7F,0x48,0x48,0x30},  // Ъ
  {0x7F,0x48,0x78,0x00,0x7F},  // Ы
  {0x7F,0x48,0x48,0x48,0x30},  // Ь
  {0x41,0x41,0x49,0x49,0x3E},  // Э
  {0x7F,0x08,0x3E,0x41,0x3E},  // Ю
  {0x46,0x29,0x19,0x09,0x7F},  // Я
};
static const uint8_t FM_CYR_LO[32][5] PROGMEM = {
  {0x20,0x54,0x54,0x54,0x78},  // а
  {0x3E,0x49,0x49,0x49,0x31},  // б
  {0x7C,0x54,0x54,0x54,0x28},  // в
  {0x7C,0x04,0x04,0x04,0x04},  // г
  {0xC0,0x78,0x44,0x7C,0xC0},  // д
  {0x38,0x54,0x54,0x54,0x18},  // е
  {0x6C,0x10,0x7C,0x10,0x6C},  // ж
  {0x44,0x54,0x54,0x54,0x28},  // з
  {0x7C,0x20,0x10,0x08,0x7C},  // и
  {0x7C,0x21,0x12,0x09,0x7C},  // й
  {0x7C,0x10,0x10,0x28,0x44},  // к
  {0x40,0x38,0x04,0x04,0x7C},  // л
  {0x7C,0x08,0x10,0x08,0x7C},  // м
  {0x7C,0x10,0x10,0x10,0x7C},  // н
  {0x38,0x44,0x44,0x44,0x38},  // о
  {0x7C,0x04,0x04,0x04,0x7C},  // п
  {0xFC,0x44,0x44,0x44,0x38},  // р
  {0x38,0x44,0x44,0x44,0x44},  // с
  {0x04,0x04,0x7C,0x04,0x04},  // т
  {0x1C,0xA0,0xA0,0xA0,0x7C},  // у
  {0x38,0x44,0xFF,0x44,0x38},  // ф
  {0x44,0x28,0x10,0x28,0x44},  // х
  {0x7C,0x40,0x40,0x7C,0xC0},  // ц
  {0x0C,0x10,0x10,0x10,0x7C},  // ч
  {0x7C,0x40,0x7C,0x40,0x7C},  // ш
  {0x7C,0x40,0x7C,0x40,0xFC},  // щ
  {0x04,0x7C,0x50,0x50,0x20},  // ъ
  {0x7C,0x50,0x70,0x00,0x7C},  // ы
  {0x7C,0x50,0x50,0x50,0x20},  // ь
  {0x44,0x44,0x54,0x54,0x38},  // э
  {0x7C,0x10,0x38,0x44,0x38},  // ю
  {0x48,0x34,0x14,0x14,0x7C},  // я
};
static const uint8_t FM_CYR_YO_UP[5] PROGMEM = {0x7E,0x4B,0x4A,0x4B,0x42};   // Ё
static const uint8_t FM_CYR_YO_LO[5] PROGMEM = {0x38,0x55,0x54,0x55,0x18};   // ё

// ---------- Словарь переводов (английский -> русский) ----------
struct TrEntry { const char* en; const char* ru; };
static const TrEntry TR_TABLE[] = {
  // --- главное меню ---
  {"< GAMES >", "< ИГРЫ >"}, {"< SETTINGS >", "< НАСТРОЙКИ >"},
  {"< APPS >", "< ПРИЛОЖЕНИЯ >"}, {"< PET >", "< ПИТОМЕЦ >"},
  {"< UTILITIES >", "< УТИЛИТЫ >"}, {"< ENTERTAINMENT >", "< РАЗВЛЕЧЕНИЯ >"},
  {"< INTERFACE >", "< ИНТЕРФЕЙС >"}, {"< WIFI >", "< ВАЙ-ФАЙ >"},
  {"UTILITIES", "УТИЛИТЫ"}, {"ENTERTAINMENT", "РАЗВЛЕЧЕНИЯ"}, {"INTERFACE", "ИНТЕРФЕЙС"},

  // --- меню приложений ---
  {"< STOPWATCH >", "< СЕКУНДОМЕР >"}, {" < FLASHLIGHT > ", " < ФОНАРИК > "},
  {"< CALCULATOR >", "< КАЛЬКУЛЯТОР >"}, {"< RANDOM GENERATOR >", "< ГЕНЕРАТОР ЧИСЕЛ >"},
  {"< COMMAND LINE >", "< КОМАНДНАЯ СТРОКА >"}, {"< SOUNDPAD >", "< САУНДПАД >"},
  {"< MP3 PLAYER >", "< MP3 ПЛЕЕР >"}, {"< 3D RENDER >", "< 3D РЕНДЕР >"},
  {"< CLOCK >", "< ЧАСЫ >"}, {"< JOYSTICK >", "< ДЖОЙСТИК >"},
  {"< WEATHER >", "< ПОГОДА >"},
  {"< FILES >", "< ФАЙЛЫ >"}, {"< PAINT >", "< РИСОВАЛКА >"},

  // --- меню игр ---
  {"< SNAKE >", "< ЗМЕЙКА >"}, {"< PONG >", "< ПОНГ >"}, {"< RACER >", "< ГОНКИ >"},
  {"< DINO >", "< ДИНО >"}, {"< SHOOTER >", "< СТРЕЛЯЛКА >"},
  {"< FLAPPY BIRD >", "< ФЛЭППИ БЁРД >"}, {"< ARKANOID >", "< АРКАНОИД >"},
  {"< TANKS >", "< ТАНКИ >"}, {"< TETRIS >", "< ТЕТРИС >"}, {"< DOOM >", "< ДУМ >"},
  {"< FREERIDE >", "< ФРИРАЙД >"}, {"< PACMAN >", "< ПАКМАН >"}, {"< TOWER >", "< БАШНЯ >"},

  // --- меню настроек ---
  {"< RECORDS >", "< РЕКОРДЫ >"}, {"< THEME >", "< ТЕМА >"}, {"< CODE >", "< КОД >"},
  {"< ABOUT >", "< О ПРОШИВКЕ >"}, {"< BOOT MUSIC >", "< МУЗЫКА ЗАГРУЗКИ >"},
  {"< BOOT SPEED >", "< ВРЕМЯ ЗАГРУЗКИ >"}, {"< BOOT LOGO >", "< ЛОГО ЗАГРУЗКИ >"},
  {"< BRIGHTNESS >", "< ЯРКОСТЬ >"}, {"< WEB INTERFACE >", "< ВЕБ-ИНТЕРФЕЙС >"},
  {"< LANGUAGE: EN >", "< ЯЗЫК: ENGLISH >"}, {"< LANGUAGE: RU >", "< ЯЗЫК: РУССКИЙ >"},

  // --- питомец ---
  {"HUNGER:", "ГОЛОД:"}, {"MOOD:", "НАСТР:"}, {"HAPPY", "СЧАСТЛИВЫЙ"}, {"SAD", "ГРУСТНЫЙ"},
  {"FINE", "ХОРОШО"}, {"LEVEL:", "УРОВЕНЬ:"}, {"MAX", "МАКС"},
  {"...HAPPY!", "...УРА!"}, {"...FEED ME!", "...ПОКОРМИ!"}, {" ...HELLO!", " ...ПРИВЕТ!"},
  {"CAUGHT", "ПОЙМАЛ"}, {"CATCH HEARTS!", "ЛОВИ СЕРДЦА!"}, {"PET: ", "ПИТОМЕЦ: "},

  // --- экран GAME OVER (заголовок печатается размером 2: максимум 10 символов) ---
  {"GAME OVER", "КОНЕЦ ИГРЫ"}, {"Game: ", "Игра: "}, {"Score: ", "Счёт: "},
  {"BACK: Skip", "НАЗАД: Пропуск"},

  // --- названия игр (для экрана GAME OVER) ---
  {"SNAKE", "ЗМЕЙКА"}, {"PONG", "ПОНГ"}, {"RACER", "ГОНКИ"}, {"DINO", "ДИНО"},
  {"SHOOTER", "СТРЕЛЯЛКА"}, {"FLAPPY", "ФЛЭППИ"}, {"ARKANOID", "АРКАНОИД"},
  {"TANKS", "ТАНКИ"}, {"TETRIS", "ТЕТРИС"}, {"DOOM", "ДУМ"}, {"FREERIDE", "ФРИРАЙД"}, {"PACMAN", "ПАКМАН"}, {"TOWER", "БАШНЯ"},

  // --- тема / рекорды ---
  {"THEME", "ТЕМА"}, {"MODE: ", "РЕЖИМ: "}, {"WHITE", "СВЕТЛАЯ"}, {"DARK", "ТЁМНАЯ"},
  {"RECORDS", "РЕКОРДЫ"},
  {"SNAKE:    ", "ЗМЕЙКА:   "}, {"PONG:     ", "ПОНГ:     "}, {"RACER:    ", "ГОНКИ:    "},
  {"DINO:     ", "ДИНО:     "}, {"SHOOTER:  ", "СТРЕЛЯЛКА:"}, {"FLAPPY:   ", "ФЛЭППИ:   "},
  {"ARKANOID: ", "АРКАНОИД: "}, {"TANKS:    ", "ТАНКИ:    "}, {"TETRIS:   ", "ТЕТРИС:   "},
  {"DOOM ", "ДУМ"}  ,     {"FREERIDE: ", "ФРИРАЙД:  "}, {"PACMAN:   ", "ПАКМАН:   "}, {"TOWER:    ", "БАШНЯ:    "},
  {"BEST ", "РЕК "}, {"PERFECT!", "ИДЕАЛЬНО!"},

  // --- джойстик ---
  {"JOYSTICK", "ДЖОЙСТИК"}, {"CONNECTED", "ПОДКЛЮЧЕН"}, {"WAITING", "ОЖИДАНИЕ"},
  {"BACK", "НАЗ"},

  // --- генератор случайных чисел ---
  {"RANDOM GENERATOR", "ГЕНЕРАТОР ЧИСЕЛ"}, {"EDITING MIN VALUE", "ПРАВКА МИНИМУМА"},
  {"EDITING MAX VALUE", "ПРАВКА МАКСИМУМА"}, {"MIN: ", "МИН: "}, {"MAX: ", "МАКС: "},
  {"RESULT: ", "РЕЗУЛЬТАТ: "},
  {"SEL:GEN  LEFT:SWITCH  UP/DN:CHANGE", "ОК:ГЕН  ЛЕВО:СМЕНА"},

  // --- калькулятор / секундомер / о прошивке ---
  {"CALCULATOR", "КАЛЬКУЛЯТОР"}, {"RES: ", "РЕЗ: "},
  {"SEL:START/STOP", "ОК:СТАРТ/СТОП"}, {"UP:RESET", "ВВЕРХ:СБРОС"},
  {"Created: 13.04.26", "Создано: 13.04.26"}, {"by Sonly251", "автор Sonly251"},
  {"snl frimware v8.0", "прошивка snl v8.0"},

  // --- игры: строка состояния ---
  {"SCORE:", "СЧЁТ:"}, {"SCORE: ", "СЧЁТ: "}, {"LIVES:", "ЖИЗНИ:"}, {"WAVE:", "ВОЛ:"},
  {"WAVE", "ВОЛНА"}, {"GET READY!", "К БОЮ!"}, {"Lv:", "Ур:"}, {"NEXT:", "СЛЕД:"},
  {"LEVEL OK!", "ПРОЙДЕНО!"}, {"Next level...", "След. уровень..."},
  {"ARKANOID: LEVEL", "АРКАНОИД: УРОВЕНЬ"}, {"NO SD: CLASSIC ONLY", "НЕТ SD: КЛАССИКА"},
  {"LEVELS ON SD: ", "УРОВНЕЙ НА SD: "}, {"LOAD ERROR", "ОШИБКА ЗАГРУЗКИ"},
  {"BACKFLIP", "БЭКФЛИП"}, {"FRONTFLIP", "ФРОНТФЛИП"},
  {"READY!", "ГОТОВ?"}, {"LEVEL ", "УРОВЕНЬ "},

  // --- пауза ---
  {"PAUSED", "ПАУЗА."}, {"SEL:RESUME", "ОК:ДАЛЬШЕ"}, {"BACK:EXIT", "НАЗ:ВЫХОД"},

  // --- командная строка ---
  {"COMMAND RESULT", "РЕЗУЛЬТАТ КОМАНДЫ"}, {"UP/DN scroll  BACK", "ВВЕРХ/ВНИЗ  НАЗАД"},
  {"BACK: Return", "НАЗАД: Вернуться"},

  // --- веб-пульт ---
  {"WEB REMOTE", "ВЕБ-ПУЛЬТ"}, {"ON", "ВКЛ"}, {"OFF", "ВЫКЛ"}, {"Pass: ", "Пароль: "},
  {"Open in browser:", "Открой в браузере:"}, {"Remote is stopped.", "Пульт остановлен."},
  {"Screen + buttons in", "Экран и кнопки в"}, {"your browser.", "твоём браузере."},
  {"SELECT:stop BACK:exit", "ОК:стоп НАЗАД:выход"}, {"SELECT:run BACK:exit", "ОК:старт НАЗАД:выход"},

  // --- звуки ---
  {"SOUNDPAD", "ЗВУКИ"}, {"COIN", "МОНЕТА"}, {"BEEP", "ПИСК"}, {"LASER", "ЛАЗЕР"},
  {"EXPLODE", "ВЗРЫВ"}, {"POWERUP", "БОНУС"}, {"ERROR", "ОШИБКА"}, {"BOING", "ПРУЖИНА"},
  {"VICTORY", "ПОБЕДА"},

  // --- время загрузки / яркость ---
  {"BOOT SPEED", "ВРЕМЯ ЗАГРУЗКИ"}, {"boot time", "время загрузки"},
  {"UP/DOWN: CHANGE", "ВВЕРХ/ВНИЗ: ИЗМЕН."}, {"SEL:SAVE BACK:CANCEL", "ОК:СОХР НАЗ:ОТМЕНА"},
  {"BRIGHTNESS", "ЯРКОСТЬ"},

  // --- MP3 ---
  {"DFPLAYER ERROR!", "ОШИБКА DFPLAYER!"}, {"MP3 PLAYER", "MP3 ПЛЕЕР"}, {"TRACK: ", "ТРЕК: "},
  {"VOLUME: ", "ГРОМКОСТЬ: "}, {"PLAYING", "ИГРАЕТ"}, {"UP/DN:VOL", "ВВЕРХ/ВНИЗ:ГРОМК"},
  {"LEFT/RIGHT:TRACK", "ВЛЕВО/ВПРАВО:ТРЕК"},

  // --- 3D ---
  {"CUBE", "КУБ"}, {"PYRAMID", "ПИРАМИДА"}, {"RECT", "ПРЯМОУГОЛЬНИК"}, {"SPD:", "СКР:"},
  {"L/R:SHAPE UP/DN:SPEED", "L/R:ФОРМА UP/DN:СКОР."},

  // --- часы / Wi-Fi ---
  {"SELECT CITY", "ВЫБОР ГОРОДА"}, {"WiFi: OFFLINE", "WiFi: НЕТ СЕТИ"},
  {"CONNECT WiFi", "ПОДКЛЮЧИ WiFi"}, {"UP: CITY  BACK: EXIT", "ВВЕРХ:ГОРОД НАЗ:ВЫХ"},
  {"SYNC...", "СИНХР..."}, {"Connecting", "Подключение"}, {"SELECT WIFI", "ВЫБОР СЕТИ"},
  {"NO NETWORKS", "НЕТ СЕТЕЙ"}, {"SAVED NETWORKS", "СОХРАНЁННЫЕ СЕТИ"},
  {"NO SAVED WIFI", "НЕТ СОХРАНЁННЫХ"}, {"PASS: ", "ПАРОЛЬ: "},
  {"JOIN SAVED WIFI", "К СОХРАНЁННЫМ"}, {"ADD NETWORK", "ДОБАВИТЬ СЕТЬ"},
  {"DISABLE WIFI", "ВЫКЛЮЧИТЬ WIFI"}, {"WiFi disabled.", "WiFi выключен."},
  {"Connected", "Подключено"}, {"Connection failed.", "Ошибка подключения."},
  {"Check password.", "Проверь пароль."},

  // --- погода ---
  {"LOADING...", "ЗАГРУЗКА..."}, {"NO DATA", "НЕТ ДАННЫХ"}, {"FEELS ", "ОЩУЩ. "},
  {"NOVOSIBIRSK", "НОВОСИБИРСК"}, {"MOSCOW", "МОСКВА"}, {"LONDON", "ЛОНДОН"},
  {"NEW YORK", "НЬЮ-ЙОРК"}, {"TOKYO", "ТОКИО"}, {"DUBAI", "ДУБАЙ"},
  {"BEIJING", "ПЕКИН"}, {"LOS ANGELES", "ЛОС-АНДЖЕЛЕС"},

  // --- boot-логотип ---
  {"READING SD...", "ЧТЕНИЕ SD..."}, {"NOW: ", "СЕЙЧАС: "}, {"NO SD CARD", "НЕТ SD-КАРТЫ"},
  {"BACK: EXIT", "НАЗАД: ВЫХОД"}, {"NO PICTURES IN", "НЕТ КАРТИНОК В"},
  {"DRAW SOME IN PAINT", "НАРИСУЙ В РИСОВАНИИ"}, {"SEL:SET  RIGHT:VIEW", "ОК:ВЫБОР  ПРАВО:ВИД"},
  {"READ ERROR", "ОШИБКА ЧТЕНИЯ"}, {"USE DEFAULT LOGO?", "СТАНДАРТНЫЙ ЛОГО?"},
  {"SELECT: YES", "ВЫБОР: ДА"}, {"BACK:   NO", "НАЗАД:  НЕТ"},
  {"CHOOSE FROM SD", "ВЫБРАТЬ С SD"}, {"RESET TO DEFAULT", "СБРОС НА СТАНДАРТ"},

  // --- рисовалка ---
  {"PAINT  ", "РИСОВАЛКА"}, {"SD-OK", "SD-ОК"}, {"NO SD", "НЕТ SD"},
  {"SEL:SAVE UP:HIDE BACK", "ОК:СОХР UP:СКРЫТЬ НАЗ"}, {"SAVED", "СОХР."}, {"SAVE ERR", "ОШИБКА"},

  // --- проводник (в строке помещается не больше 20 символов) ---
  {"  (NO SD CARD)", "  (НЕТ SD-КАРТЫ)"}, {"  (EMPTY)", "  (ПУСТО)"},
  {"TOO MANY FILES", "СЛИШКОМ МНОГО ФАЙЛОВ"}, {"BACK: CANCEL", "НАЗАД: ОТМЕНА"},
  {"DELETE FOLDER?", "УДАЛИТЬ ПАПКУ?"}, {"DELETE FILE?", "УДАЛИТЬ ФАЙЛ?"},
  {"WITH ALL CONTENTS!", "ВМЕСТЕ С СОДЕРЖИМЫМ!"}, {"INFO", "ИНФО"}, {"(EMPTY FILE)", "(ПУСТОЙ ФАЙЛ)"},
  {"FILES", "ФАЙЛЫ"}, {"SELECT: RETRY", "ВЫБОР: ПОВТОР"}, {"BACK:   EXIT", "НАЗАД:  ВЫХОД"},
  {"WI-FI UPLOAD", "ЗАГРУЗКА ПО WI-FI"}, {"WAITING...", "ОЖИДАНИЕ..."}, {"BACK: STOP", "НАЗАД: СТОП"},
  {"OPEN", "ОТКРЫТЬ"}, {"DELETE", "УДАЛИТЬ"}, {"DELETED", "УДАЛЕНО"},
  {"DELETE FAILED", "ОШИБКА УДАЛЕНИЯ"},
  {"COPY", "КОПИРОВАТЬ"}, {"MOVE", "ПЕРЕНЕСТИ"}, 
  {"+ PASTE", "+ ВСТАВИТЬ"}, 
  {"COPYING...", "КОПИРОВАНИЕ..."}, {"MOVING...", "ПЕРЕНОС..."},
  {"COPIED", "СКОПИРОВАНО"}, {"MOVED", "ПЕРЕНЕСЕНО"}, 
  {"COPY FAILED", "ОШИБКА КОПИРОВАНИЯ"}, {"MOVE FAILED", "ОШИБКА ПЕРЕНОСА"},
  {"CANCELLED", "ОТМЕНЕНО"},
  {"PICK FOLDER > PASTE", "ПАПКА > ВСТАВИТЬ"}, {"CLIPBOARD CLEARED", "БУФЕР ОЧИЩЕН"},
  {"SAME FOLDER", "ТА ЖЕ ПАПКА"}, {"INTO ITSELF", "ПАПКА В СЕБЯ НЕЛЬЗЯ"},
  {"NAME EXISTS", "ИМЯ ЗАНЯТО"}, 
  {"SOURCE GONE", "ФАЙЛ ИСЧЕЗ"}, {"READ-ONLY", "ТОЛЬКО ЧТЕНИЕ"},
};
#define TR_COUNT (sizeof(TR_TABLE) / sizeof(TR_TABLE[0]))

// Перевод строки (только в режиме RU). nullptr — перевода нет.
static const char* trLookup(const char* s) {
  if (!langRU || !s || !s[0]) return nullptr;
  for (size_t i = 0; i < TR_COUNT; i++) {
    const char* k = TR_TABLE[i].en;
    if (k[0] == s[0] && strcmp(k, s) == 0) return TR_TABLE[i].ru;
  }
  return nullptr;
}

// Перевод, а если его нет — исходная строка.
static inline const char* trs(const char* s) {
  const char* t = trLookup(s);
  return t ? t : s;
}

// Длина строки в символах (кириллица в UTF-8 занимает 2 байта).
static size_t trLen(const char* s) {
  size_t n = 0;
  for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++;
  return n;
}

// ---------- Дисплей с поддержкой перевода и кириллицы ----------
class LangDisplay : public Adafruit_SSD1306 {
 public:
  LangDisplay(uint8_t w, uint8_t h, TwoWire* twi, int8_t rst)
      : Adafruit_SSD1306(w, h, twi, rst) {}

  using Print::write;
  size_t write(const uint8_t* buf, size_t size) override {
    if (langRU && size >= 2 && size < 64) {
      char tmp[64];
      memcpy(tmp, buf, size);
      tmp[size] = 0;
      const char* t = trLookup(tmp);
      if (t) { rawWrite((const uint8_t*)t, strlen(t)); return size; }
    }
    return rawWrite(buf, size);
  }

 private:
  // Печать без перевода: UTF-8 кириллица рисуется нашим шрифтом, остальное — как раньше.
  size_t rawWrite(const uint8_t* buf, size_t size) {
    size_t i = 0;
    while (i < size) {
      uint8_t c = buf[i];
      if ((c == 0xD0 || c == 0xD1) && i + 1 < size && !gfxFont) {
        uint8_t d = buf[i + 1];
        uint16_t cell = 0;
        if (c == 0xD0) {
          if (d == 0x81) cell = 0x401;                                  // Ё
          else if (d >= 0x90 && d <= 0xBF) cell = 0x410 + (d - 0x90);   // А-п
        } else {
          if (d == 0x91) cell = 0x451;                                  // ё
          else if (d >= 0x80 && d <= 0x8F) cell = 0x440 + (d - 0x80);   // р-я
        }
        if (cell) { drawCyr(cell); i += 2; continue; }
      }
      Adafruit_GFX::write(c);
      i++;
    }
    return size;
  }

  void drawCyr(uint16_t cell) {
    uint8_t g[5];
    if (cell == 0x401)       for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_YO_UP[i]);
    else if (cell == 0x451)  for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_YO_LO[i]);
    else if (cell <= 0x42F)  for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_UP[cell - 0x410][i]);
    else                     for (int i = 0; i < 5; i++) g[i] = pgm_read_byte(&FM_CYR_LO[cell - 0x430][i]);

    int s = textsize_x ? textsize_x : 1;
    if (wrap && (cursor_x + s * 6 > _width)) {          // перенос строки, как у обычных символов
      cursor_x = 0;
      cursor_y += textsize_y * 8;
    }
    if (textbgcolor != textcolor) fillRect(cursor_x, cursor_y, s * 6, textsize_y * 8, textbgcolor);
    for (int cx = 0; cx < 5; cx++) {
      for (int ry = 0; ry < 8; ry++) {
        if (!(g[cx] & (1 << ry))) continue;
        if (s == 1 && textsize_y == 1) drawPixel(cursor_x + cx, cursor_y + ry, textcolor);
        else fillRect(cursor_x + cx * s, cursor_y + ry * textsize_y, s, textsize_y, textcolor);
      }
    }
    cursor_x += s * 6;
  }
};

#endif
