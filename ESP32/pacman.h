#ifndef _pacman_game_h
#define _pacman_game_h
// ============================================================
//  PAC-MAN для OLED 128x64
//
//  * Лабиринт 21x9 клеток по 6 px (126x54 px), под строкой счёта.
//  * 4 призрака с «характерами» (как в оригинале):
//      Blinky — гонится за вами, Pinky — целится вперёд,
//      Inky — хитрый, Clyde — трусит, когда близко.
//    На 1-м уровне 3 призрака, дальше 4. Режимы SCATTER / CHASE / FRIGHT.
//  * Энерго-таблетки (4 угла): призраки синеют, их можно съесть (20/40/80/160).
//    Съеденный призрак «глазами» возвращается в домик и выходит снова.
//  * Туннель по краям средней строки. Жизни: 3, бонусная жизнь каждые 250 очков.
//  * Уровни: каждый следующий быстрее, фаза страха короче.
//  * Управление: стрелки (кнопки направления). BACK — пауза/выход (как в других играх).
//  * Рекорд хранится в 2 байтах EEPROM (EE_PACMAN_LO / EE_PACMAN_HI).
// ============================================================

// прямые объявления из основного скетча
inline void setDrawColor();
inline void setInverseColor();
inline void drawRect(int x, int y, int w, int h, bool fill);
void gameOver(String game, int score, int eeAddr, int &highScore);

#define PAC_COLS      21
#define PAC_ROWS      9
#define PAC_T         6                    // размер клетки, px
#define PAC_OX        1                    // сдвиг лабиринта на экране
#define PAC_OY        10
#define PAC_WPX       (PAC_COLS * PAC_T)   // 126 — ширина лабиринта (для туннеля)
#define PAC_FRAME_MS  30                   // период кадра игры
#define PAC_TUNNEL_ROW 4
#define PAC_HOME_R    4                    // клетка «домика» призраков (центр)
#define PAC_HOME_C    10

// '#' стена, '.' точка, 'o' энерго-таблетка, ' ' пусто (домик), '-' дверь домика
static const char PAC_MAZE[PAC_ROWS][PAC_COLS + 1] PROGMEM = {
  "#####################",
  "#o.................o#",
  "#.###.#.##.##.#.###.#",
  "#.###.#.##-##.#.###.#",
  "........#   #........",
  "#.###.#.#####.#.###.#",
  "#.###.#.#####.#.###.#",
  "#o.................o#",
  "#####################"
};

// Спрайты 6x6 (бит 7 = левый столбец). Пакман смотрит ВПРАВО, остальное — поворотом.
static const uint8_t PAC_SPR_PAC[3][6] PROGMEM = {
  {0x78, 0xFC, 0xFC, 0xFC, 0xFC, 0x78},    // рот закрыт
  {0x78, 0xF8, 0xF0, 0xF0, 0xF8, 0x78},    // рот приоткрыт
  {0x70, 0xE0, 0xC0, 0xC0, 0xE0, 0x70}     // рот широко
};
static const uint8_t PAC_SPR_GHOST[2][6] PROGMEM = {
  {0x78, 0xFC, 0xFC, 0xFC, 0xFC, 0xB4},    // «юбка» кадр A
  {0x78, 0xFC, 0xFC, 0xFC, 0xFC, 0xCC}     // «юбка» кадр B
};

enum { PG_HOUSE = 0, PG_LEAVE, PG_NORMAL, PG_FRIGHT, PG_EYES };
enum { PS_READY = 0, PS_PLAY, PS_FREEZE, PS_DYING, PS_CLEAR };

struct PacGhost {
  int16_t  x, y;           // позиция (левый верх клетки-спрайта), px внутри лабиринта
  int8_t   dx, dy;         // направление
  uint8_t  mode;           // PG_*
  uint8_t  type;           // 0 Blinky, 1 Pinky, 2 Inky, 3 Clyde
  uint16_t timer;          // задержка выхода из домика (кадры)
  uint16_t acc;            // накопитель скорости (1/16 px)
};

