#ifndef _webremote_h
#define _webremote_h

// ============================================================
// WEB REMOTE — веб-управление устройством.
// Команда в CMD:  snldo web      (включить)   /   snldo web off  (выключить)
//
// ESP32 поднимает точку доступа SNL-GAMING (как в PAINT / FILES) и веб-сервер
// на порту 8080:   http://192.168.4.1:8080
// (если устройство подключено к домашнему Wi-Fi — адрес LAN показывает команда).
//
// Страница показывает экран OLED в реальном времени (128x64), а внизу —
// кнопки UP / DOWN / LEFT / RIGHT / SELECT / BACK. Кнопки можно удерживать
// (для игр), на компьютере работают стрелки/WASD, Enter, Esc.
// Ещё есть строка команд (то же, что CMD на устройстве) и скриншот.
//
// Сервер работает в фоне во всех экранах, поэтому видно и меню, и игры.
// Порт 8080 выбран, чтобы не мешать PAINT и проводнику (они на порту 80).
// Виртуальные кнопки подмешиваются в digitalRead() — см. wrDigitalRead() в скетче.
//
// Подключается в скетче после paint.h.
// ============================================================

// Всё это объявлено/определено в скетче
String executeCmd(String cmd);
void changeState(int state);
void startSavedWifi();

extern bool wrHeld[6];
extern bool wrSeen[6];
extern bool wrRel[6];
extern uint32_t wrMinAt[6];
extern uint32_t wrMaxAt[6];

#define WR_PORT 8080

// Иконка пункта WEB в Настройках (26x26)
const unsigned char iconWeb[] PROGMEM = {
0x1f, 0xff, 0xfe, 0x00, 0x20, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x40, 
 0x80, 0x00, 0x00, 0x40, 0x83, 0xff, 0xf0, 0x40, 0x86, 0xaa, 0xa8, 0x40, 0x85, 0x55, 0x58, 0x40, 
 0x86, 0xaa, 0xa8, 0x40, 0x85, 0x55, 0x58, 0x40, 0x86, 0xaa, 0xa8, 0x40, 0x85, 0x55, 0x58, 0x40, 
 0x86, 0xaa, 0xa8, 0x40, 0xbf, 0xff, 0x58, 0x40, 0xa0, 0x01, 0xa8, 0x40, 0xa4, 0xfd, 0x58, 0x40, 
 0xa8, 0x85, 0xf0, 0x40, 0xa2, 0x85, 0x00, 0x40, 0xa4, 0xfd, 0x00, 0x40, 0xa0, 0x01, 0xfe, 0x40, 
 0xbf, 0xff, 0xfe, 0x40, 0x9f, 0xff, 0xfe, 0x40, 0x80, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 
 0x20, 0x00, 0x01, 0x00, 0x1f, 0xff, 0xfe, 0x00
};

