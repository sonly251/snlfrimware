#ifndef _doom_game_h
#define _doom_game_h
#include <math.h>

// прямые объявления функций из основного скетча
inline void setDrawColor();
void gameOver(String game, int score, int eeAddr, int &highScore);
void playBeepSound();
void playCoinSound();
void playLaserSound();
void playErrorSound();
void playExplosionSound();

// ---- реализация прототипов из types.h / entities.h ----
UID create_uid(EType type, uint8_t x, uint8_t y) {
  return ((UID)(type & 0xF) << 12) | ((UID)(x & 0x3F) << 6) | (UID)(y & 0x3F);
}
EType uid_get_type(UID uid) { return (uid >> 12) & 0xF; }

Coords create_coords(double x, double y) {
  Coords c; c.x = x; c.y = y; return c;
}
uint8_t coords_distance(Coords* a, Coords* b) {
  double dx = a->x - b->x, dy = a->y - b->y;
  double d = sqrt(dx * dx + dy * dy);
  if (d > 255) d = 255;
  return (uint8_t)d;
}
Entity create_entity(uint8_t type, uint8_t x, uint8_t y, uint8_t initialState, uint8_t initialHealth) {
  Entity e;
  e.uid = create_uid(type, x, y);
  e.pos = create_coords((double)x + 0.5, (double)y + 0.5);
  e.state = initialState;
  e.health = initialHealth;
  e.distance = 0;
  e.timer = 0;
  return e;
}
StaticEntity create_static_entity(UID uid, uint8_t x, uint8_t y, bool active) {
  StaticEntity s; s.uid = uid; s.x = x; s.y = y; s.active = active; return s;
}

// ---- настройки ----
#define DOOM_MAX_ENEMIES    24
#define DOOM_MAX_ITEMS      12
#define DOOM_MAX_DOORS      24
#define DOOM_MAX_FIREBALLS   6
#define DOOM_MOVE_SPEED     2.6f
#define DOOM_ROT_SPEED      2.3f
#define DOOM_PLAYER_RADIUS  0.22f
#define DOOM_MAX_VIS (DOOM_MAX_ENEMIES + DOOM_MAX_ITEMS + DOOM_MAX_FIREBALLS)

// 26x26 значок для меню игр ("Без названия" стиль остальных иконок)
const unsigned char iconDoom[] PROGMEM = {
  0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x40,
  0x80, 0x3f, 0x00, 0x40, 0x80, 0x40, 0x80, 0x40, 0x80, 0x80, 0x40, 0x40, 0x81, 0x00, 0x20, 0x40,
  0x82, 0x00, 0x10, 0x40, 0x82, 0xe1, 0xd0, 0x40, 0x82, 0xe1, 0xd0, 0x40, 0x82, 0xe1, 0xd0, 0x40,
  0x82, 0x00, 0x10, 0x40, 0x81, 0x0c, 0x20, 0x40, 0x80, 0x80, 0x40, 0x40, 0x80, 0x40, 0x80, 0x40,
  0x80, 0x7f, 0x80, 0x40, 0x80, 0x55, 0x80, 0x40, 0x80, 0x55, 0x80, 0x40, 0x80, 0x55, 0x80, 0x40,
  0x80, 0x55, 0x80, 0x40, 0x80, 0x7f, 0x80, 0x40, 0x80, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80,
  0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00
};

Player doomPlayer;

Entity doomEnemies[DOOM_MAX_ENEMIES];
uint8_t doomEnemyCount = 0;
unsigned long doomEnemyShotAt[DOOM_MAX_ENEMIES];

StaticEntity doomItems[DOOM_MAX_ITEMS];
uint8_t doomItemCount = 0;

StaticEntity doomDoors[DOOM_MAX_DOORS];
uint8_t doomDoorCount = 0;