static const int8_t PAC_DX[4] = { 0, -1, 0, 1 };   // порядок приоритета как в оригинале: вверх, влево, вниз, вправо
static const int8_t PAC_DY[4] = { -1, 0, 1, 0 };

static bool     pacmanActive = false;
int             highPacman = 0;

static uint8_t  pacCell[PAC_ROWS][PAC_COLS];       // 0 пусто, 1 точка, 2 таблетка
static uint8_t  pacHomeDist[PAC_ROWS][PAC_COLS];   // расстояние до домика (для «глаз»)
static PacGhost pacG[4];
static int      pacGhostN = 3;

static int16_t  pacX, pacY;
static int8_t   pacDx, pacDy, pacWx, pacWy;
static uint16_t pacAcc;
static int      pacScore, pacLives, pacLevel, pacDots, pacChain, pacBonusMark;
static uint16_t pacFright, pacModeTimer, pacStateTimer, pacFreezeT;
static uint8_t  pacModeIdx, pacState;
static bool     pacScatter, pacToggle;
static uint32_t pacFrame;
static unsigned long pacLastTick;
static int16_t  pacPopX, pacPopY; static int pacPopVal;

static const uint16_t PAC_SCHED[] = { 120, 360, 120, 360, 90, 0xFFFF };   // SCATTER, CHASE, ... (кадры)

// ---------------- клетки лабиринта ----------------
static inline char pacCh(int r, int c) {
  if (r < 0 || r >= PAC_ROWS) return '#';
  c = ((c % PAC_COLS) + PAC_COLS) % PAC_COLS;
  return (char)pgm_read_byte(&PAC_MAZE[r][c]);
}
static inline bool pacOpen(int r, int c, bool door) {
  char ch = pacCh(r, c);
  if (ch == '#') return false;
  if (ch == '-') return door;
  return true;
}
static inline bool pacAligned(int x, int y) { return (x % PAC_T) == 0 && (y % PAC_T) == 0; }
static inline void pacWrapX(int16_t &x) {
  if (x < 0) x += PAC_WPX; else if (x >= PAC_WPX) x -= PAC_WPX;
}
static inline void pacBeep(int f, int ms) { tone(SPEAKER_PIN, f, ms); }

// ---------------- подготовка ----------------
static void pacBuildHomeDist() {
  for (int r = 0; r < PAC_ROWS; r++) for (int c = 0; c < PAC_COLS; c++) pacHomeDist[r][c] = 255;
  int16_t qr[PAC_ROWS * PAC_COLS], qc[PAC_ROWS * PAC_COLS];
  int qh = 0, qt = 0;
  pacHomeDist[PAC_HOME_R][PAC_HOME_C] = 0;
  qr[qt] = PAC_HOME_R; qc[qt] = PAC_HOME_C; qt++;
  while (qh < qt) {
    int r = qr[qh], c = qc[qh]; qh++;
    for (int d = 0; d < 4; d++) {
      int nr = r + PAC_DY[d], nc = (c + PAC_DX[d] + PAC_COLS) % PAC_COLS;
      if (!pacOpen(nr, nc, true)) continue;
      if (pacHomeDist[nr][nc] != 255) continue;
      pacHomeDist[nr][nc] = pacHomeDist[r][c] + 1;
      qr[qt] = nr; qc[qt] = nc; qt++;
    }
  }
}

static void pacLoadMaze() {
  pacDots = 0;
  for (int r = 0; r < PAC_ROWS; r++) {
    for (int c = 0; c < PAC_COLS; c++) {
      char ch = pacCh(r, c);
      pacCell[r][c] = (ch == '.') ? 1 : (ch == 'o' ? 2 : 0);
      if (pacCell[r][c]) pacDots++;
    }
  }
  pacCell[7][10] = 0;      // под стартом пакмана точки нет
  pacDots--;
}

