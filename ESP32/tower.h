#ifndef _tower_game_h
#define _tower_game_h
// ============================================================
//  TOWER (башня из полосок) для OLED 128x64
//
//  * Полоска летает по горизонтали туда-сюда над башней.
//  * SELECT (или UP/DOWN) — бросить полоску на башню.
//  * Всё, что выступает за край нижней полоски, обрезается и падает вниз;
//    следующая полоска будет ровно такой же ширины, как осталось.
//  * Попал «чётко» (в пределах 1 px) — полоска встаёт идеально, ширина не
//    теряется; три идеальных подряд — полоска даже немного расширяется.
//  * Промах мимо башни — конец игры. Счёт = число построенных этажей.
//  * Камера плавно едет вверх, когда башня доходит до середины экрана.
//  * BACK — пауза/выход (обрабатывается общим кодом пауз в loop()).
//  * Рекорд — 1 байт EEPROM (EE_TOWER), сохраняется внутри gameOver().
// ============================================================

// прямые объявления из основного скетча
inline void setDrawColor();
inline void setInverseColor();
void gameOver(String game, int score, int eeAddr, int &highScore);

#define TW_BLOCK_H    5     // высота полоски, px
#define TW_PITCH      6     // шаг по вертикали (полоска + 1 px зазора)
#define TW_BASE_Y     58    // экранный Y самой нижней полоски (пока башня низкая)
#define TW_TOP_Y      36    // куда камера «прижимает» верхнюю полоску
#define TW_KEEP       14    // сколько верхних полосок помним (больше на экран не влезает)
#define TW_START_W    34    // ширина первой полоски
#define TW_MIN_W      2     // уже нельзя — считаем промахом
#define TW_FRAME_MS   25

int highTower = 0;

struct TowBlock { int16_t x; int16_t w; };
struct TowDebris { float x, y, vy; int16_t w; bool on; };

static bool       towerActive = false;
static TowBlock   towB[TW_KEEP];          // [0] — верхняя полоска башни
static int        towN = 0;               // сколько полосок в towB
static int        towScore = 0;           // построено этажей (без основания)
static float      towX = 0, towSpeed = 1; // летящая полоска: позиция и скорость (px/кадр)
static int        towDir = 1;
static int        towW = TW_START_W;      // ширина летящей полоски
static float      towTopY = TW_BASE_Y;    // экранный Y верхней полоски башни (плавный)
static TowDebris  towD[4];
static int        towCombo = 0;           // идеальных попаданий подряд
static int        towFlash = 0;           // сколько кадров ещё показывать «PERFECT!»
static int        towDead = 0;            // >0 — идёт анимация падения после промаха
static bool       towArmed = false;       // кнопка отпущена — можно бросать
static unsigned long towLastTick = 0, towLastCall = 0;

static inline void towBeep(int f, int ms) { tone(SPEAKER_PIN, f, ms); }

static float towSpeedFor(int score) {
  float s = 1.1f + score * 0.07f;
  return s > 3.4f ? 3.4f : s;
}

static void towAddDebris(float x, int w, float y) {
  for (int i = 0; i < 4; i++) {
    if (!towD[i].on) {
      towD[i].x = x; towD[i].y = y; towD[i].vy = 0.4f; towD[i].w = w; towD[i].on = true;
      return;
    }
  }
}

// запускает следующую летящую полоску с противоположной стороны
static void towSpawn() {
  towDir = -towDir;
  towX = (towDir > 0) ? 0 : (128 - towW);
  towSpeed = towSpeedFor(towScore);
}

void initTower() {
  towerActive = true;
  towScore = 0; towCombo = 0; towFlash = 0; towDead = 0;
  for (int i = 0; i < 4; i++) towD[i].on = false;
  towB[0].w = TW_START_W;
  towB[0].x = (128 - TW_START_W) / 2;
  towN = 1;
  towW = TW_START_W;
  towTopY = TW_BASE_Y;
  towDir = random(2) ? 1 : -1;     // towSpawn() развернёт — стартуем с любой стороны
  towSpawn();
  towArmed = false;                 // нажатие, которым запустили игру, бросать не должно
  towLastTick = millis();
  towLastCall = millis();
}

static void towFinish() {
  towerActive = false;
  int hs = highTower;
  gameOver("TOWER", towScore, EE_TOWER, hs);
  highTower = hs;
}