Entity doomFireballs[DOOM_MAX_FIREBALLS];
bool  doomFireballActive[DOOM_MAX_FIREBALLS];
float doomFireballDirX[DOOM_MAX_FIREBALLS];
float doomFireballDirY[DOOM_MAX_FIREBALLS];

int doomExitX = -1, doomExitY = -1;
bool doomActive = false;
int doomKills = 0;
int highDoom = 0;
unsigned long doomLastFrame = 0;
unsigned long doomLastShot = 0;
uint8_t doomMuzzleTimer = 0;
uint8_t doomZBuffer[SCREEN_WIDTH];

#define DSPR_ENEMY    0
#define DSPR_MEDIKIT  1
#define DSPR_KEY      2
#define DSPR_FIREBALL 3

struct DoomVis { float x, y, dist; uint8_t kind, frame; };
DoomVis doomVis[DOOM_MAX_VIS];
int doomVisCount = 0;

// ---------------- карта ----------------
uint8_t doomMapAt(int x, int y) {
  if (x < 0 || y < 0 || x >= LEVEL_WIDTH || y >= LEVEL_HEIGHT) return E_WALL;
  uint8_t b = pgm_read_byte(&sto_level_1[y * (LEVEL_WIDTH / 2) + x / 2]);
  return (x % 2 == 0) ? ((b >> 4) & 0xF) : (b & 0xF);
}

int doomFindDoor(int x, int y) {
  for (int i = 0; i < doomDoorCount; i++)
    if (doomDoors[i].x == x && doomDoors[i].y == y) return i;
  return -1;
}

bool doomIsSolid(int x, int y) {
  uint8_t t = doomMapAt(x, y);
  if (t == E_WALL) return true;
  if (t == E_DOOR || t == E_LOCKEDDOOR) {
    int d = doomFindDoor(x, y);
    if (d < 0) return true;
    return !doomDoors[d].active;   // active == открыта
  }
  return false;
}

bool doomCanMoveTo(float x, float y) {
  float r = DOOM_PLAYER_RADIUS;
  if (doomIsSolid((int)(x - r), (int)(y - r))) return false;
  if (doomIsSolid((int)(x + r), (int)(y - r))) return false;
  if (doomIsSolid((int)(x - r), (int)(y + r))) return false;
  if (doomIsSolid((int)(x + r), (int)(y + r))) return false;
  return true;
}

// ---------------- инициализация ----------------
void initDoom() {
  doomEnemyCount = 0; doomItemCount = 0; doomDoorCount = 0;
  doomExitX = -1; doomExitY = -1;
  for (int i = 0; i < DOOM_MAX_FIREBALLS; i++) doomFireballActive[i] = false;

  int px = 30, py = 46;
  for (int y = 0; y < LEVEL_HEIGHT; y++) {
    for (int x = 0; x < LEVEL_WIDTH; x++) {
      uint8_t t = doomMapAt(x, y);
      switch (t) {
        case E_PLAYER: px = x; py = y; break;
        case E_ENEMY:
          if (doomEnemyCount < DOOM_MAX_ENEMIES) {
            doomEnemies[doomEnemyCount] = create_enemy(x, y);
            doomEnemyShotAt[doomEnemyCount] = 0;
            doomEnemyCount++;
          }
          break;
        case E_MEDIKIT:
          if (doomItemCount < DOOM_MAX_ITEMS)
            doomItems[doomItemCount++] = create_static_entity(create_uid(E_MEDIKIT, x, y), x, y, true);
          break;
        case E_KEY:
          if (doomItemCount < DOOM_MAX_ITEMS)
            doomItems[doomItemCount++] = create_static_entity(create_uid(E_KEY, x, y), x, y, true);
          break;
        case E_DOOR:
        case E_LOCKEDDOOR:
          if (doomDoorCount < DOOM_MAX_DOORS)
            doomDoors[doomDoorCount++] = create_static_entity(create_uid(t, x, y), x, y, false);
          break;
        case E_EXIT:
          doomExitX = x; doomExitY = y; break;
      }
    }
  }

  doomPlayer = create_player(px, py);
  doomKills = 0;
  doomActive = true;
  doomLastFrame = millis();
  doomLastShot = 0;
  doomMuzzleTimer = 0;
}