static void pacResetActors() {
  pacX = PAC_HOME_C * PAC_T; pacY = 7 * PAC_T;
  pacDx = -1; pacDy = 0; pacWx = -1; pacWy = 0; pacAcc = 0;

  pacGhostN = (pacLevel < 2) ? 3 : 4;
  static const uint8_t order[4] = { 0, 1, 3, 2 };            // уровень 1: Blinky, Pinky, Clyde; потом + Inky
  int rel = 90 - (pacLevel > 4 ? 4 : pacLevel) * 12;         // пауза между выходами, кадры
  for (int i = 0; i < 4; i++) {
    PacGhost &g = pacG[i];
    g.type = order[i]; g.acc = 0; g.dx = 0; g.dy = 0;
    if (i == 0) {                                            // Blinky стоит над домиком
      g.x = PAC_HOME_C * PAC_T; g.y = 1 * PAC_T; g.dx = -1; g.mode = PG_NORMAL; g.timer = 0;
    } else {
      g.mode = PG_HOUSE;
      g.y = PAC_HOME_R * PAC_T;
      g.x = (PAC_HOME_C + (i == 2 ? -1 : (i == 3 ? 1 : 0))) * PAC_T;   // слоты: центр, лево, право
      g.timer = (i == 1) ? 10 : (uint16_t)(i * rel);
    }
  }
  pacFright = 0; pacChain = 0; pacFreezeT = 0;
  pacModeIdx = 0; pacModeTimer = PAC_SCHED[0]; pacScatter = true;
}

static void pacBeginRound() {            // «READY!» перед стартом / после гибели
  pacResetActors();
  pacState = PS_READY;
  pacStateTimer = 45;
}

static void pacStartLevel() {
  pacLoadMaze();
  pacBeginRound();
}

void initPacman() {
  pacBuildHomeDist();
  pacScore = 0; pacLives = 3; pacLevel = 1; pacBonusMark = 0;
  pacFrame = 0; pacToggle = false; pacLastTick = millis();
  pacmanActive = true;
  pacStartLevel();
}

// ---------------- призраки ----------------
static void pacTarget(const PacGhost &g, int &tr, int &tc) {
  int pr = (pacY + 3) / PAC_T, pc = ((pacX + 3) % PAC_WPX) / PAC_T;
  if (pacScatter) {                      // у каждого — свой угол
    static const int8_t cr[4] = { 1, 1, 7, 7 };
    static const int8_t cc[4] = { 19, 1, 19, 1 };
    tr = cr[g.type]; tc = cc[g.type];
    return;
  }
  switch (g.type) {
    case 0: tr = pr; tc = pc; break;                                     // Blinky
    case 1: tr = pr + pacDy * 2; tc = pc + pacDx * 2; break;             // Pinky — на 2 клетки вперёд
    case 2: {                                                            // Inky — зеркально от Blinky
      int ar = pr + pacDy * 1, ac = pc + pacDx * 1;
      int br = pacG[0].y / PAC_T, bc = pacG[0].x / PAC_T;
      tr = ar * 2 - br; tc = ac * 2 - bc; break;
    }
    default: {                                                           // Clyde — трусит вблизи
      int gr = g.y / PAC_T, gc = g.x / PAC_T;
      int d2 = (gr - pr) * (gr - pr) + (gc - pc) * (gc - pc);
      if (d2 > 16) { tr = pr; tc = pc; } else { tr = 7; tc = 1; }
    }
  }
}