// бросок полоски
static void towDrop() {
  TowBlock top = towB[0];
  int mx = (int)roundf(towX);
  int movY = (int)roundf(towTopY) - TW_PITCH;

  int left  = mx > top.x ? mx : top.x;
  int right = (mx + towW) < (top.x + top.w) ? (mx + towW) : (top.x + top.w);
  int ov = right - left;

  if (ov < TW_MIN_W) {                     // промах — полоска падает целиком
    towAddDebris((float)mx, towW, (float)movY);
    towDead = 36;
    towBeep(160, 140);
    return;
  }

  bool perfect = (abs(mx - top.x) <= 1 && towW == top.w);
  if (perfect) {
    left = top.x; ov = top.w;
    towCombo++;
    towFlash = 22;
    if (towCombo >= 3 && ov < TW_START_W) {      // награда: полоска немного растёт
      int grow = TW_START_W - ov; if (grow > 2) grow = 2;
      left -= grow / 2; ov += grow;
      if (left < 0) left = 0;
      if (left + ov > 128) left = 128 - ov;
    }
    towBeep(1100 + (towCombo > 6 ? 6 : towCombo) * 90, 45);
  } else {
    towCombo = 0;
    if (mx < top.x)             towAddDebris((float)mx, left - mx, (float)movY);           // выступ слева
    if (mx + towW > top.x + top.w) towAddDebris((float)right, (mx + towW) - right, (float)movY); // справа
    towBeep(420 + (towScore % 12) * 35, 35);
  }

  // новая верхняя полоска
  for (int i = TW_KEEP - 1; i > 0; i--) towB[i] = towB[i - 1];
  towB[0].x = left; towB[0].w = ov;
  if (towN < TW_KEEP) towN++;
  towW = ov;
  towScore++;
  towTopY -= TW_PITCH;                    
     // новая полоска «встаёт» туда, где летела прежняя
  towSpawn();
}

static void towDraw() {
  uint16_t fg = whiteTheme ? BLACK : WHITE;
  uint16_t bg = whiteTheme ? WHITE : BLACK;
  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);

  // --- счёт и рекорд ---
  setDrawColor();
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setCursor(2,1); display.print("SCORE:");
  display.print(towScore);
  display.setTextSize(1);
  setDrawColor();
  int best = (towScore > highTower) ? towScore : highTower;
  int digits = best >= 100 ? 3 : (best >= 10 ? 2 : 1);
  int bw = ((int)trLen(trs("BEST ")) + digits) * 6;

  // --- «идеально!» ---
  if (towFlash > 0) {
    int w = (int)trLen(trs("PERFECT!")) * 6;
    display.setCursor((128 - w) / 2 + 12, 20);
    display.print("PERFECT!");
  }

  // --- башня ---
  int ty = (int)roundf(towTopY);
  for (int i = 0; i < towN; i++) {
    int y = ty + i * TW_PITCH;
    if (y >= 64) break;
    if (y + TW_BLOCK_H <= 0) continue;
    display.fillRect(towB[i].x, y, towB[i].w, TW_BLOCK_H, fg);
    int floorNo = towScore - i;                       // отметки каждые 5 этажей по краям
    if (floorNo > 0 && floorNo % 5 == 0) {
      display.drawFastHLine(0, y + 2, 3, fg);
      display.drawFastHLine(125, y + 2, 3, fg);
    }
  }

  // --- летящая полоска ---
  if (!towDead) {
    int my = ty - TW_PITCH;
    display.fillRect((int)roundf(towX), my, towW, TW_BLOCK_H, fg);
  }

  // --- обрезки ---
  for (int i = 0; i < 4; i++) {
    if (!towD[i].on) continue;
    display.fillRect((int)roundf(towD[i].x), (int)roundf(towD[i].y), towD[i].w, TW_BLOCK_H, fg);
    // «дырочка» в обрезке, чтобы отличался от башни
    if (towD[i].w > 4) display.drawFastHLine((int)roundf(towD[i].x) + 1, (int)roundf(towD[i].y) + 2, towD[i].w - 2, bg);
  }

  display.display();
}

void runTower() {
  if (!towerActive) return;

  unsigned long now = millis();
  if (now - towLastCall > 150) towArmed = false;      // вышли из паузы — ждём, пока кнопку отпустят
  towLastCall = now;

  bool pressed = (digitalRead(BTN_SELECT) == LOW) || (digitalRead(BTN_UP) == LOW) ||
                 (digitalRead(BTN_DOWN) == LOW);
  if (!pressed) towArmed = true;
  if (pressed && towArmed && !towDead) {
    towArmed = false;
    towDrop();
  }

  if (now - towLastTick < TW_FRAME_MS) return;
  towLastTick = now;

  if (!towDead) {
    // движение летящей полоски (отскок от краёв экрана)
    towX += towDir * towSpeed;
    if (towX < 0)             { towX = 0;             towDir = 1;  }
    if (towX > 128 - towW)    { towX = 128 - towW;    towDir = -1; }
  }

  // камера: плавно подъезжаем к нужной высоте
  int count = towScore;                                  // верхняя полоска стоит на (count) этажей выше основания
  float target = TW_BASE_Y - (float)TW_PITCH * count;
  if (target < TW_TOP_Y) target = TW_TOP_Y;
  towTopY += (target - towTopY) * 0.25f;
  if (fabsf(target - towTopY) < 0.3f) towTopY = target;

  // обрезки падают
  for (int i = 0; i < 4; i++) {
    if (!towD[i].on) continue;
    towD[i].vy += 0.45f;
    towD[i].y  += towD[i].vy;
    if (towD[i].y > 64) towD[i].on = false;
  }

  if (towFlash > 0) towFlash--;

  if (towDead > 0) {
    towDead--;
    if (towDead == 0) { towFinish(); return; }
  }

  towDraw();
}

#endif