// ---------------- логика ----------------
void doomRotate(float ang) {
  float cs = cosf(ang), sn = sinf(ang);
  float odx = doomPlayer.dir.x, ody = doomPlayer.dir.y;
  doomPlayer.dir.x = odx * cs - ody * sn;
  doomPlayer.dir.y = odx * sn + ody * cs;
  float opx = doomPlayer.plane.x, opy = doomPlayer.plane.y;
  doomPlayer.plane.x = opx * cs - opy * sn;
  doomPlayer.plane.y = opx * sn + opy * cs;
}

void doomMove(float amount) {
  float nx = doomPlayer.pos.x + doomPlayer.dir.x * amount;
  float ny = doomPlayer.pos.y + doomPlayer.dir.y * amount;
  if (doomCanMoveTo(nx, doomPlayer.pos.y)) doomPlayer.pos.x = nx;
  if (doomCanMoveTo(doomPlayer.pos.x, ny)) doomPlayer.pos.y = ny;
}

void doomOpenDoorAt(int x, int y) {
  int d = doomFindDoor(x, y);
  if (d < 0 || doomDoors[d].active) return;
  if (uid_get_type(doomDoors[d].uid) == E_LOCKEDDOOR) {
    if (doomPlayer.keys <= 0) return;
    doomPlayer.keys--;
  }
  doomDoors[d].active = true;
  playBeepSound();
}

void doomTryOpenNearbyDoors() {
  int px = (int)doomPlayer.pos.x, py = (int)doomPlayer.pos.y;
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
      doomOpenDoorAt(px + dx, py + dy);
}

void doomPickupItems() {
  for (int i = 0; i < doomItemCount; i++) {
    if (!doomItems[i].active) continue;
    float dx = (doomItems[i].x + 0.5f) - doomPlayer.pos.x;
    float dy = (doomItems[i].y + 0.5f) - doomPlayer.pos.y;
    if (dx * dx + dy * dy < 0.30f) {
      EType t = uid_get_type(doomItems[i].uid);
      if (t == E_MEDIKIT) {
        int h = doomPlayer.health + 25;
        doomPlayer.health = h > 100 ? 100 : h;
      } else if (t == E_KEY) {
        doomPlayer.keys++;
      }
      doomItems[i].active = false;
      playCoinSound();
    }
  }
}

void doomSpawnFireball(float x, float y, float dx, float dy) {
  for (int i = 0; i < DOOM_MAX_FIREBALLS; i++) {
    if (doomFireballActive[i]) continue;
    doomFireballs[i] = create_fireball((int)x, (int)y, 0);
    doomFireballs[i].pos = create_coords(x, y);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) len = 0.001f;
    doomFireballDirX[i] = dx / len;
    doomFireballDirY[i] = dy / len;
    doomFireballActive[i] = true;
    return;
  }
}

void doomUpdateEnemies(float dt) {
  for (int i = 0; i < doomEnemyCount; i++) {
    if (doomEnemies[i].state == S_DEAD) continue;
    float dx = doomPlayer.pos.x - doomEnemies[i].pos.x;
    float dy = doomPlayer.pos.y - doomEnemies[i].pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 9.0f) {
      doomEnemies[i].state = S_ALERT;
      if (dist > 1.3f) {
        float step = 0.7f * dt;
        float nx = doomEnemies[i].pos.x + (dx / dist) * step;
        float ny = doomEnemies[i].pos.y + (dy / dist) * step;
        if (!doomIsSolid((int)nx, (int)doomEnemies[i].pos.y)) doomEnemies[i].pos.x = nx;
        if (!doomIsSolid((int)doomEnemies[i].pos.x, (int)ny)) doomEnemies[i].pos.y = ny;
      }
      if (dist < 7.0f && millis() - doomEnemyShotAt[i] > 1400) {
        doomEnemyShotAt[i] = millis();
        doomSpawnFireball(doomEnemies[i].pos.x, doomEnemies[i].pos.y, dx, dy);
      }
    }
  }
}