static void pacGhostChoose(PacGhost &g) {
  int r = g.y / PAC_T, c = g.x / PAC_T;

  if (g.mode == PG_EYES) {                         // «глаза» летят домой по карте расстояний
    if (r == PAC_HOME_R && c == PAC_HOME_C) { g.mode = PG_LEAVE; g.dx = 0; g.dy = 0; return; }
    int best = -1, bd = 1000;
    for (int d = 0; d < 4; d++) {
      int nr = r + PAC_DY[d], nc = (c + PAC_DX[d] + PAC_COLS) % PAC_COLS;
      if (!pacOpen(nr, nc, true)) continue;
      if (pacHomeDist[nr][nc] < bd) { bd = pacHomeDist[nr][nc]; best = d; }
    }
    if (best >= 0) { g.dx = PAC_DX[best]; g.dy = PAC_DY[best]; }
    return;
  }

  int rev = -1;
  for (int d = 0; d < 4; d++) if ((g.dx || g.dy) && PAC_DX[d] == -g.dx && PAC_DY[d] == -g.dy) rev = d;
  int cand[4], n = 0;
  for (int d = 0; d < 4; d++) {
    if (d == rev) continue;
    if (pacOpen(r + PAC_DY[d], c + PAC_DX[d], false)) cand[n++] = d;
  }
  if (n == 0) {
    if (rev >= 0 && pacOpen(r + PAC_DY[rev], c + PAC_DX[rev], false)) cand[n++] = rev;
    else { g.dx = 0; g.dy = 0; return; }
  }
  int pick = cand[0];
  if (n > 1) {
    if (g.mode == PG_FRIGHT) {
      pick = cand[random(n)];
    } else {
      int tr, tc; pacTarget(g, tr, tc);
      long best = 1000000L;
      for (int i = 0; i < n; i++) {
        int nr = r + PAC_DY[cand[i]], nc = c + PAC_DX[cand[i]];
        long dd = (long)(nr - tr) * (nr - tr) + (long)(nc - tc) * (nc - tc);
        if (dd < best) { best = dd; pick = cand[i]; }
      }
    }
  }
  g.dx = PAC_DX[pick]; g.dy = PAC_DY[pick];
}

static void pacGhostStep(PacGhost &g) {
  if (g.mode == PG_HOUSE) return;
  if (g.mode == PG_LEAVE) {                        // из домика: к центру, затем вверх через дверь
    int cx = PAC_HOME_C * PAC_T;
    if (g.x != cx) { g.dx = (g.x < cx) ? 1 : -1; g.dy = 0; }
    else           { g.dx = 0; g.dy = -1; }
    g.x += g.dx; g.y += g.dy;
    if (g.x == cx && g.y <= 1 * PAC_T) {
      g.y = 1 * PAC_T;
      g.mode = pacFright ? PG_FRIGHT : PG_NORMAL;
      g.dx = (random(2) ? 1 : -1); g.dy = 0;
    }
    return;
  }
  if (pacAligned(g.x, g.y)) pacGhostChoose(g);
  if (g.mode == PG_LEAVE) return;                  // «глаза» только что добрались до дома
  g.x += g.dx; g.y += g.dy;
  pacWrapX(g.x);
}

static int pacGhostSpeed(const PacGhost &g) {      // в 1/16 px за кадр
  switch (g.mode) {
    case PG_LEAVE:  return 17;
    case PG_EYES:   return 36;
    case PG_FRIGHT: return 9;
    default: {
      int lv = pacLevel > 6 ? 6 : pacLevel;
      return 12 + (lv * 3) / 4;                    // 12..16 (медленнее пакмана)
    }
  }
}

static void pacReverseGhosts() {
  for (int i = 0; i < pacGhostN; i++) {
    PacGhost &g = pacG[i];
    if (g.mode == PG_NORMAL || g.mode == PG_FRIGHT) { g.dx = -g.dx; g.dy = -g.dy; }
  }
}

// ---------------- пакман ----------------
static void pacEatAt() {
  int c = ((pacX + 3) % PAC_WPX) / PAC_T, r = (pacY + 3) / PAC_T;
  if (r < 0 || r >= PAC_ROWS) return;
  uint8_t v = pacCell[r][c];
  if (!v) return;
  pacCell[r][c] = 0;
  pacDots--;
  pacToggle = !pacToggle;
  if (v == 1) {
    pacScore += 1;
    pacBeep(pacToggle ? 520 : 390, 10);
  } else {
    pacScore += 5;
    pacBeep(900, 14);
    pacFright = (uint16_t)max(70, 210 - (pacLevel - 1) * 22);
    pacChain = 0;
    for (int i = 0; i < pacGhostN; i++) {
      PacGhost &g = pacG[i];
      if (g.mode == PG_NORMAL) { g.mode = PG_FRIGHT; g.dx = -g.dx; g.dy = -g.dy; }
    }
  }
  if (pacScore / 250 > pacBonusMark) {            // бонусная жизнь
    pacBonusMark = pacScore / 250;
    if (pacLives < 5) pacLives++;
    pacBeep(1500, 15);
  }
}