static const char WR_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>SNL GAMING — пульт</title><style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;min-height:100%}
body{background:#111;color:#eee;font:15px sans-serif;display:flex;flex-direction:column;align-items:center;padding:8px;gap:10px;overscroll-behavior:none}
#top{width:100%;max-width:640px;display:flex;align-items:center;gap:8px;font-size:13px;color:#9a9}
.dot{width:10px;height:10px;border-radius:50%;background:#a33;flex:none}.dot.ok{background:#2a7}
#st{flex:1;overflow:hidden;white-space:nowrap;text-overflow:ellipsis}
#scr{width:100%;max-width:640px;background:#000;border:3px solid #333;border-radius:10px;padding:0;line-height:0}
#c{width:100%;aspect-ratio:2/1;image-rendering:pixelated;image-rendering:crisp-edges;display:block}
#tools{width:100%;max-width:640px;display:flex;gap:6px;justify-content:flex-end}
button{background:#333;color:#eee;border:1px solid #555;border-radius:10px;font-size:15px}
#tools button{padding:6px 10px;font-size:13px}
#pad{width:100%;max-width:420px;display:grid;grid-template-columns:repeat(4,1fr);grid-template-areas:". u . b" "l o r ." ". d . .";gap:8px;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none}
.k{height:62px;font-size:20px;touch-action:none}
.k.d{background:#2a7;border-color:#2a7;color:#fff}
.k.ok{font-size:16px;font-weight:bold}.k.bk{font-size:14px}
#cmd{width:100%;max-width:640px;display:flex;gap:6px}
#ci{flex:1;min-width:0;background:#222;color:#eee;border:1px solid #555;border-radius:10px;padding:9px;font-size:15px}
#cmd button{padding:9px 12px}
#co{width:100%;max-width:640px;margin:0;background:#1a1a1a;border-radius:10px;padding:8px;font:13px monospace;white-space:pre-wrap;min-height:20px;color:#bdb}
</style></head><body>
<div id="top"><span id="on" class="dot"></span><span id="st">подключение…</span><span id="fps"></span></div>
<div id="scr"><canvas id="c" width="128" height="64"></canvas></div>
<div id="tools"><button id="col">Цвет</button><button id="shot">Скриншот</button></div>
<div id="pad">
<button class="k" data-b="up" style="grid-area:u">▲</button>
<button class="k bk" data-b="back" style="grid-area:b">BACK</button>
<button class="k" data-b="left" style="grid-area:l">◄</button>
<button class="k ok" data-b="select" style="grid-area:o">OK</button>
<button class="k" data-b="right" style="grid-area:r">►</button>
<button class="k" data-b="down" style="grid-area:d">▼</button>
</div>
<div id="cmd"><input id="ci" value="snldo " autocapitalize="none" autocomplete="off" spellcheck="false"><button id="go">Выполнить</button></div>
<pre id="co"></pre>
<script>
const $=i=>document.getElementById(i);
const cv=$('c'),g=cv.getContext('2d'),im=g.createImageData(128,64);
const COL=[[255,255,255],[60,255,140],[255,176,0]];
let ci=0,inv=0,fr=0,last=null;
try{ci=(+localStorage.getItem('wc'))||0}catch(e){}
function draw(b){
  const f=COL[ci],d=im.data;
  for(let y=0;y<64;y++){
    const row=(y>>3)*128,bit=y&7;
    for(let x=0;x<128;x++){
      const on=((b[row+x]>>bit)&1)^inv,i=(y*128+x)*4;
      if(on){d[i]=f[0];d[i+1]=f[1];d[i+2]=f[2]}else{d[i]=d[i+1]=d[i+2]=0}
      d[i+3]=255;
    }
  }
  g.putImageData(im,0,0);
}
async function loop(){
  let dl=30;
  try{
    const r=await fetch('/fb',{cache:'no-store'});
    if(!r.ok)throw 0;
    const b=new Uint8Array(await r.arrayBuffer());
    if(b.length==1024){
      inv=r.headers.get('X-Inv')=='1'?1:0;
      last=b;draw(b);fr++;
      $('st').textContent=(r.headers.get('X-State')||'')+' · '+Math.round((+r.headers.get('X-Heap')||0)/1024)+' KB';
      $('on').className='dot ok';
    }
  }catch(e){$('on').className='dot';$('st').textContent='нет связи';dl=700}
  setTimeout(loop,dl);
}
setInterval(()=>{$('fps').textContent=fr+' fps';fr=0},1000);
loop();
const held={};
function send(b,s){fetch('/btn?b='+b+'&s='+s,{cache:'no-store',keepalive:true}).catch(()=>{})}
function press(b,el){if(held[b])return;held[b]=setInterval(()=>send(b,1),250);send(b,1);if(el)el.classList.add('d')}
function rel(b,el){if(!held[b])return;clearInterval(held[b]);delete held[b];send(b,0);if(el)el.classList.remove('d')}
document.querySelectorAll('.k').forEach(el=>{
  const b=el.dataset.b;
  el.onpointerdown=e=>{e.preventDefault();try{el.setPointerCapture(e.pointerId)}catch(x){}press(b,el)};
  el.onpointerup=el.onpointercancel=()=>rel(b,el);
  el.oncontextmenu=e=>e.preventDefault();
});
const KM={arrowup:'up',w:'up',arrowdown:'down',s:'down',arrowleft:'left',a:'left',arrowright:'right',d:'right',enter:'select',' ':'select',escape:'back',backspace:'back'};
function elOf(b){return document.querySelector('[data-b="'+b+'"]')}
addEventListener('keydown',e=>{
  if(e.target.tagName=='INPUT')return;
  const b=KM[e.key.toLowerCase()];
  if(b){e.preventDefault();if(!e.repeat)press(b,elOf(b))}
});
addEventListener('keyup',e=>{
  const b=KM[e.key.toLowerCase()];
  if(b)rel(b,elOf(b));
});
function relAll(){Object.keys(held).forEach(b=>rel(b,elOf(b)))}
addEventListener('blur',relAll);
document.addEventListener('visibilitychange',()=>{if(document.hidden)relAll()});
$('col').onclick=()=>{ci=(ci+1)%COL.length;try{localStorage.setItem('wc',ci)}catch(e){}if(last)draw(last)};
$('shot').onclick=()=>{
  if(!last)return;
  const c=document.createElement('canvas');c.width=1024;c.height=512;
  const x=c.getContext('2d');x.imageSmoothingEnabled=false;x.drawImage(cv,0,0,1024,512);
  c.toBlob(b=>{const a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='snl-screen.png';a.click()});
};
async function run(){
  const v=$('ci').value.trim();
  if(!v)return;
  $('co').textContent='…';
  try{
    const r=await fetch('/cmd?c='+encodeURIComponent(v),{cache:'no-store'});
    $('co').textContent=(await r.text())||'(ok)';
  }catch(e){$('co').textContent='нет связи'}
}
$('go').onclick=run;
$('ci').onkeydown=e=>{if(e.key=='Enter')run()};
</script></body></html>
)rawliteral";

static WebServer wrSrv(WR_PORT);
static bool wrEnabled = false, wrRoutes = false, wrServerUp = false;
static bool wrInverted = false;                 // ставится командой invert — страница инвертирует картинку так же
static unsigned long wrNextCheck = 0;

static int wrBtnIndex(const String& n) {
  if (n == "up") return 0;
  if (n == "down") return 1;
  if (n == "left") return 2;
  if (n == "right") return 3;
  if (n == "select") return 4;
  if (n == "back") return 5;
  return -1;
}

static const char* wrStateName() {
  switch (currentState) {
    case STARTUP: return "Boot";
    case MAIN_MENU: return "Main menu";
    case GAMES_MENU: return "Games";
    case SETTINGS_MENU: return "Settings";
    case APPS_MENU: return "Apps";
    case UTILS_MENU: return "Utilities";
    case FUN_MENU: return "Entertainment";
    case INTERFACE_MENU: return "Interface";
    case SNAKE: return "Snake";
    case PONG: return "Pong";
    case CAR: return "Racer";
    case DINO: return "Dino";
    case SHOOTER: return "Shooter";
    case STOPWATCH: return "Stopwatch";
    case ABOUT: return "About";
    case SHOW_RECORDS: return "Records";
    case THEME_SETTINGS: return "Theme";
    case FLAPPY: return "Flappy";
    case ARKANOID: return "Arkanoid";
    case TANKS: return "Tanks";
    case RANDOM_GEN: return "Random";
    case CODE_VIEWER: return "Code";
    case PET_MENU: return "Pet";
    case PET_FEED: return "Pet feed";
    case FLASHLIGHT: return "Flashlight";
    case CALCULATOR: return "Calculator";
    case CMD_LINE: return "CMD";
    case CMD_RESULT: return "CMD result";
    case SOUND_PAD: return "Sound pad";
    case BOOT_SPEED_SETTINGS: return "Boot speed";
    case MP3_PLAYER: return "MP3";
    case RENDER_3D: return "3D";
    case TETRIS: return "Tetris";
    case DOOM: return "Doom";
    case CLOCK: return "Clock";
    case WEATHER_APP: return "Weather";
    case WIFI_SETUP: return "Wi-Fi";
    case JOYSTICK_APP: return "Joystick";
    case BOOT_LOGO_SETTINGS: return "Boot logo";
    case FILE_MANAGER: return "Files";
    case FREERIDE: return "Freeride";
    case TOWER: return "Tower";
    case BRIGHTNESS_SETTINGS: return "Brightness";
    case PAINT: return "Paint";
    case WEB_REMOTE: return "Web remote";
    default: return "?";
  }
}

// ---------- HTTP ----------
static void wrHandleRoot() {
  wrSrv.send_P(200, "text/html; charset=utf-8", WR_PAGE);
}

static void wrHandleFb() {                      // 1024 байта буфера SSD1306 (страницы по 8 пикселей по вертикали)
  wrSrv.sendHeader("Cache-Control", "no-store");
  wrSrv.sendHeader("X-State", wrStateName());
  wrSrv.sendHeader("X-Inv", wrInverted ? "1" : "0");
  wrSrv.sendHeader("X-Heap", String(ESP.getFreeHeap()));
  wrSrv.setContentLength(1024);
  wrSrv.send(200, "application/octet-stream", "");
  wrSrv.sendContent((const char*)display.getBuffer(), 1024);
}

static void wrHandleBtn() {                     // ?b=up|down|left|right|select|back&s=1|0
  int i = wrBtnIndex(wrSrv.arg("b"));
  if (i < 0) { wrSrv.send(400, "text/plain", "bad"); return; }
  uint32_t now = millis();
  if (wrSrv.arg("s").toInt()) {
    if (!wrHeld[i]) { wrHeld[i] = true; wrSeen[i] = false; wrMinAt[i] = now + 70; }
    wrRel[i] = false;
    wrMaxAt[i] = now + 700;                     // страница шлёт s=1 каждые 250 мс, пока кнопка зажата
  } else if (wrHeld[i]) {
    wrRel[i] = true;                            // отпустим, когда прошивка хотя бы раз увидит нажатие
  }
  wrSrv.send(200, "text/plain", "ok");
}

static void wrHandleCmd() {                     // ?c=snldo help
  String out = executeCmd(wrSrv.arg("c"));
  wrSrv.sendHeader("Cache-Control", "no-store");
  wrSrv.send(200, "text/plain; charset=utf-8", out);
}

// ---------- Wi-Fi / сервер ----------
static void wrApUp() {
  if (!(WiFi.getMode() & WIFI_MODE_AP)) {
    WiFi.mode(WIFI_AP_STA);                     // STA-подключение к домашней сети сохраняется
    delay(50);
  }
  bool ok = WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  if (!ok) {
    delay(100);
    WiFi.softAP(BL_AP_SSID, BL_AP_PASS);
  }
  if (wrServerUp) wrSrv.stop();
  wrSrv.begin();
  wrServerUp = true;
}

static void wrReleaseAll() {
  for (int i = 0; i < 6; i++) { wrHeld[i] = false; wrRel[i] = false; }
}

static void wrStart() {
  if (!wrRoutes) {
    wrSrv.on("/", HTTP_GET, wrHandleRoot);
    wrSrv.on("/fb", HTTP_GET, wrHandleFb);
    wrSrv.on("/btn", HTTP_GET, wrHandleBtn);
    wrSrv.on("/cmd", HTTP_GET, wrHandleCmd);
    wrRoutes = true;
  }
  wrEnabled = true;
  wrApUp();
  wrNextCheck = millis() + 1500;
}

static void wrStop() {
  if (!wrEnabled) return;
  wrEnabled = false;
  if (wrServerUp) { wrSrv.stop(); wrServerUp = false; }
  wrReleaseAll();
  // Если AP сейчас нужна PAINT / проводнику / настройке boot-логотипа — они сами ею управляют
  if (currentState != PAINT && currentState != FILE_MANAGER && currentState != BOOT_LOGO_SETTINGS) {
    WiFi.softAPdisconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    startSavedWifi();
  }
}

static String wrInfoText() {
  String s;
  if (!wrEnabled) return "Web remote: OFF";
  s += "Web remote: ON\n";
  s += "WiFi: "; s += BL_AP_SSID; s += "\n";
  s += "Pass: "; s += BL_AP_PASS; s += "\n";
  s += "http://"; s += WiFi.softAPIP().toString(); s += ":"; s += String(WR_PORT);
  if (WiFi.status() == WL_CONNECTED) {
    s += "\nLAN: http://"; s += WiFi.localIP().toString(); s += ":"; s += String(WR_PORT);
  }
  return s;
}

// Вызывается из loop() на каждом проходе
void wrService() {
  if (!wrEnabled) return;
  unsigned long now = millis();

  // Отпускание виртуальных кнопок
  for (int i = 0; i < 6; i++) {
    if (!wrHeld[i]) continue;
    if (now >= wrMaxAt[i] || (wrRel[i] && wrSeen[i] && now >= wrMinAt[i])) {
      wrHeld[i] = false;
      wrRel[i] = false;
    }
  }

  // PAINT / проводник при выходе гасят AP — поднимаем обратно
  if (now >= wrNextCheck) {
    wrNextCheck = now + 1500;
    if (!(WiFi.getMode() & WIFI_MODE_AP)) wrApUp();
  }

  wrSrv.handleClient();
}

#endif