void doomUpdateFireballs(float dt) {
  for (int i = 0; i < DOOM_MAX_FIREBALLS; i++) {
    if (!doomFireballActive[i]) continue;
    doomFireballs[i].pos.x += doomFireballDirX[i] * 3.0f * dt;
    doomFireballs[i].pos.y += doomFireballDirY[i] * 3.0f * dt;
    if (doomIsSolid((int)doomFireballs[i].pos.x, (int)doomFireballs[i].pos.y)) {
      doomFireballActive[i] = false; continue;
    }
    float dx = doomFireballs[i].pos.x - doomPlayer.pos.x;
    float dy = doomFireballs[i].pos.y - doomPlayer.pos.y;
    if (dx * dx + dy * dy < 0.30f) {
      doomFireballActive[i] = false;
      doomPlayer.health = (doomPlayer.health > 12) ? doomPlayer.health - 12 : 0;
      playErrorSound();
    }
  }
}

void doomShoot() {
  int bestIdx = -1; float bestDist = 1e9f;
  for (int i = 0; i < doomEnemyCount; i++) {
    if (doomEnemies[i].state == S_DEAD) continue;
    float dx = doomEnemies[i].pos.x - doomPlayer.pos.x;
    float dy = doomEnemies[i].pos.y - doomPlayer.pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 8.0f) continue;
    float dot = (dx * doomPlayer.dir.x + dy * doomPlayer.dir.y) / (dist + 0.001f);
    if (dot < 0.94f) continue;
    if (dist < bestDist) { bestDist = dist; bestIdx = i; }
  }
  doomMuzzleTimer = 4;
  playLaserSound();
  if (bestIdx >= 0) {
    if (doomEnemies[bestIdx].health > 34) {
      doomEnemies[bestIdx].health -= 34;
    } else {
      doomEnemies[bestIdx].health = 0;
      doomEnemies[bestIdx].state = S_DEAD;
      doomKills++;
      playExplosionSound();
    }
  }
}

// ---------------- рендер стен ----------------
void doomCastWalls() {
  for (int col = 0; col < SCREEN_WIDTH; col++) {
    float camX = 2.0f * col / (float)SCREEN_WIDTH - 1.0f;
    float rayDirX = doomPlayer.dir.x + doomPlayer.plane.x * camX;
    float rayDirY = doomPlayer.dir.y + doomPlayer.plane.y * camX;
    int mapX = (int)doomPlayer.pos.x;
    int mapY = (int)doomPlayer.pos.y;
    float deltaX = (rayDirX == 0) ? 1e6f : fabsf(1.0f / rayDirX);
    float deltaY = (rayDirY == 0) ? 1e6f : fabsf(1.0f / rayDirY);
    int stepX, stepY;
    float sideDistX, sideDistY;
    if (rayDirX < 0) { stepX = -1; sideDistX = ((float)doomPlayer.pos.x - mapX) * deltaX; }
    else { stepX = 1; sideDistX = (mapX + 1.0f - (float)doomPlayer.pos.x) * deltaX; }
    if (rayDirY < 0) { stepY = -1; sideDistY = ((float)doomPlayer.pos.y - mapY) * deltaY; }
    else { stepY = 1; sideDistY = (mapY + 1.0f - (float)doomPlayer.pos.y) * deltaY; }

    int side = 0; bool hit = false;
    for (int i = 0; i < 40 && !hit; i++) {
      if (sideDistX < sideDistY) { sideDistX += deltaX; mapX += stepX; side = 0; }
      else { sideDistY += deltaY; mapY += stepY; side = 1; }
      if (doomIsSolid(mapX, mapY)) hit = true;
    }
    float dist = side == 0 ? (sideDistX - deltaX) : (sideDistY - deltaY);
    if (dist < 0.08f) dist = 0.08f;
    doomZBuffer[col] = (uint8_t)(dist * 16.0f > 255 ? 255 : dist * 16.0f);

    int lineHeight = (int)(SCREEN_HEIGHT / dist);
    int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2; if (drawStart < 0) drawStart = 0;
    int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2; if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

    int shade = 7 - (int)(dist * 1.1f) - (side == 1 ? 1 : 0);
    if (shade < 0) shade = 0; if (shade > 7) shade = 7;

    for (int y = drawStart; y <= drawEnd; y++) {
      int rowInLevel = y & (GRADIENT_HEIGHT - 1);
      int gy = shade * GRADIENT_HEIGHT + rowInLevel;
      int bitIndex = col & 15;
      uint8_t gb = pgm_read_byte(&gradient[gy * GRADIENT_WIDTH + (bitIndex >> 3)]);
      bool on = (gb >> (7 - (bitIndex & 7))) & 1;
      if (on) display.drawPixel(col, y, whiteTheme ? BLACK : WHITE);
    }
  }
}