static void pacMoveStep() {
  if (!pacAligned(pacX, pacY)) {
    if ((pacDx || pacDy) && pacWx == -pacDx && pacWy == -pacDy) { pacDx = pacWx; pacDy = pacWy; }   // разворот на ходу
  } else {
    int r = pacY / PAC_T, c = pacX / PAC_T;
    if ((pacWx || pacWy) && pacOpen(r + pacWy, c + pacWx, false)) { pacDx = pacWx; pacDy = pacWy; }
    if (!pacOpen(r + pacDy, c + pacDx, false)) { pacDx = 0; pacDy = 0; }
  }
  pacX += pacDx; pacY += pacDy;
  pacWrapX(pacX);
  pacEatAt();
}

static bool pacHit(const PacGhost &g) {
  int ax = g.x - pacX; if (ax < 0) ax = -ax;
  if (PAC_WPX - ax < ax) ax = PAC_WPX - ax;        // через туннель
  int ay = g.y - pacY; if (ay < 0) ay = -ay;
  return ax < 4 && ay < 4;
}

static void pacGameOver() {
  pacmanActive = false;
  gameActive = false;
  if (pacScore > highPacman) {                     // рекорд хранится в 2 байтах
    highPacman = pacScore > 65534 ? 65534 : pacScore;
    EEPROM.write(EE_PACMAN_LO, highPacman & 0xFF);
    EEPROM.write(EE_PACMAN_HI, (highPacman >> 8) & 0xFF);
    EEPROM.commit();
  }
  int noSave = 99999;                              // однобайтовое сохранение внутри gameOver() отключаем
  gameOver("PACMAN", pacScore, EE_PACMAN_LO, noSave);
}

static void pacCheckCollisions() {
  for (int i = 0; i < pacGhostN; i++) {
    PacGhost &g = pacG[i];
    if (g.mode == PG_FRIGHT && pacHit(g)) {
      int pts = 20 << (pacChain > 3 ? 3 : pacChain);
      pacChain++;
      pacScore += pts;
      pacPopX = g.x; pacPopY = g.y; pacPopVal = pts;
      g.mode = PG_EYES;
      pacFreezeT = 14; pacState = PS_FREEZE;
      pacBeep(1200, 14);
      return;
    }
    if ((g.mode == PG_NORMAL || g.mode == PG_LEAVE) && pacHit(g)) {
      pacState = PS_DYING; pacStateTimer = 40;
      pacBeep(200, 20);
      return;
    }
  }
}

// ---------------- отрисовка ----------------
static inline bool pacWallAt(int r, int c) {       // за пределами поля считаем стеной (линий там нет)
  if (r < 0 || r >= PAC_ROWS || c < 0 || c >= PAC_COLS) return true;
  return pacCh(r, c) == '#';
}

