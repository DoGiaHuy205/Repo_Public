#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "P205";
const char* WIFI_PASS = "67896789";
// const char* WIFI_SSID = "Nha moi lap Wifi 6";
// const char* WIFI_PASS = "Hoilamgi@";
// const char* WIFI_SSID = "PTIT_WIFI";
// const char* WIFI_PASS = "";

#define STM32_UART  Serial2
#define STM32_BAUD  115200
#define STM32_RX    16
#define STM32_TX    17

#define PROTO_S0    0xAA
#define PROTO_S1    0xBB
#define PROTO_S2    0xCC
#define TYPE_RAW    0x01
#define TYPE_FIR    0x02
#define TYPE_IIR    0x03

#define MAX_WAV  (50 * 1024)

uint8_t*  raw_buf  = nullptr;
uint8_t*  fir_buf  = nullptr;
uint8_t*  iir_buf  = nullptr;
uint32_t  raw_size = 0;
uint32_t  fir_size = 0;
uint32_t  iir_size = 0;

bool raw_ready = false;
bool fir_ready = false;
bool iir_ready = false;

WebServer server(80);

enum RxState { RX_IDLE,RX_S1,RX_S2,RX_TYPE,RX_SZ0,RX_SZ1,RX_SZ2,RX_SZ3,RX_DATA };
RxState  rxState    = RX_IDLE;
uint8_t  rxType     = 0;
uint32_t rxExpected = 0;
uint32_t rxReceived = 0;
uint8_t* rxTarget   = nullptr;

void processUART() {
    while (STM32_UART.available()) {
        uint8_t b = (uint8_t)STM32_UART.read();
        switch (rxState) {
            case RX_IDLE: rxState=(b==PROTO_S0)?RX_S1:RX_IDLE; break;
            case RX_S1:   rxState=(b==PROTO_S1)?RX_S2:RX_IDLE; break;
            case RX_S2:   rxState=(b==PROTO_S2)?RX_TYPE:RX_IDLE; break;
            case RX_TYPE:
                rxType=b;
                if      (b==TYPE_RAW) rxTarget=raw_buf;
                else if (b==TYPE_FIR) rxTarget=fir_buf;
                else if (b==TYPE_IIR) rxTarget=iir_buf;
                else                  rxTarget=nullptr;
                rxExpected=0; rxReceived=0; rxState=RX_SZ0; break;
            case RX_SZ0: rxExpected =(uint32_t)b<<24; rxState=RX_SZ1; break;
            case RX_SZ1: rxExpected|=(uint32_t)b<<16; rxState=RX_SZ2; break;
            case RX_SZ2: rxExpected|=(uint32_t)b<<8;  rxState=RX_SZ3; break;
            case RX_SZ3:
                rxExpected|=(uint32_t)b;
                Serial.printf("[UART] TYPE=0x%02X size=%u\n",rxType,rxExpected);
                rxState=(rxTarget&&rxExpected>0&&rxExpected<=MAX_WAV)?RX_DATA:RX_IDLE;
                break;
            case RX_DATA:
                if (rxReceived<rxExpected&&rxTarget) rxTarget[rxReceived]=b;
                rxReceived++;
                if (rxReceived>=rxExpected) {
                    if      (rxType==TYPE_RAW){raw_size=rxReceived;raw_ready=true;}
                    else if (rxType==TYPE_FIR){fir_size=rxReceived;fir_ready=true;}
                    else if (rxType==TYPE_IIR){iir_size=rxReceived;iir_ready=true;}
                    Serial.printf("[UART] DONE 0x%02X %u bytes\n",rxType,rxReceived);
                    rxState=RX_IDLE;
                }
                break;
        }
    }
}