// ---------------- рендер спрайтов ----------------
void doomCollectSprites() {
  doomVisCount = 0;
  for (int i = 0; i < doomEnemyCount && doomVisCount < DOOM_MAX_VIS; i++) {
    if (doomEnemies[i].state == S_DEAD) continue;
    float dx = doomEnemies[i].pos.x - doomPlayer.pos.x;
    float dy = doomEnemies[i].pos.y - doomPlayer.pos.y;
    doomVis[doomVisCount].x = doomEnemies[i].pos.x;
    doomVis[doomVisCount].y = doomEnemies[i].pos.y;
    doomVis[doomVisCount].dist = dx * dx + dy * dy;
    doomVis[doomVisCount].kind = DSPR_ENEMY;
    doomVis[doomVisCount].frame = (doomEnemies[i].health < 34) ? 2 : (doomEnemies[i].state == S_ALERT ? 1 : 0);
    doomVisCount++;
  }
  for (int i = 0; i < doomItemCount && doomVisCount < DOOM_MAX_VIS; i++) {
    if (!doomItems[i].active) continue;
    float dx = (doomItems[i].x + 0.5f) - doomPlayer.pos.x;
    float dy = (doomItems[i].y + 0.5f) - doomPlayer.pos.y;
    doomVis[doomVisCount].x = doomItems[i].x + 0.5f;
    doomVis[doomVisCount].y = doomItems[i].y + 0.5f;
    doomVis[doomVisCount].dist = dx * dx + dy * dy;
    doomVis[doomVisCount].kind = (uid_get_type(doomItems[i].uid) == E_MEDIKIT) ? DSPR_MEDIKIT : DSPR_KEY;
    doomVis[doomVisCount].frame = 0;
    doomVisCount++;
  }
  for (int i = 0; i < DOOM_MAX_FIREBALLS && doomVisCount < DOOM_MAX_VIS; i++) {
    if (!doomFireballActive[i]) continue;
    float dx = doomFireballs[i].pos.x - doomPlayer.pos.x;
    float dy = doomFireballs[i].pos.y - doomPlayer.pos.y;
    doomVis[doomVisCount].x = doomFireballs[i].pos.x;
    doomVis[doomVisCount].y = doomFireballs[i].pos.y;
    doomVis[doomVisCount].dist = dx * dx + dy * dy;
    doomVis[doomVisCount].kind = DSPR_FIREBALL;
    doomVis[doomVisCount].frame = 0;
    doomVisCount++;
  }
  // сортировка вставками, дальние — первыми
  for (int i = 1; i < doomVisCount; i++) {
    DoomVis key = doomVis[i]; int j = i - 1;
    while (j >= 0 && doomVis[j].dist < key.dist) { doomVis[j + 1] = doomVis[j]; j--; }
    doomVis[j + 1] = key;
  }
}