static void pacDrawWalls(uint16_t col) {
  for (int r = 0; r < PAC_ROWS; r++) {
    for (int c = 0; c < PAC_COLS; c++) {
      int tx = PAC_OX + c * PAC_T, ty = PAC_OY + r * PAC_T;
      char ch = pacCh(r, c);
      if (ch == '-') {                              // дверь домика — пунктир
        for (int i = 0; i < PAC_T; i += 2) display.drawPixel(tx + i, ty + 2, col);
        continue;
      }
      if (ch != '#') continue;
      bool up = !pacWallAt(r - 1, c), dn = !pacWallAt(r + 1, c);
      bool lf = !pacWallAt(r, c - 1), rt = !pacWallAt(r, c + 1);
      if (up) display.drawFastHLine(tx + (lf ? 1 : 0), ty + 1, PAC_T - (lf ? 1 : 0) - (rt ? 1 : 0), col);
      if (dn) display.drawFastHLine(tx + (lf ? 1 : 0), ty + 4, PAC_T - (lf ? 1 : 0) - (rt ? 1 : 0), col);
      if (lf) display.drawFastVLine(tx + 1, ty + (up ? 1 : 0), PAC_T - (up ? 1 : 0) - (dn ? 1 : 0), col);
      if (rt) display.drawFastVLine(tx + 4, ty + (up ? 1 : 0), PAC_T - (up ? 1 : 0) - (dn ? 1 : 0), col);
      // внутренние углы: диагональная клетка свободна, а обе соседние по стороне — стены
      if (pacWallAt(r - 1, c) && pacWallAt(r, c - 1) && !pacWallAt(r - 1, c - 1)) {
        display.drawPixel(tx + 1, ty, col); display.drawPixel(tx + 1, ty + 1, col); display.drawPixel(tx, ty + 1, col);
      }
      if (pacWallAt(r - 1, c) && pacWallAt(r, c + 1) && !pacWallAt(r - 1, c + 1)) {
        display.drawPixel(tx + 4, ty, col); display.drawPixel(tx + 4, ty + 1, col); display.drawPixel(tx + 5, ty + 1, col);
      }
      if (pacWallAt(r + 1, c) && pacWallAt(r, c - 1) && !pacWallAt(r + 1, c - 1)) {
        display.drawPixel(tx + 1, ty + 5, col); display.drawPixel(tx + 1, ty + 4, col); display.drawPixel(tx, ty + 4, col);
      }
      if (pacWallAt(r + 1, c) && pacWallAt(r, c + 1) && !pacWallAt(r + 1, c + 1)) {
        display.drawPixel(tx + 4, ty + 5, col); display.drawPixel(tx + 4, ty + 4, col); display.drawPixel(tx + 5, ty + 4, col);
      }
    }
  }
}

// Рисует спрайт 6x6. orient: 0 вправо, 1 влево, 2 вверх, 3 вниз.
static void pacSprite(int sx, int sy, const uint8_t* rows, uint8_t orient, uint16_t col) {
  for (int i = 0; i < 6; i++) {
    uint8_t b = pgm_read_byte(&rows[i]);
    for (int j = 0; j < 6; j++) {
      if (!(b & (0x80 >> j))) continue;
      int px, py;
      switch (orient) {
        case 1:  px = 5 - j; py = i; break;
        case 2:  px = i;     py = 5 - j; break;
        case 3:  px = 5 - i; py = j; break;
        default: px = j;     py = i; break;
      }
      display.drawPixel(sx + px, sy + py, col);
    }
  }
}

static void pacDrawGhost(const PacGhost &g, uint16_t fg, uint16_t bg) {
  int sx = PAC_OX + g.x, sy = PAC_OY + g.y;
  if (g.mode == PG_HOUSE && ((pacFrame >> 2) & 1)) sy++;          // покачивание в домике
  for (int k = 0; k < 2; k++) {
    if (k == 1) { if (g.x < 0) sx += PAC_WPX; else if (g.x > PAC_WPX - PAC_T) sx -= PAC_WPX; else break; }
    if (g.mode == PG_EYES) {
      display.drawPixel(sx + 1, sy + 2, fg); display.drawPixel(sx + 1, sy + 3, fg);
      display.drawPixel(sx + 4, sy + 2, fg); display.drawPixel(sx + 4, sy + 3, fg);
    } else if (g.mode == PG_FRIGHT) {
      bool blink = pacFright < 40 && ((pacFrame >> 2) & 1);        // в конце страха мигают
      pacSprite(sx, sy, PAC_SPR_GHOST[(pacFrame >> 2) & 1], 0, fg);
      if (!blink) {                                                 // «синий» призрак: решётка поверх
        for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++)
          if (((i + j) & 1) == 0) display.drawPixel(sx + j, sy + i, bg);
      } else {
        display.drawPixel(sx + 1, sy + 2, bg); display.drawPixel(sx + 4, sy + 2, bg);
      }
    } else {
      pacSprite(sx, sy, PAC_SPR_GHOST[(pacFrame >> 2) & 1], 0, fg);
      display.drawPixel(sx + 1, sy + 2, bg); display.drawPixel(sx + 1, sy + 3, bg);   // глаза
      display.drawPixel(sx + 4, sy + 2, bg); display.drawPixel(sx + 4, sy + 3, bg);
    }
    if (k == 0 && !(g.x < 0 || g.x > PAC_WPX - PAC_T)) break;
  }
}