// HTML PAGE
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>STM32 FIR/IIR Filter Demo</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;700&display=swap');
:root{
  --bg:#0d1117;--card:#161b22;--border:#30363d;
  --blue:#58a6ff;--green:#3fb950;--purple:#bc8cff;
  --yellow:#d29922;--red:#f85149;
  --text:#e6edf3;--muted:#8b949e;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:'Inter',sans-serif;padding:12px;min-height:100vh}
h1{font-size:1.35rem;font-weight:700;text-align:center;padding:10px 0 2px;
   background:linear-gradient(90deg,var(--blue),var(--purple));
   -webkit-background-clip:text;-webkit-text-fill-color:transparent}
.sub{text-align:center;color:var(--muted);font-size:.75rem;margin-bottom:12px}

/* Status */
.sbar{display:flex;align-items:center;gap:8px;padding:7px 12px;
  background:var(--card);border:1px solid var(--border);border-radius:8px;margin-bottom:10px;font-size:.82rem}
.dot{width:9px;height:9px;border-radius:50%;background:var(--muted);flex-shrink:0}
.dot.live{background:var(--green);box-shadow:0 0 6px var(--green);animation:p 1.5s infinite}
.dot.wait{background:var(--blue);animation:p 1.5s infinite}
@keyframes p{0%,100%{opacity:1}50%{opacity:.3}}

/* Badges toggle */
.badges{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-bottom:10px}
.badge{display:flex;align-items:center;gap:5px;padding:4px 10px;border-radius:16px;
  font-size:.75rem;font-weight:600;border:1px solid transparent;
  cursor:pointer;user-select:none;opacity:.38;transition:.25s}
.badge:hover{opacity:.7}
.badge.on{opacity:1}
.badge.raw{border-color:var(--blue);color:var(--blue)}
.badge.fir{border-color:var(--green);color:var(--green)}
.badge.iir{border-color:var(--purple);color:var(--purple)}
.badge.js-fir{border-color:#f0c040;color:#f0c040}
.badge.js-iir{border-color:#ff7eb3;color:#ff7eb3}
.bdot{width:7px;height:7px;border-radius:50%;background:currentColor}
.badge-hint{font-size:.7rem;color:var(--muted);margin-left:auto}

/* Charts */
.cc{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:12px;margin-bottom:10px}
.ct{font-size:.78rem;font-weight:600;color:var(--muted);text-transform:uppercase;
  letter-spacing:.06em;margin-bottom:8px;display:flex;align-items:center;gap:6px}
canvas{width:100%;border-radius:5px;background:#010409;display:block}

/* ===== FILTER CONFIG PANEL ===== */
.fcfg{background:var(--card);border:1px solid var(--border);border-radius:10px;
  padding:14px;margin-bottom:10px}
.fcfg-title{font-size:.82rem;font-weight:700;margin-bottom:12px;
  display:flex;align-items:center;gap:8px}
.fcfg-title svg{flex-shrink:0}
.frow{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:10px}
@media(max-width:540px){.frow{grid-template-columns:1fr}}
.fblock{background:#0d1117;border:1px solid var(--border);border-radius:8px;padding:10px}
.flabel{font-size:.75rem;font-weight:600;margin-bottom:8px;display:flex;align-items:center;gap:6px}
.flabel.fir{color:var(--yellow)} .flabel.iir{color:#ff7eb3}
.fcontrols{display:flex;align-items:center;gap:8px;margin-bottom:6px}
input[type=range]{flex:1;accent-color:var(--blue);height:4px;cursor:pointer}
.fnum{width:72px;background:#1c2128;border:1px solid var(--border);border-radius:5px;
  color:var(--text);font-size:.82rem;padding:3px 6px;text-align:center}
.fnum:focus{outline:none;border-color:var(--blue)}
.funit{font-size:.75rem;color:var(--muted)}
.finfo{font-size:.7rem;color:var(--muted)}
.fbtn-row{display:flex;gap:8px;justify-content:flex-end}
.fbtn{background:linear-gradient(135deg,var(--blue),var(--purple));
  border:none;border-radius:6px;color:white;font-size:.8rem;font-weight:600;
  padding:7px 18px;cursor:pointer;transition:.2s}
.fbtn:hover{opacity:.85;transform:scale(1.03)}
.fbtn.sec{background:var(--card);border:1px solid var(--border);color:var(--muted)}

/* Comparison labels */
.cmp-label{font-size:.72rem;background:#21262d;border-radius:4px;padding:2px 6px;color:var(--muted)}

/* Info row */
.irow{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:10px}
.ic{background:var(--card);border:1px solid var(--border);border-radius:8px;padding:9px;text-align:center}
.ic-lbl{font-size:.68rem;color:var(--muted);margin-bottom:4px}
.ic-vals{display:flex;gap:6px;justify-content:center;flex-wrap:wrap}
.iv{font-size:.88rem;font-weight:700}
.iv.raw{color:var(--blue)} .iv.fir{color:var(--green)} .iv.iir{color:var(--purple)}

/* Audio */
.agrid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:10px}
.acard{background:var(--card);border:1px solid var(--border);border-radius:8px;padding:9px}
.albl{font-size:.72rem;font-weight:600;margin-bottom:5px}
.albl.raw{color:var(--blue)}.albl.fir{color:var(--green)}.albl.iir{color:var(--purple)}
audio{width:100%;height:28px}

/* Log */
.log{background:var(--card);border:1px solid var(--border);border-radius:8px;
  padding:8px;font-size:.72rem;max-height:120px;overflow-y:auto;font-family:monospace}
.log p{padding:1px 0;border-bottom:1px solid #1c2128}
.ok{color:var(--green)}.warn{color:var(--red)}.inf{color:var(--blue)}
</style>
</head>
<body>
<h1>STM32 FIR/IIR Filter Demo</h1>
<p class="sub">STM32F103 → ESP32 | 8kHz 16-bit Mono | Điều chỉnh Fc thời gian thực</p>

<div class="sbar">
  <div class="dot wait" id="dot"></div>
  <span id="stxt">Đang chờ STM32...</span>
  <span style="margin-left:auto;font-size:.7rem;color:var(--muted)" id="heap"></span>
</div>

<!-- Visibility toggles -->
<div class="badges">
  <div class="badge raw on"   id="bg-raw"    onclick="tog('raw')"   ><div class="bdot"></div>RAW</div>
  <div class="badge fir"      id="bg-stm-fir" onclick="tog('stm-fir')"><div class="bdot"></div>FIR STM32</div>
  <div class="badge iir"      id="bg-stm-iir" onclick="tog('stm-iir')"><div class="bdot"></div>IIR STM32</div>
  <div class="badge js-fir on" id="bg-js-fir" onclick="tog('js-fir')"><div class="bdot"></div>FIR (JS)</div>
  <div class="badge js-iir on" id="bg-js-iir" onclick="tog('js-iir')"><div class="bdot"></div>IIR (JS)</div>
  <span class="badge-hint">← Click ẩn/hiện</span>
</div>

<!-- Charts -->
<div class="cc">
  <div class="ct">
    ⏱ Miền Thời Gian
    <span class="cmp-label">RAW vs Filtered (JS Fc) vs STM32</span>
  </div>
  <canvas id="cv-time" height="155"></canvas>
</div>
<div class="cc">
  <div class="ct">
    📊 Miền Tần Số — FFT
    <span class="cmp-label">RAW vs Filtered (JS Fc) vs STM32</span>
  </div>
  <canvas id="cv-fft" height="155"></canvas>
</div>

<!-- ===== FILTER CONFIG ===== -->
<div class="fcfg">
  <div class="fcfg-title">
    <svg width="14" height="14" viewBox="0 0 16 16" fill="#58a6ff"><path d="M8 3a5 5 0 1 0 0 10A5 5 0 0 0 8 3zm0 1a4 4 0 1 1 0 8A4 4 0 0 1 8 4zm0 2a2 2 0 1 0 0 4 2 2 0 0 0 0-4z"/></svg>
    Cấu hình bộ lọc — Điều chỉnh tần số cắt Fc
    <span style="font-size:.7rem;color:var(--muted);font-weight:400">| Áp dụng ngay lên RAW PCM (không cần STM32)</span>
  </div>
  <div class="frow">
    <!-- FIR -->
    <div class="fblock">
      <div class="flabel fir">⚙ FIR Lowpass — Hamming 31-tap</div>
      <div class="fcontrols">
        <input type="range" id="sl-fir" min="100" max="3500" value="1000" step="50">
        <input type="number" class="fnum" id="num-fir" min="100" max="3500" value="1000" step="50">
        <span class="funit">Hz</span>
      </div>
      <div class="finfo">≤ Nyquist: 4000 Hz | STM32 mặc định: 1000 Hz</div>
    </div>
    <!-- IIR -->
    <div class="fblock">
      <div class="flabel iir">⚙ IIR Lowpass — Butterworth bậc 2</div>
      <div class="fcontrols">
        <input type="range" id="sl-iir" min="100" max="3500" value="1000" step="50">
        <input type="number" class="fnum" id="num-iir" min="100" max="3500" value="1000" step="50">
        <span class="funit">Hz</span>
      </div>
      <div class="finfo">≤ Nyquist: 4000 Hz | STM32 mặc định: 1000 Hz</div>
    </div>
  </div>
  <div class="fbtn-row">
    <button class="fbtn sec" onclick="resetFc()">↺ Reset 1kHz</button>
    <button class="fbtn" onclick="applyFilters()">▶ Áp dụng bộ lọc</button>
  </div>
</div>

<!-- Info -->
<div class="irow">
  <div class="ic">
    <div class="ic-lbl">⏳ Thời lượng (s)</div>
    <div class="ic-vals">
      <span class="iv raw" id="dur-raw">—</span>
      <span class="iv fir" id="dur-fir">—</span>
      <span class="iv iir" id="dur-iir">—</span>
    </div>
  </div>
  <div class="ic">
    <div class="ic-lbl">📦 Kích thước (KB)</div>
    <div class="ic-vals">
      <span class="iv raw" id="sz-raw">—</span>
      <span class="iv fir" id="sz-fir">—</span>
      <span class="iv iir" id="sz-iir">—</span>
    </div>
  </div>
  <div class="ic">
    <div class="ic-lbl">📶 RMS (dBFS)</div>
    <div class="ic-vals">
      <span class="iv raw" id="rms-raw">—</span>
      <span class="iv fir" id="rms-fir">—</span>
      <span class="iv iir" id="rms-iir">—</span>
    </div>
  </div>
</div>

<!-- Audio players -->
<div class="agrid" style="grid-template-columns:1fr 1fr 1fr">
  <div class="acard">
    <div class="albl raw">🎙 RAW (gốc)</div>
    <audio id="aud-raw" controls></audio>
  </div>
  <div class="acard">
    <div class="albl fir">✨ FIR (JS) — fc=<span id="tfc-fir">1000</span>Hz</div>
    <audio id="aud-js-fir" controls></audio>
    <div style="font-size:.68rem;color:var(--muted);margin-top:3px">Cập nhật khi kéo slider</div>
  </div>
  <div class="acard">
    <div class="albl iir">⚡ IIR (JS) — fc=<span id="tfc-iir">1000</span>Hz</div>
    <audio id="aud-js-iir" controls></audio>
    <div style="font-size:.68rem;color:var(--muted);margin-top:3px">Cập nhật khi kéo slider</div>
  </div>
</div>

<div class="log" id="lb"><p class="inf">» Khởi động...</p></div>

<script>
const FS=8000, POLL=1500;
/* Colors: raw=blue, stm-fir=green(dashed), stm-iir=purple(dashed), js-fir=yellow, js-iir=pink */
const COL={raw:'#58a6ff','stm-fir':'#3fb950','stm-iir':'#bc8cff','js-fir':'#f0c040','js-iir':'#ff7eb3'};
let rawPcm=null, stmFirPcm=null, stmIirPcm=null;
let jsFirPcm=null, jsIirPcm=null;
let rawVer=0,firVer=0,iirVer=0;
let audioCtx=null;
let actualSR=8000; /* Sample rate thực sau khi browser decode (44100 hoặc 48000) */
let show={raw:true,'stm-fir':false,'stm-iir':false,'js-fir':true,'js-iir':true};
let debTimer=null;

// VISIBILITY TOGGLE
function tog(ch){
  show[ch]=!show[ch];
  const el=document.getElementById('bg-'+ch);
  el.classList.toggle('on',show[ch]);
  redrawAll();
  log((show[ch]?'Hiện':'Ẩn')+' '+ch,'inf');
}

// FIR DESIGN: Windowed Sinc Hamming
// h[n] = sinc(wc*(n-M/2)) * hamming(n,M)
function designFIR(fc,fs,N){
  const h=new Float32Array(N), M=N-1, wc=2*Math.PI*fc/fs;
  for(let n=0;n<N;n++){
    const m=n-M/2;
    const s=(m===0)?wc/Math.PI:Math.sin(wc*m)/(Math.PI*m);
    const w=0.54-0.46*Math.cos(2*Math.PI*n/M);
    h[n]=s*w;
  }
  return h;
}
function applyFIR(pcm,h){
  const N=h.length, out=new Float32Array(pcm.length);
  for(let i=0;i<pcm.length;i++){
    let s=0;
    for(let k=0;k<N;k++) if(i-k>=0) s+=h[k]*pcm[i-k];
    out[i]=s;
  }
  return out;
}

// IIR DESIGN: Butterworth bac 2 (Bilinear Transform)
// Biquad: H(z) = (b0 + b1z^-1 + b2z^-2) / (1 + a1z^-1 + a2z^-2)
function designIIR(fc,fs){
  const k=Math.tan(Math.PI*fc/fs);
  const k2=k*k, m=Math.SQRT2;
  const norm=1/(1+m*k+k2);
  return{b0:k2*norm, b1:2*k2*norm, b2:k2*norm,
         a1:2*(k2-1)*norm, a2:(1-m*k+k2)*norm};
}
function applyIIR(pcm,{b0,b1,b2,a1,a2}){
  const out=new Float32Array(pcm.length);
  let x1=0,x2=0,y1=0,y2=0;
  for(let i=0;i<pcm.length;i++){
    const x0=pcm[i];
    const y0=b0*x0+b1*x1+b2*x2-a1*y1-a2*y2;
    out[i]=y0; x2=x1;x1=x0;y2=y1;y1=y0;
  }
  return out;
}

// PCM → WAV BLOB
function pcmToWav(pcm,fs){
  const n=pcm.length, buf=new ArrayBuffer(44+n*2), v=new DataView(buf);
  const ws=(o,s)=>{for(let i=0;i<s.length;i++)v.setUint8(o+i,s.charCodeAt(i));};
  ws(0,'RIFF');v.setUint32(4,36+n*2,true);ws(8,'WAVE');ws(12,'fmt ');
  v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,1,true);
  v.setUint32(24,fs,true);v.setUint32(28,fs*2,true);
  v.setUint16(32,2,true);v.setUint16(34,16,true);
  ws(36,'data');v.setUint32(40,n*2,true);
  for(let i=0;i<n;i++){const s=Math.max(-1,Math.min(1,pcm[i]));v.setInt16(44+i*2,s<0?s*32768:s*32767,true);}
  return new Blob([buf],{type:'audio/wav'});
}

// APPLY FILTERS (JS-side)
// Called on slider change or button click
function applyFilters(){
  if(!rawPcm){log('Chưa có RAW data','warn');return;}
  const fc_fir=Math.max(100,Math.min(3500,parseInt(document.getElementById('num-fir').value)||1000));
  const fc_iir=Math.max(100,Math.min(3500,parseInt(document.getElementById('num-iir').value)||1000));

  // Tinh he so bo loc — dung actualSR (sample rate thuc cua browser, e.g. 44100Hz)
  // De dam bao Fc/Fs dung: neu dung FS=8000 thi Fc/Fs sai 5.5× → sai cutoff
  const h   = designFIR(fc_fir, actualSR, 31);
  const iirC= designIIR(fc_iir, actualSR);

  // Ap dung loc
  jsFirPcm = applyFIR(rawPcm, h);
  jsIirPcm = applyIIR(rawPcm, iirC);

  // Cap nhat audio player voi WAV da loc — actualSR dam bao duration khop RAW
  const urlFir = URL.createObjectURL(pcmToWav(jsFirPcm, actualSR));
  const urlIir = URL.createObjectURL(pcmToWav(jsIirPcm, actualSR));
  const pFir = document.getElementById('aud-js-fir');
  const pIir = document.getElementById('aud-js-iir');
  if(pFir.src) URL.revokeObjectURL(pFir.src);
  if(pIir.src) URL.revokeObjectURL(pIir.src);
  pFir.src = urlFir;
  pIir.src = urlIir;

  // Cap nhat nhan hien thi fc
  document.getElementById('tfc-fir').textContent = fc_fir;
  document.getElementById('tfc-iir').textContent = fc_iir;

  redrawAll();
  log(`✓ FIR(${fc_fir}Hz) & IIR(${fc_iir}Hz) áp dụng xong`, 'ok');
}

function resetFc(){
  document.getElementById('sl-fir').value=1000;
  document.getElementById('num-fir').value=1000;
  document.getElementById('sl-iir').value=1000;
  document.getElementById('num-iir').value=1000;
  applyFilters();
}

// Sync slider ↔ number input, debounce auto-apply
function syncFC(type,val){
  document.getElementById('sl-'+type).value=val;
  document.getElementById('num-'+type).value=val;
  clearTimeout(debTimer);
  debTimer=setTimeout(applyFilters,400);
}

// CANVAS HELPERS
function rz(id){const c=document.getElementById(id);c.width=c.offsetWidth||720;return c;}

function drawTimeOverlay(){
  const cv=rz('cv-time'); const ctx=cv.getContext('2d');
  const W=cv.width,H=cv.height;
  ctx.fillStyle='#010409';ctx.fillRect(0,0,W,H);
  // grid
  ctx.strokeStyle='#1c2128';ctx.lineWidth=1;
  for(let y=H/4;y<H;y+=H/4){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(W,y);ctx.stroke();}
  ctx.strokeStyle='#21262d';ctx.beginPath();ctx.moveTo(0,H/2);ctx.lineTo(W,H/2);ctx.stroke();

  function wave(pcm,color,dash,lbl,ly){
    if(!pcm)return;
    const step=Math.max(1,Math.floor(pcm.length/W));
    ctx.save();ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=1.5;
    ctx.setLineDash(dash);ctx.globalAlpha=.82;
    for(let x=0;x<W;x++){
      let mn=1,mx=-1;
      for(let j=0;j<step;j++){const v=pcm[x*step+j]||0;if(v<mn)mn=v;if(v>mx)mx=v;}
      const y1=H/2-mx*H/2*.9,y2=H/2-mn*H/2*.9;
      x===0?ctx.moveTo(x,y1):ctx.lineTo(x,y1);
      if(y1!==y2)ctx.lineTo(x,y2);
    }
    ctx.stroke();ctx.setLineDash([]);ctx.globalAlpha=1;
    ctx.fillStyle=color;ctx.font='bold 10px Inter';ctx.fillText(lbl,6,ly);
    ctx.restore();
  }
  if(show.raw)          wave(rawPcm,    COL.raw,     [],    'RAW',     12);
  if(show['js-fir'])    wave(jsFirPcm,  COL['js-fir'],[],   'FIR(JS)', 24);
  if(show['js-iir'])    wave(jsIirPcm,  COL['js-iir'],[],   'IIR(JS)', 36);
  if(show['stm-fir'])   wave(stmFirPcm, COL['stm-fir'],[4,3],'FIR-STM',48);
  if(show['stm-iir'])   wave(stmIirPcm, COL['stm-iir'],[4,3],'IIR-STM',60);
}

function drawFFTOverlay(){
  const cv=rz('cv-fft'); const ctx=cv.getContext('2d');
  const W=cv.width,H=cv.height;
  ctx.fillStyle='#010409';ctx.fillRect(0,0,W,H);
  // freq grid
  [500,1000,2000,3000].forEach(f=>{
    const x=Math.floor(f/4000*W);
    ctx.strokeStyle='#1c2128';ctx.lineWidth=1;
    ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,H);ctx.stroke();
    ctx.fillStyle='#4a556866';ctx.font='9px Inter';
    ctx.fillText(f>=1000?(f/1000)+'kHz':f+'Hz',x+2,H-3);
  });

  function fftAndDraw(pcm,color,dash,label,ly){
    if(!pcm)return;
    const N=1024;
    const mag=new Float32Array(N/2);
    const s=Math.max(1,Math.floor(N/256));
    for(let k=0;k<N/2;k++){
      let sr=0,si=0;
      for(let n=0;n<N;n+=s){
        const a=2*Math.PI*k*n/N;
        sr+=pcm[n]*Math.cos(a);si-=pcm[n]*Math.sin(a);
      }
      mag[k]=Math.sqrt(sr*sr+si*si)/(N/s);
    }
    let mx=1e-9;for(let i=1;i<mag.length;i++)if(mag[i]>mx)mx=mag[i];
    ctx.save();ctx.beginPath();ctx.strokeStyle=color;
    ctx.lineWidth=1.6;ctx.setLineDash(dash);ctx.globalAlpha=.85;
    for(let i=0;i<N/2;i++){
      const x=i/(N/2)*W;
      const y=H-(mag[i]/mx)*H*.92;
      i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
    }
    ctx.stroke();ctx.setLineDash([]);ctx.globalAlpha=1;
    // Fc marker if js filter
    ctx.fillStyle=color;ctx.font='bold 10px Inter';ctx.fillText(label,W-58,ly);
    ctx.restore();
  }
  // fc line for current JS Fc
  const fc_fir=parseInt(document.getElementById('num-fir').value)||1000;
  const fc_iir=parseInt(document.getElementById('num-iir').value)||1000;

  if(show['js-fir']&&jsFirPcm){
    const xf=fc_fir/4000*W;
    ctx.save();ctx.strokeStyle=COL['js-fir'];ctx.lineWidth=1;ctx.setLineDash([3,3]);ctx.globalAlpha=.5;
    ctx.beginPath();ctx.moveTo(xf,0);ctx.lineTo(xf,H-14);ctx.stroke();
    ctx.fillStyle=COL['js-fir'];ctx.globalAlpha=.8;ctx.font='9px Inter';
    ctx.fillText('fc='+fc_fir+'Hz',xf+2,12);ctx.restore();
  }
  if(show['js-iir']&&jsIirPcm){
    const xi=fc_iir/4000*W;
    ctx.save();ctx.strokeStyle=COL['js-iir'];ctx.lineWidth=1;ctx.setLineDash([3,3]);ctx.globalAlpha=.5;
    ctx.beginPath();ctx.moveTo(xi,0);ctx.lineTo(xi,H-14);ctx.stroke();
    ctx.fillStyle=COL['js-iir'];ctx.globalAlpha=.8;ctx.font='9px Inter';
    ctx.fillText('fc='+fc_iir+'Hz',xi+2,22);ctx.restore();
  }

  if(show.raw)           fftAndDraw(rawPcm,    COL.raw,     [],    'RAW',     14);
  if(show['js-fir'])     fftAndDraw(jsFirPcm,  COL['js-fir'],[],   'FIR(JS)', 26);
  if(show['js-iir'])     fftAndDraw(jsIirPcm,  COL['js-iir'],[],   'IIR(JS)', 38);
  if(show['stm-fir'])    fftAndDraw(stmFirPcm, COL['stm-fir'],[4,3],'FIR-STM',50);
  if(show['stm-iir'])    fftAndDraw(stmIirPcm, COL['stm-iir'],[4,3],'IIR-STM',62);
}

function redrawAll(){drawTimeOverlay();drawFFTOverlay();}

// LOAD AUDIO FROM SERVER
function log(m,c=''){
  const b=document.getElementById('lb');
  const p=document.createElement('p');
  p.className=c;p.textContent='['+new Date().toLocaleTimeString('vi-VN')+'] '+m;
  b.prepend(p);if(b.children.length>30)b.lastChild.remove();
}
function setStatus(t,m){document.getElementById('dot').className='dot '+t;document.getElementById('stxt').textContent=m;}

async function loadAudio(ch,ep){
  try{
    const r=await fetch('/'+ep+'?'+Date.now());
    if(!r.ok)throw new Error('HTTP '+r.status);
    const ab=await r.arrayBuffer();
    if(!audioCtx)audioCtx=new(window.AudioContext||window.webkitAudioContext)();
    return new Promise(res=>{
      audioCtx.decodeAudioData(ab.slice(0),buf=>{
        const pcm=buf.getChannelData(0);
        // Luu sample rate thuc cua browser (44100/48000) de dung cho JS filter va pcmToWav
        if(ch==='raw'){ rawPcm=pcm; actualSR=buf.sampleRate; }
        else if(ch==='fir'){stmFirPcm=pcm;}
        else{stmIirPcm=pcm;}
        const dur=buf.duration.toFixed(2);
        const kb=(ab.byteLength/1024).toFixed(1);
        let rms=0;for(let i=0;i<pcm.length;i++)rms+=pcm[i]*pcm[i];
        rms=(20*Math.log10(Math.sqrt(rms/pcm.length)+1e-9)).toFixed(1);
        document.getElementById('dur-'+ch).textContent=dur+'s';
        document.getElementById('sz-'+ch).textContent=kb;
        document.getElementById('rms-'+ch).textContent=rms;
        // RAW player
        if(ch==='raw'){
          const blob=new Blob([new Uint8Array(ab.slice(0))],{type:'audio/wav'});
          document.getElementById('aud-raw').src=URL.createObjectURL(blob);
          // Ngay lap tuc apply JS filter khi co RAW
          applyFilters();
        }
        log('✓ '+ch.toUpperCase()+': '+dur+'s, '+kb+'KB','ok');
        res();
      },e=>{log('Decode lỗi '+ch+': '+(e||'?'),'warn');res();});
    });
  }catch(e){log('Lỗi '+ch+': '+e.message,'warn');}
}

// POLLING
async function poll(){
  try{
    const r=await fetch('/check?'+Date.now());
    const d=await r.json();
    if(d.heap)document.getElementById('heap').textContent='Heap: '+(d.heap/1024).toFixed(0)+'KB';
    const t=[];
    if(d.raw&&d.raw!==rawVer){rawVer=d.raw;t.push(loadAudio('raw','audio_raw'));}
    if(d.fir&&d.fir!==firVer){firVer=d.fir;t.push(loadAudio('fir','audio_fir'));}
    if(d.iir&&d.iir!==iirVer){iirVer=d.iir;t.push(loadAudio('iir','audio_iir'));}
    if(t.length){
      setStatus('wait','Đang tải...');
      await Promise.all(t);
      redrawAll();
      const cnt=[rawPcm,stmFirPcm,stmIirPcm].filter(Boolean).length;
      setStatus('live',cnt===3?'Tất cả 3 tín hiệu sẵn sàng ✓':'Đã nhận '+cnt+'/3');
    }
  }catch(e){}
}

// SLIDER EVENT BINDING
window.addEventListener('load',async()=>{
  log('Trang khởi động','ok');
  setTimeout(redrawAll,80);

  // Slider ↔ number sync
  ['fir','iir'].forEach(t=>{
    document.getElementById('sl-'+t).addEventListener('input',e=>syncFC(t,e.target.value));
    document.getElementById('num-'+t).addEventListener('input',e=>syncFC(t,e.target.value));
  });

  // Initial fetch
  try{
    const r=await fetch('/check?'+Date.now());const d=await r.json();
    const ta=[];
    if(d.raw){rawVer=d.raw;ta.push(loadAudio('raw','audio_raw'));}
    if(d.fir){firVer=d.fir;ta.push(loadAudio('fir','audio_fir'));}
    if(d.iir){iirVer=d.iir;ta.push(loadAudio('iir','audio_iir'));}
    if(ta.length){await Promise.all(ta);redrawAll();}
  }catch(e){}
  setInterval(poll,POLL);
  log('Polling mỗi '+POLL/1000+'s','inf');
});
window.addEventListener('resize',()=>setTimeout(redrawAll,50));
</script>
</body>
</html>
)rawhtml";

// HANDLERS
void handleRoot(){ server.send_P(200,"text/html; charset=utf-8",HTML_PAGE); }

void serveWAV(uint8_t* buf, uint32_t sz){
    if(!buf||sz==0){server.send(404,"text/plain","No data");return;}
    server.setContentLength(sz);
    server.send(200,"audio/wav","");
    WiFiClient client=server.client();
    const size_t CK=1024;
    for(size_t off=0;off<sz;off+=CK){
        size_t n=min((size_t)CK,(size_t)(sz-off));
        client.write(buf+off,n);
        processUART(); yield();
    }
}
void handleAudioRaw(){serveWAV(raw_buf,raw_size);raw_ready=false;Serial.printf("[HTTP] RAW %uKB\n",raw_size/1024);}
void handleAudioFIR(){serveWAV(fir_buf,fir_size);fir_ready=false;Serial.printf("[HTTP] FIR %uKB\n",fir_size/1024);}
void handleAudioIIR(){serveWAV(iir_buf,iir_size);iir_ready=false;Serial.printf("[HTTP] IIR %uKB\n",iir_size/1024);}

void handleCheck(){
    static uint16_t rv=0,fv=0,iv=0;
    static bool rs=false,fs=false,is=false;
    if(raw_ready&&!rs){rv++;rs=true;Serial.printf("[CHECK] RAW v%u\n",rv);}
    if(fir_ready&&!fs){fv++;fs=true;Serial.printf("[CHECK] FIR v%u\n",fv);}
    if(iir_ready&&!is){iv++;is=true;Serial.printf("[CHECK] IIR v%u\n",iv);}
    if(!raw_ready)rs=false; if(!fir_ready)fs=false; if(!iir_ready)is=false;
    String j="{\"raw\":"+String(rv)+",\"fir\":"+String(fv)+
             ",\"iir\":"+String(iv)+",\"heap\":"+String(ESP.getFreeHeap())+"}";
    server.sendHeader("Cache-Control","no-cache");
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.send(200,"application/json",j);
}

// SETUP & LOOP
void setup(){
    // MALLOC TRUOC TIEN — tranh fragmentation
    raw_buf=(uint8_t*)malloc(MAX_WAV);
    fir_buf=(uint8_t*)malloc(MAX_WAV);
    iir_buf=(uint8_t*)malloc(MAX_WAV);
    bool ok=(raw_buf&&fir_buf&&iir_buf);

    Serial.begin(115200); delay(300);
    Serial.println("\n=== STM32 Filter Demo v4.0 ===");
    if(!ok){
        Serial.printf("FATAL malloc! r=%p f=%p i=%p free=%u\n",
                      raw_buf,fir_buf,iir_buf,ESP.getFreeHeap());
        delay(2000); ESP.restart();
    }
    Serial.printf("Buffers OK: 3x%uKB, heap left: %uKB\n",
                  MAX_WAV/1024, ESP.getFreeHeap()/1024);

    STM32_UART.setRxBufferSize(4096);
    STM32_UART.begin(STM32_BAUD,SERIAL_8N1,STM32_RX,STM32_TX);
    Serial.printf("UART RX=%d TX=%d @%d\n",STM32_RX,STM32_TX,STM32_BAUD);

    Serial.printf("WiFi: %s...",WIFI_SSID);
    WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID,WIFI_PASS);
    int n=0;
    while(WiFi.status()!=WL_CONNECTED&&n++<40){delay(500);Serial.print(".");}
    if(WiFi.status()==WL_CONNECTED){
        Serial.printf("\nIP: %s\nOpen: http://%s\n",
          WiFi.localIP().toString().c_str(),WiFi.localIP().toString().c_str());
    }else{Serial.println("\nWiFi FAIL! Restart...");delay(3000);ESP.restart();}

    server.on("/",          HTTP_GET, handleRoot);
    server.on("/audio_raw", HTTP_GET, handleAudioRaw);
    server.on("/audio_fir", HTTP_GET, handleAudioFIR);
    server.on("/audio_iir", HTTP_GET, handleAudioIIR);
    server.on("/check",     HTTP_GET, handleCheck);
    server.begin();
    Serial.println("HTTP server started.");
}

void loop(){
    processUART();
    server.handleClient();
}