void doomDrawBillboard(const DoomVis &s) {
  float dx = s.x - doomPlayer.pos.x, dy = s.y - doomPlayer.pos.y;
  float invDet = 1.0f / (doomPlayer.plane.x * doomPlayer.dir.y - doomPlayer.dir.x * doomPlayer.plane.y);
  float tx = invDet * (doomPlayer.dir.y * dx - doomPlayer.dir.x * dy);
  float ty = invDet * (-doomPlayer.plane.y * dx + doomPlayer.plane.x * dy);
  if (ty < 0.15f) return;

  int screenX = (int)((SCREEN_WIDTH / 2) * (1.0f + tx / ty));
  int size = (int)(SCREEN_HEIGHT / ty);
  if (size < 2 || size > 220) return;

  const uint8_t* bits; const uint8_t* mask; int srcW, srcH, frameBytes;
  switch (s.kind) {
    case DSPR_ENEMY:
      bits = bmp_imp_bits; mask = bmp_imp_mask;
      srcW = BMP_IMP_WIDTH; srcH = BMP_IMP_HEIGHT;
      frameBytes = (srcW / 8) * srcH * s.frame;
      break;
    case DSPR_MEDIKIT:
      bits = bmp_items_bits; mask = bmp_items_mask;
      srcW = BMP_ITEMS_WIDTH; srcH = BMP_ITEMS_HEIGHT;
      frameBytes = 0;
      break;
    case DSPR_KEY:
      bits = bmp_items_bits; mask = bmp_items_mask;
      srcW = BMP_ITEMS_WIDTH; srcH = BMP_ITEMS_HEIGHT;
      frameBytes = (srcW / 8) * srcH;
      break;
    default:
      bits = bmp_fireball_bits; mask = bmp_fireball_mask;
      srcW = BMP_FIREBALL_WIDTH; srcH = BMP_FIREBALL_HEIGHT;
      frameBytes = 0;
      break;
  }
  int rowBytes = srcW / 8;
  int x0 = screenX - size / 2, x1 = screenX + size / 2;
  int y0 = SCREEN_HEIGHT / 2 - size / 2, y1 = SCREEN_HEIGHT / 2 + size / 2;
  uint8_t zVal = (uint8_t)(ty * 16.0f > 255 ? 255 : ty * 16.0f);

  int xs = x0 < 0 ? 0 : x0;
  int xe = x1 >= SCREEN_WIDTH ? SCREEN_WIDTH - 1 : x1;
  int ys = y0 < 0 ? 0 : y0;
  int ye = y1 >= SCREEN_HEIGHT ? SCREEN_HEIGHT - 1 : y1;

  for (int px = xs; px <= xe; px++) {
    if (zVal >= doomZBuffer[px]) continue;
    int texX = (px - x0) * srcW / size;
    if (texX < 0 || texX >= srcW) continue;
    for (int py = ys; py <= ye; py++) {
      int texY = (py - y0) * srcH / size;
      if (texY < 0 || texY >= srcH) continue;
      int idx = frameBytes + texY * rowBytes + texX / 8;
      uint8_t mb = pgm_read_byte(&mask[idx]);
      if (!((mb >> (7 - (texX & 7))) & 1)) continue;
      uint8_t bb = pgm_read_byte(&bits[idx]);
      bool on = (bb >> (7 - (texX & 7))) & 1;
      display.drawPixel(px, py, (on != whiteTheme) ? WHITE : BLACK);
    }
  }
}

void doomDrawSprites() {
  doomCollectSprites();
  for (int i = 0; i < doomVisCount; i++) doomDrawBillboard(doomVis[i]);
}