static void pacDrawPac(uint16_t fg, uint16_t bg) {
  int sx = PAC_OX + pacX, sy = PAC_OY + pacY;
  if (pacState == PS_DYING) {                                       // анимация гибели
    int t = 40 - pacStateTimer;
    int cx = sx + 3, cy = sy + 3;
    if (t < 14) {
      const uint8_t* f = PAC_SPR_PAC[(t / 3) % 3];
      pacSprite(sx, sy, f, 2, fg);
    } else {
      int rad = 3 - (t - 14) / 7;
      if (rad >= 1) display.drawCircle(cx, cy, rad, fg);
      else if ((t - 14) / 7 == 3) { display.drawPixel(cx, cy, fg); }
    }
    return;
  }
  uint8_t orient = 0;
  if (pacDx < 0) orient = 1; else if (pacDy < 0) orient = 2; else if (pacDy > 0) orient = 3;
  else if (pacWx < 0) orient = 1;
  uint8_t frame = (pacDx || pacDy) ? (uint8_t)((pacFrame >> 1) % 4) : 0;   // 0,1,2,1
  if (frame == 3) frame = 1;
  for (int k = 0; k < 2; k++) {
    pacSprite(sx, sy, PAC_SPR_PAC[frame], orient, fg);
    if (pacX < 0) { sx += PAC_WPX; continue; }
    if (pacX > PAC_WPX - PAC_T) { sx -= PAC_WPX; continue; }
    break;
  }
  (void)bg;
}

static void pacDraw() {
  uint16_t fg = whiteTheme ? BLACK : WHITE;
  uint16_t bg = whiteTheme ? WHITE : BLACK;
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);

  // --- панель: счёт, уровень, жизни ---
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setTextSize(1);
  display.setCursor(2, 1);  display.print("SCORE:"); display.print(pacScore);
  display.setCursor(70, 1); display.print("Lv:");    display.print(pacLevel);
  for (int i = 0; i < pacLives - 1 && i < 4; i++) pacSprite(120 - i * 8, 2, PAC_SPR_PAC[1], 0, bg);

  // --- лабиринт ---
  bool flash = (pacState == PS_CLEAR) && ((pacStateTimer >> 2) & 1);
  pacDrawWalls(flash ? bg : fg);
  if (flash) display.fillRect(0, PAC_OY, 128, PAC_ROWS * PAC_T, fg), pacDrawWalls(bg);

  // --- точки и таблетки ---
  bool pulse = (pacFrame >> 3) & 1;
  for (int r = 0; r < PAC_ROWS; r++) {
    for (int c = 0; c < PAC_COLS; c++) {
      uint8_t v = pacCell[r][c];
      if (!v) continue;
      int tx = PAC_OX + c * PAC_T, ty = PAC_OY + r * PAC_T;
      if (v == 1) display.fillRect(tx + 2, ty + 2, 2, 2, flash ? bg : fg);
      else if (pulse) display.fillCircle(tx + 3, ty + 3, 2, flash ? bg : fg);
      else            display.fillCircle(tx + 3, ty + 3, 1, flash ? bg : fg);
    }
  }

  // --- призраки и пакман ---
  if (pacState != PS_DYING || pacStateTimer > 28)
    for (int i = 0; i < pacGhostN; i++) pacDrawGhost(pacG[i], fg, bg);
  pacDrawPac(fg, bg);

  if (pacState == PS_FREEZE) {                      // очки за съеденного призрака
    int sx = PAC_OX + pacPopX - 4, sy = PAC_OY + pacPopY;
    if (sx < 0) sx = 0;
    if (sx > 128 - 24) sx = 128 - 24;
    display.fillRect(sx, sy, 24, 7, bg);
    display.setTextColor(fg);
    display.setCursor(sx + 1, sy);
    display.print(pacPopVal);
  }
  if (pacState == PS_READY) {                       // «READY!» в центре
    display.fillRect(44, 41, 40, 10, bg);
    display.setTextColor(fg);
    display.setCursor(46, 42);
    display.print("READY!");
  }
  setDrawColor();
  display.display();
}

