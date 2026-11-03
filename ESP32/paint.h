#ifndef _paint_h
#define _paint_h

// ============================================================
// PAINT — рисовалка по Wi-Fi.
// ESP32 поднимает точку доступа (как в загрузке boot-логотипа) и веб-сервер.
// Страница на телефоне — клетчатое поле 128x64 (клетка = пиксель OLED).
// Всё, что нарисовано, сразу появляется на экране.
// Рисунки автоматически сохраняются на SD-карту в /pictures
// (pic_0001.bin, pic_0002.bin, ... — без ограничения количества).
// Формат файла: 1024 байта, 128x64, 1 бит на пиксель (строка = 16 байт, старший бит слева) —
// такой же, как у boot-логотипов, поэтому файл открывается в проводнике (BMP)
// и годится как boot-логотип.
//
// Кнопки на устройстве: SELECT — сохранить сейчас, UP — показать/скрыть данные Wi-Fi,
// BACK — выход в APPS.
//
// Подключается в скетче после filemanager.h (использует его монтирование SD-карты).
// ============================================================

#include <WebServer.h>
#include <SD.h>

#define PT_DIR   "/pictures"
#define PT_BYTES 1024

static const char PT_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>Рисовалка</title><style>
*{box-sizing:border-box}html,body{height:100%;margin:0}
body{background:#111;color:#eee;font:15px sans-serif;display:flex;flex-direction:column;overscroll-behavior:none}
#bar{background:#222;padding:6px;display:flex;flex-wrap:wrap;gap:6px;align-items:center}
button{background:#333;color:#eee;border:1px solid #555;border-radius:8px;padding:8px 11px;font-size:15px}
button.on{border-color:#2a7;box-shadow:0 0 0 2px #2a7}#t2.on{background:#2a7}
#st{font-size:12px;color:#9a9;flex:1;min-width:90px}
#wrap{flex:1;overflow:auto;-webkit-overflow-scrolling:touch}
#c{display:block;image-rendering:pixelated}
#gal{position:fixed;inset:0;background:#111;overflow:auto;display:none;padding:8px;z-index:9}
#gal .top{position:sticky;top:0;background:#111;padding-bottom:8px}
#grid{display:flex;flex-wrap:wrap;gap:8px}
.th{width:136px;background:#222;border-radius:8px;padding:4px;text-align:center;font-size:11px}
.th canvas{width:128px;height:64px;background:#000;image-rendering:pixelated;display:block;border:1px solid #444}
.th button{padding:4px 8px;font-size:13px;margin-top:3px}
</style></head><body>
<div id="bar">
<button id="t1" class="on">Рисовать</button><button id="t0">Стереть</button><button id="t2">Двигать</button>
<button id="zm">−</button><button id="zp">+</button>
<button id="cl">Очистить</button><button id="nw">Новый</button><button id="sv">Сохранить</button><button id="gb">Галерея</button>
<span id="st"></span></div>
<div id="wrap"><canvas id="c"></canvas></div>
<div id="gal"><div class="top"><button id="gc">← Назад</button> <span id="gi"></span></div><div id="grid"></div><p><button id="gm">Ещё</button></p></div>
<script>
const W=128,H=64,$=i=>document.getElementById(i);
let th=0,cs=10,tool=1,px=new Uint8Array(W*H),cv=$('c'),g=cv.getContext('2d'),q=[],busy=false,drawing=false,lx=0,ly=0;
const ink=()=>th?'#000':'#fff',bgc=()=>th?'#fff':'#000';
function applyTh(){$('t1').style.background=ink();$('t1').style.color=bgc();$('t0').style.background=bgc();$('t0').style.color=ink()}
function cell(x,y){g.fillStyle=px[y*W+x]?ink():bgc();g.fillRect(x*cs,y*cs,cs-1,cs-1)}
function all(){cv.width=W*cs;cv.height=H*cs;g.fillStyle=th?'#bbb':'#444';g.fillRect(0,0,cv.width,cv.height);for(let y=0;y<H;y++)for(let x=0;x<W;x++)cell(x,y)}
function put(x,y){if(x<0||y<0||x>=W||y>=H||px[y*W+x]==tool)return;px[y*W+x]=tool;cell(x,y);q.push(x+','+y+','+tool)}
function line(a,b,c,d){let dx=Math.abs(c-a),dy=-Math.abs(d-b),sx=a<c?1:-1,sy=b<d?1:-1,e=dx+dy;for(;;){put(a,b);if(a==c&&b==d)break;let e2=2*e;if(e2>=dy){e+=dy;a+=sx}if(e2<=dx){e+=dx;b+=sy}}}
function pos(e){const r=cv.getBoundingClientRect();return[Math.floor((e.clientX-r.left)/cs),Math.floor((e.clientY-r.top)/cs)]}
cv.onpointerdown=e=>{if(tool>1)return;drawing=true;cv.setPointerCapture(e.pointerId);[lx,ly]=pos(e);put(lx,ly);e.preventDefault()};
cv.onpointermove=e=>{if(!drawing)return;const[x,y]=pos(e);line(lx,ly,x,y);lx=x;ly=y;e.preventDefault()};
cv.onpointerup=cv.onpointercancel=()=>{drawing=false};
async function flush(){if(busy||!q.length)return;busy=true;const b=q;q=[];try{await fetch('/px',{method:'POST',body:b.join(';')})}catch(e){q=b.concat(q)}busy=false}
setInterval(flush,60);
function setTool(t){tool=t;for(let i=0;i<3;i++)$('t'+i).classList.toggle('on',i==t);cv.style.touchAction=t>1?'auto':'none'}
$('t1').onclick=()=>setTool(1);$('t0').onclick=()=>setTool(0);$('t2').onclick=()=>setTool(2);
function zoom(d){cs=Math.max(4,Math.min(24,cs+d));all()}
$('zm').onclick=()=>zoom(-2);$('zp').onclick=()=>zoom(2);
async function post(u){await flush();while(busy)await new Promise(r=>setTimeout(r,20));try{return await (await fetch(u,{method:'POST'})).text()}catch(e){return 'ERR'}}
async function load(){try{const r=await fetch('/state'),b=new Uint8Array(await r.arrayBuffer());for(let y=0;y<H;y++)for(let x=0;x<W;x++)px[y*W+x]=(b[y*16+(x>>3)]>>(7-(x&7)))&1;all()}catch(e){}}
async function info(){try{const t=(await (await fetch('/info')).text()).split('|');if(+t[3]!=th&&t.length>3){th=+t[3];applyTh();all()}$('st').textContent=(t[1]=='1'?(t[2]=='1'?'Сохраняю…':'Сохранено: '+(t[0]||'—')):'Нет SD — не сохраняется')}catch(e){$('st').textContent='Нет связи'}}
setInterval(info,1500);
$('cl').onclick=async()=>{if(confirm('Очистить поле?')){px.fill(0);all();await post('/clear')}};
$('nw').onclick=async()=>{await post('/new');px.fill(0);all();info()};
$('sv').onclick=async()=>{await post('/save');info()};
let off=0,loading=false;
async function thumbs(names){for(const n of names){const d=document.createElement('div');d.className='th';const c=document.createElement('canvas');c.width=W;c.height=H;const o=document.createElement('button');o.textContent='Открыть';const x=document.createElement('button');x.textContent='Удалить';d.append(c,n,document.createElement('br'),o,x);$('grid').append(d);
o.onclick=async()=>{await post('/open?f='+encodeURIComponent(n));await load();$('gal').style.display='none';info()};
x.onclick=async()=>{if(confirm('Удалить '+n+'?')){await post('/del?f='+encodeURIComponent(n));d.remove()}};
try{const b=new Uint8Array(await (await fetch('/pic?f='+encodeURIComponent(n))).arrayBuffer()),cx=c.getContext('2d'),im=cx.createImageData(W,H);for(let y=0;y<H;y++)for(let xx=0;xx<W;xx++){const v=((b[y*16+(xx>>3)]>>(7-(xx&7)))&1)^th?255:0,i=(y*W+xx)*4;im.data[i]=im.data[i+1]=im.data[i+2]=v;im.data[i+3]=255}cx.putImageData(im,0,0)}catch(e){}}}
async function more(){if(loading)return;loading=true;try{const r=await fetch('/list?off='+off+'&n=24'),t=(await r.text()).split('\n').filter(s=>s);off+=t.length;$('gm').style.display=r.headers.get('X-More')=='1'?'':'none';$('gi').textContent=off?'':'Рисунков пока нет';await thumbs(t)}catch(e){}loading=false}
$('gb').onclick=async()=>{await flush();$('grid').innerHTML='';off=0;$('gal').style.display='block';more()};
$('gm').onclick=more;$('gc').onclick=()=>{$('gal').style.display='none'};
setTool(1);applyTh();info().then(load);
</script></body></html>
)rawliteral";

static WebServer ptServer(80);
static uint8_t ptBuf[PT_BYTES];
static bool ptRoutes = false, ptRunning = false, ptAp = false;
static bool ptSdOk = false;
static char ptName[32] = "";                 // текущий файл (пусто = ещё не сохранён)
static int  ptNextNum = 1;                   // номер для следующего нового рисунка
static bool ptDirty = false;                 // есть несохранённые правки
static bool ptRedraw = true, ptShowInfo = true;
static unsigned long ptNextSaveAt = 0, ptLastDraw = 0, ptMsgUntil = 0;
static const char* ptMsg = "";

// ---------- вспомогательное ----------
static bool ptValidName(const char* n) {      // только простые имена *.bin, без папок
  size_t l = strlen(n);
  if (l < 5 || l > 31 || n[0] == '.') return false;
  for (size_t i = 0; i < l; i++) {
    char c = n[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-';
    if (!ok) return false;
  }
  return strcasecmp(n + l - 4, ".bin") == 0;
}

static bool ptIsBlank() {
  for (int i = 0; i < PT_BYTES; i++) if (ptBuf[i]) return false;
  return true;
}

static void ptMakePath(char* out, size_t n, const char* name) {
  snprintf(out, n, "%s/%s", PT_DIR, name);
}

static void ptScanNext() {                    // ищем максимальный номер pic_NNNN.bin
  ptNextNum = 1;
  File dir = SD.open(PT_DIR);
  if (!dir) return;
  if (dir.isDirectory()) {
    File f;
    while ((f = dir.openNextFile())) {
      const char* nm = strrchr(f.name(), '/');
      nm = nm ? nm + 1 : f.name();
      if (strncmp(nm, "pic_", 4) == 0) {
        int num = atoi(nm + 4);
        if (num >= ptNextNum) ptNextNum = num + 1;
      }
      f.close();
    }
  }
  dir.close();
}

static bool ptSave() {
  if (!ptSdOk) return false;
  if (!ptName[0] && ptIsBlank()) { ptDirty = false; return true; }   // пустой новый рисунок не сохраняем
  char nm[32];
  if (ptName[0]) strcpy(nm, ptName);
  else {
    // следующий свободный номер (на случай, если файлы добавляли вручную)
    char path[64];
    for (int guard = 0; guard < 50; guard++) {
      snprintf(nm, sizeof(nm), "pic_%04d.bin", ptNextNum);
      ptMakePath(path, sizeof(path), nm);
      if (!SD.exists(path)) break;
      ptNextNum++;
    }
  }
  char path[64];
  ptMakePath(path, sizeof(path), nm);
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;
  size_t w = f.write(ptBuf, PT_BYTES);
  f.close();
  if (w != PT_BYTES) return false;
  if (!ptName[0]) { strcpy(ptName, nm); ptNextNum++; }
  ptDirty = false;
  return true;
}

// ---------- HTTP ----------
static void ptTouchClient() {                  // страница открыта — показываем рисунок на экране
  if (ptShowInfo) { ptShowInfo = false; ptRedraw = true; }
}

static void ptHandleRoot() {
  ptTouchClient();
  ptServer.send_P(200, "text/html; charset=utf-8", PT_PAGE);
}

static void ptHandleState() {
  ptTouchClient();
  ptServer.sendHeader("Cache-Control", "no-store");
  ptServer.setContentLength(PT_BYTES);
  ptServer.send(200, "application/octet-stream", "");
  ptServer.sendContent((const char*)ptBuf, PT_BYTES);
}

static void ptHandleInfo() {
  String s = ptName;
  s += '|'; s += ptSdOk ? '1' : '0';
  s += '|'; s += ptDirty ? '1' : '0';
  s += '|'; s += whiteTheme ? '1' : '0';       // тема: страница рисует холст в тех же цветах
  ptServer.sendHeader("Cache-Control", "no-store");
  ptServer.send(200, "text/plain", s);
}

static void ptHandlePx() {                     // тело: "x,y,v;x,y,v;..."
  const String& b = ptServer.arg("plain");
  const char* p = b.c_str();
  bool any = false;
  while (*p) {
    char* e;
    long x = strtol(p, &e, 10); if (e == p || *e != ',') break; p = e + 1;
    long y = strtol(p, &e, 10); if (e == p || *e != ',') break; p = e + 1;
    long v = strtol(p, &e, 10); if (e == p) break; p = e;
    if (*p == ';') p++;
    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
      uint8_t m = 0x80 >> (x & 7);
      uint8_t& cell = ptBuf[y * 16 + (x >> 3)];
      if (v) cell |= m; else cell &= ~m;
      any = true;
    }
  }
  if (any) { ptDirty = true; ptRedraw = true; ptNextSaveAt = millis() + 1500; }
  ptServer.send(200, "text/plain", "ok");
}

static void ptHandleClear() {
  memset(ptBuf, 0, PT_BYTES);
  ptDirty = true; ptRedraw = true; ptNextSaveAt = millis() + 1500;
  ptServer.send(200, "text/plain", "ok");
}

static void ptHandleNew() {                    // сохранить текущий и начать чистый рисунок
  if (ptDirty) ptSave();
  memset(ptBuf, 0, PT_BYTES);
  ptName[0] = 0; ptDirty = false; ptRedraw = true;
  ptServer.send(200, "text/plain", "ok");
}

static void ptHandleSave() {
  bool ok = ptSave();
  if (ok) { ptMsg = "SAVED"; ptMsgUntil = millis() + 1200; ptRedraw = true; }
  ptServer.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "ERR");
}

static void ptHandleList() {                   // ?off=N&n=K — имена *.bin размером 1024 байта
  String out;
  bool more = false;
  if (ptSdOk) {
    int off = ptServer.arg("off").toInt();
    int n = ptServer.arg("n").toInt();
    if (off < 0) off = 0;
    if (n <= 0 || n > 60) n = 24;
    File dir = SD.open(PT_DIR);
    if (dir && dir.isDirectory()) {
      int idx = 0, got = 0;
      File f;
      while ((f = dir.openNextFile())) {
        const char* nm = strrchr(f.name(), '/');
        nm = nm ? nm + 1 : f.name();
        if (!f.isDirectory() && f.size() == PT_BYTES && ptValidName(nm)) {
          if (idx >= off) {
            if (got < n) { out += nm; out += '\n'; got++; }
            else { more = true; f.close(); break; }
          }
          idx++;
        }
        f.close();
      }
    }
    if (dir) dir.close();
  }
  ptServer.sendHeader("X-More", more ? "1" : "0");
  ptServer.send(200, "text/plain", out);
}

static void ptHandlePic() {                    // ?f=NAME — сырые 1024 байта (для превью)
  String f = ptServer.arg("f");
  char path[64];
  if (!ptSdOk || !ptValidName(f.c_str())) { ptServer.send(400, "text/plain", "bad"); return; }
  ptMakePath(path, sizeof(path), f.c_str());
  File fl = SD.open(path);
  if (!fl || fl.size() != PT_BYTES) { if (fl) fl.close(); ptServer.send(404, "text/plain", "none"); return; }
  uint8_t tmp[PT_BYTES];
  fl.read(tmp, PT_BYTES);
  fl.close();
  ptServer.setContentLength(PT_BYTES);
  ptServer.send(200, "application/octet-stream", "");
  ptServer.sendContent((const char*)tmp, PT_BYTES);
}

static void ptHandleOpen() {                   // ?f=NAME — загрузить рисунок в редактор
  String f = ptServer.arg("f");
  char path[64];
  if (!ptSdOk || !ptValidName(f.c_str())) { ptServer.send(400, "text/plain", "bad"); return; }
  if (ptDirty) ptSave();
  ptMakePath(path, sizeof(path), f.c_str());
  File fl = SD.open(path);
  if (!fl || fl.size() != PT_BYTES) { if (fl) fl.close(); ptServer.send(404, "text/plain", "none"); return; }
  fl.read(ptBuf, PT_BYTES);
  fl.close();
  strncpy(ptName, f.c_str(), sizeof(ptName) - 1);
  ptName[sizeof(ptName) - 1] = 0;
  ptDirty = false; ptRedraw = true;
  ptServer.send(200, "text/plain", "ok");
}

static void ptHandleDel() {                    // ?f=NAME
  String f = ptServer.arg("f");
  char path[64];
  if (!ptSdOk || !ptValidName(f.c_str())) { ptServer.send(400, "text/plain", "bad"); return; }
  ptMakePath(path, sizeof(path), f.c_str());
  bool ok = SD.remove(path);
  if (ok && strcmp(f.c_str(), ptName) == 0) { ptName[0] = 0; ptDirty = false; }   // дальше рисунок будет новым
  ptServer.send(ok ? 200 : 500, "text/plain", ok ? "ok" : "err");
}

// ---------- Wi-Fi / сервер ----------
static void ptStartServer() {
  if (ptRunning) return;
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAPdisconnect(true);
  delay(100);
  ptAp = WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  if (!ptAp) {
    WiFi.mode(WIFI_AP);
    delay(100);
    ptAp = WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  }
  if (!ptRoutes) {
    ptServer.on("/", HTTP_GET, ptHandleRoot);
    ptServer.on("/state", HTTP_GET, ptHandleState);
    ptServer.on("/info", HTTP_GET, ptHandleInfo);
    ptServer.on("/px", HTTP_POST, ptHandlePx);
    ptServer.on("/clear", HTTP_POST, ptHandleClear);
    ptServer.on("/new", HTTP_POST, ptHandleNew);
    ptServer.on("/save", HTTP_POST, ptHandleSave);
    ptServer.on("/list", HTTP_GET, ptHandleList);
    ptServer.on("/pic", HTTP_GET, ptHandlePic);
    ptServer.on("/open", HTTP_POST, ptHandleOpen);
    ptServer.on("/del", HTTP_POST, ptHandleDel);
    ptRoutes = true;
  }
  ptServer.begin();
  ptRunning = true;
}

static void ptStopServer() {
  if (!ptRunning) return;
  ptServer.stop();
  ptRunning = false;
  if (ptAp) {
    WiFi.softAPdisconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    startSavedWifi();
    ptAp = false;
  }
}

// ---------- экран ----------
// Цвета по теме: тёмная — белое на чёрном, светлая — чёрное на белом
static inline uint16_t ptFg() { return whiteTheme ? BLACK : WHITE; }
static inline uint16_t ptBg() { return whiteTheme ? WHITE : BLACK; }

static void ptDrawInfo() {
  uint16_t fg = ptFg();
  display.clearDisplay();
  display.fillRect(0, 0, 128, 64, ptBg());
  drawRect(0, 0, 128, 10, true);
  setInverseColor();
  display.setCursor(2, 0);
  display.print("PAINT  ");
  display.print(ptSdOk ? "SD-OK" : "NO SD");
  setDrawColor();
  display.setCursor(2, 13); display.print("WiFi: "); display.print(BL_AP_SSID);
  display.setCursor(2, 22); display.print("Pass: "); display.print(BL_AP_PASS);
  display.setCursor(2, 31); display.print("http://"); display.print(WiFi.softAPIP());
  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(2, 40); display.print("LAN "); display.print(WiFi.localIP());
  }
  display.setCursor(2, 56); display.print("SEL:SAVE UP:HIDE BACK");
  display.display();
}

static void ptDrawCanvas() {
  uint16_t fg = ptFg(), bg = ptBg();
  display.clearDisplay();
  display.fillRect(0, 0, 128, 64, bg);              // холст: белый в светлой теме, чёрный в тёмной
  display.drawBitmap(0, 0, ptBuf, 128, 64, fg);     // нарисованное — цветом текста темы
  if (millis() < ptMsgUntil) {                      // короткая плашка поверх рисунка
    int w = strlen(ptMsg) * 6 + 6;
    display.fillRect(0, 55, w, 9, bg);
    display.drawRect(0, 55, w, 9, fg);
    display.setTextSize(1);
    display.setTextColor(fg);
    display.setCursor(3, 56);
    display.print(ptMsg);
  }
  display.display();
}

// ---------- точки входа ----------
void initPaint() {
  memset(ptBuf, 0, PT_BYTES);
  ptName[0] = 0;
  ptDirty = false;
  ptShowInfo = true;
  ptRedraw = true;
  ptMsgUntil = 0;
  ptSdOk = fmSdMount();
  if (ptSdOk) {
    fmMkdirs(SD, PT_DIR);
    ptScanNext();
  }
  ptStartServer();
}

void paintExit() {                             // вызывается из changeState() при выходе
  if (ptDirty) ptSave();
  ptStopServer();
  fmSdUnmount();
  ptSdOk = false;
}

void runPaint() {
  ptServer.handleClient();
  unsigned long now = millis();

  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_BACK) == LOW) {
      lastButtonPress = now;
      changeState(APPS_MENU);                  // paintExit() сохранит и закроет всё
      return;
    }
    if (digitalRead(BTN_UP) == LOW) {
      lastButtonPress = now;
      ptShowInfo = !ptShowInfo;
      ptRedraw = true;
    }
    if (digitalRead(BTN_SELECT) == LOW) {
      lastButtonPress = now;
      if (!ptSdOk) {                           // карту могли вставить позже
        ptSdOk = fmSdMount();
        if (ptSdOk) { fmMkdirs(SD, PT_DIR); ptScanNext(); }
      }
      if (!ptSdOk) ptMsg = "NO SD";
      else if (ptSave()) ptMsg = "SAVED";
      else ptMsg = "SAVE ERR";
      ptMsgUntil = now + 1200;
      ptRedraw = true;
    }
  }

  // автосохранение через 1.5 с после последней правки
  if (ptDirty && ptSdOk && (long)(now - ptNextSaveAt) >= 0) {
    if (ptSave()) { ptMsg = "SAVED"; ptMsgUntil = now + 800; }
    else { ptNextSaveAt = now + 5000; ptMsg = "SAVE ERR"; ptMsgUntil = now + 1200; }
    ptRedraw = true;
  }

  if (ptMsgUntil && now >= ptMsgUntil) { ptMsgUntil = 0; ptRedraw = true; }

  if (ptRedraw && now - ptLastDraw >= 30) {
    ptRedraw = false;
    ptLastDraw = now;
    if (ptShowInfo) ptDrawInfo(); else ptDrawCanvas();
  }
}

#endif