// ---------------- оружие / HUD ----------------
void doomDrawGun() {
  int gx = SCREEN_WIDTH / 2 - BMP_GUN_WIDTH / 2;
  int gy = SCREEN_HEIGHT - BMP_GUN_HEIGHT + 6;
  int rowBytes = BMP_GUN_WIDTH / 8;
  for (int y = 0; y < BMP_GUN_HEIGHT; y++) {
    int sy = gy + y; if (sy < 0 || sy >= SCREEN_HEIGHT) continue;
    for (int x = 0; x < BMP_GUN_WIDTH; x++) {
      int sx = gx + x; if (sx < 0 || sx >= SCREEN_WIDTH) continue;
      int idx = y * rowBytes + x / 8;
      uint8_t mb = pgm_read_byte(&bmp_gun_mask[idx]);
      if (!((mb >> (7 - (x & 7))) & 1)) continue;
      uint8_t bb = pgm_read_byte(&bmp_gun_bits[idx]);
      bool on = (bb >> (7 - (x & 7))) & 1;
      display.drawPixel(sx, sy, (on != whiteTheme) ? WHITE : BLACK);
    }
  }
  if (doomMuzzleTimer > 0) {
    int fx = SCREEN_WIDTH / 2 - BMP_FIRE_WIDTH / 2;
    int fy = gy - BMP_FIRE_HEIGHT + 10;
    int frowBytes = BMP_FIRE_WIDTH / 8;
    for (int y = 0; y < BMP_FIRE_HEIGHT; y++) {
      int sy = fy + y; if (sy < 0 || sy >= SCREEN_HEIGHT) continue;
      for (int x = 0; x < BMP_FIRE_WIDTH; x++) {
        int sx = fx + x; if (sx < 0 || sx >= SCREEN_WIDTH) continue;
        uint8_t bb = pgm_read_byte(&bmp_fire_bits[y * frowBytes + x / 8]);
        if ((bb >> (7 - (x & 7))) & 1) display.drawPixel(sx, sy, whiteTheme ? BLACK : WHITE);
      }
    }
    doomMuzzleTimer--;
  }
}

void doomDrawHUD() {
  setDrawColor();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("HP:"); display.print(doomPlayer.health);
  display.setCursor(52, 0);
  display.print("KEY:"); display.print(doomPlayer.keys);
  display.setCursor(100, 0);
  display.print("K:"); display.print(doomKills);
}

// ---------------- главный цикл игры ----------------
void runDoom() {
  unsigned long now = millis();
  float dt = (now - doomLastFrame) / 1000.0f;
  if (dt > 0.12f) dt = 0.12f;
  if (dt < 0) dt = 0;
  doomLastFrame = now;

  if (digitalRead(BTN_LEFT) == LOW)  doomRotate(-DOOM_ROT_SPEED * dt);
  if (digitalRead(BTN_RIGHT) == LOW) doomRotate(DOOM_ROT_SPEED * dt);
  if (digitalRead(BTN_UP) == LOW)    doomMove(DOOM_MOVE_SPEED * dt);
  if (digitalRead(BTN_DOWN) == LOW)  doomMove(-DOOM_MOVE_SPEED * dt);
  if (digitalRead(BTN_SELECT) == LOW && millis() - doomLastShot > 350) {
    doomLastShot = millis();
    doomShoot();
  }

  doomTryOpenNearbyDoors();
  doomPickupItems();
  doomUpdateEnemies(dt);
  doomUpdateFireballs(dt);

  if (doomPlayer.health == 0) {
    gameOver("DOOM", doomKills, EE_DOOM, highDoom);
    return;
  }
  if (doomExitX >= 0) {
    float edx = (doomExitX + 0.5f) - doomPlayer.pos.x;
    float edy = (doomExitY + 0.5f) - doomPlayer.pos.y;
    if (edx * edx + edy * edy < 0.5f) {
      gameOver("DOOM", doomKills, EE_DOOM, highDoom);
      return;
    }
  }

  display.clearDisplay();
  if (whiteTheme) display.fillScreen(WHITE);
  doomCastWalls();
  doomDrawSprites();
  doomDrawGun();
  doomDrawHUD();
  display.display();
}

#endif