// ---------------- главный цикл игры ----------------
void runPacman() {
  if (!pacmanActive) return;

  // ввод читаем на каждом проходе loop() — короткие нажатия не теряются
  if (digitalRead(BTN_UP) == LOW)         { pacWx = 0;  pacWy = -1; }
  else if (digitalRead(BTN_DOWN) == LOW)  { pacWx = 0;  pacWy = 1;  }
  else if (digitalRead(BTN_LEFT) == LOW)  { pacWx = -1; pacWy = 0;  }
  else if (digitalRead(BTN_RIGHT) == LOW) { pacWx = 1;  pacWy = 0;  }

  unsigned long now = millis();
  if (now - pacLastTick < PAC_FRAME_MS) return;
  pacLastTick = now;                                 // после паузы выполняется только один кадр — без «скачка»
  pacFrame++;

  switch (pacState) {
    case PS_READY:
      if (pacStateTimer > 0) pacStateTimer--;
      if (pacStateTimer == 0) pacState = PS_PLAY;
      break;

    case PS_FREEZE:
      if (pacFreezeT > 0) pacFreezeT--;
      if (pacFreezeT == 0) pacState = PS_PLAY;
      break;

    case PS_DYING:
      if (pacStateTimer > 0) pacStateTimer--;
      if (pacStateTimer == 0) {
        pacLives--;
        if (pacLives <= 0) { pacGameOver(); return; }
        pacBeginRound();
      }
      break;

    case PS_CLEAR:
      if (pacStateTimer > 0) pacStateTimer--;
      if (pacStateTimer == 0) {
        pacLevel++;
        pacScore += 10;
        pacStartLevel();
      }
      break;

    default: {                                       // PS_PLAY
      // пакман
      pacAcc += 17;                                   // скорость пакмана ~1.06 px/кадр
      int steps = pacAcc >> 4; pacAcc &= 15;
      for (int s = 0; s < steps; s++) pacMoveStep();

      // призраки
      for (int i = 0; i < pacGhostN; i++) {
        PacGhost &g = pacG[i];
        if (g.mode == PG_HOUSE) {
          if (g.timer > 0) g.timer--;
          if (g.timer == 0) g.mode = PG_LEAVE;
          continue;
        }
        g.acc += pacGhostSpeed(g);
        int gs = g.acc >> 4; g.acc &= 15;
        for (int s = 0; s < gs; s++) pacGhostStep(g);
      }

      // таймеры: страх, SCATTER/CHASE
      if (pacFright > 0) {
        pacFright--;
        if (pacFright == 0) {
          for (int i = 0; i < pacGhostN; i++) if (pacG[i].mode == PG_FRIGHT) pacG[i].mode = PG_NORMAL;
        }
      } else if (pacModeTimer != 0xFFFF) {
        if (pacModeTimer > 0) pacModeTimer--;
        if (pacModeTimer == 0) {
          pacModeIdx++;
          pacModeTimer = PAC_SCHED[pacModeIdx];
          pacScatter = ((pacModeIdx & 1) == 0);
          pacReverseGhosts();
        }
      }

      pacCheckCollisions();

      if (pacState == PS_PLAY && pacDots <= 0) {
        pacState = PS_CLEAR; pacStateTimer = 40;
        pacBeep(1000, 15);
      }
      break;
    }
  }
  pacDraw();
}

#endif
