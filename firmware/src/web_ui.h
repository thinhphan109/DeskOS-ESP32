#pragma once
#include <Arduino.h>

// Entire web app is served from firmware flash: no CDN, no filesystem mount.
static const char WEB_UI_HTML[] PROGMEM = R"MEODESK(
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="theme-color" content="#f3efe7">
  <title>mimi.desk · Không gian của bạn</title>
  <style>
/* Source for the embedded studio theme. Run embed_studio_style.py after editing. */
:root{--bg:#f6f5ef;--paper:#fffef9;--ink:#263e30;--muted:#596b5e;--line:#dde4d5;--mint:#e1ebd4;--dark:#294b3b;--coral:#b8654b;--lilac:#eee8dc;--teal:#315b40}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:14px/1.55 'Segoe UI',system-ui,sans-serif}button,input,select{font:inherit}button,a,input,select,summary{ -webkit-tap-highlight-color:transparent}button{cursor:pointer}button:disabled{cursor:not-allowed;opacity:.55}a{color:var(--teal)}[hidden]{display:none!important}
::selection{background:#c7ddb4;color:#203c28}:focus-visible{outline:2px solid #6b8c56;outline-offset:3px}input,textarea{caret-color:#315b40}::-webkit-scrollbar{width:9px;height:6px}::-webkit-scrollbar-thumb{background:#bdc9b4;border-radius:6px}
.wrap{max-width:1440px;margin:0 auto;padding:0 28px 24px;display:grid;grid-template-columns:180px minmax(0,1fr);column-gap:28px;align-items:start}.topbar{grid-column:1/-1;display:flex;align-items:center;justify-content:space-between;gap:20px;height:88px;border-bottom:1px solid var(--line);margin-bottom:24px}
.brand{display:flex;align-items:center;gap:10px;text-decoration:none;color:var(--ink)}.brandmark{width:38px;height:38px;border-radius:11px;background:var(--dark);display:grid;place-items:center}.brandmark svg{width:24px;height:24px;stroke:#d9e9c8}.brandname{font-size:25px;letter-spacing:-.03em;font-weight:750}.brand sub{display:block;font-size:10px;color:var(--muted);line-height:1.4}.topright{display:flex;align-items:center;gap:14px}.headnote{display:none}.connection{display:flex;align-items:center;gap:9px;background:#e9edde;padding:8px 13px;border-radius:9px;font-size:12px;color:#46613f}.led,.dot{width:7px;height:7px;border-radius:50%;background:#a48251;display:inline-block}.led.live{background:#64894f}
.intro{grid-column:2;display:flex;justify-content:space-between;align-items:center;gap:20px;margin:0 0 22px}.intro h1{font-size:30px;line-height:1.2;letter-spacing:-.025em;margin:0;font-weight:700}.intro p{font-size:13px;color:var(--muted);max-width:340px;margin:0}.intro em{font-style:normal}
.studio-nav{grid-column:1;grid-row:2/10;position:sticky;top:24px;display:flex;flex-direction:column;gap:5px;padding:0 14px 20px 0}.studio-nav button{display:flex;align-items:center;gap:12px;min-height:44px;padding:11px 13px;border:0;border-radius:9px;background:transparent;color:#52674f;text-align:left;font-size:13px;font-weight:600}.studio-nav button svg{width:19px;height:19px;flex-shrink:0}.studio-nav button:hover{background:#e7ecdf}.studio-nav button[aria-selected=true]{background:#d8e6cc;color:#284f32}.sync-mode{font-size:11px;color:var(--muted);display:block;padding:6px 0}
.workspace-panel{grid-column:2;display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:20px;min-width:0}.workspace-panel>*{min-width:0;margin:0!important}.workspace-panel>.main-grid,.workspace-panel>.history-panel,.workspace-panel>.system-details,.workspace-panel>.scene-composer,.workspace-panel>.mood-gallery,.workspace-panel>.advanced-pack{grid-column:1/-1}.main-grid{display:grid;grid-template-columns:minmax(0,1.1fr) minmax(300px,1fr);gap:20px}
.pet-card{position:relative;background:var(--mint);border-radius:14px;min-height:270px;padding:24px;overflow:hidden}.pet-top{display:flex;justify-content:flex-end}.round-icon{width:36px;height:36px;display:grid;place-items:center;background:#f5f8ee;border:0;border-radius:9px;color:#44663a}.round-icon svg{width:19px;height:19px}.pet-copy{max-width:57%;margin-top:0}.pet-copy h2{font-size:28px;line-height:1.2;font-weight:650;letter-spacing:-.025em;margin:0 0 12px}.pet-copy h2 span{display:block}.pet-copy p{font-size:13px;color:#4e6348;margin:0;line-height:1.6}.pet-art{position:absolute;right:16px;bottom:48px;width:39%;max-width:190px}.pet-art img{width:100%;display:block;image-rendering:auto}.pet-foot{position:absolute;bottom:22px;left:24px;right:24px;display:flex;justify-content:space-between}.mood{display:flex;align-items:center;gap:7px;font-size:11px;font-weight:600;color:#4c6543}.pet-index{display:none}
.metric-card,.control-card,.link-card,.health-story,.health-settings,.living-info,.scene-studio,.wifi-studio,.history-panel,.lab-panel,.scene-composer,.rule-editor{background:var(--paper);border:1px solid var(--line);border-radius:14px;padding:22px;min-width:0}.metric-top{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:16px}.eyebrow,.metric-label{font-size:12px;color:var(--muted);letter-spacing:0;text-transform:none}.tag{font-size:10px;font-weight:650;color:#526a48;background:#eaf0e1;padding:4px 8px;border-radius:6px}.cpu-head{display:flex;align-items:center;gap:20px;margin-bottom:18px}.cpu-ring{background:none!important;width:auto!important;height:auto!important;flex-shrink:0}.cpu-inner{background:transparent}.cpu-inner strong{display:block;font-size:40px;line-height:1.2;font-weight:650;letter-spacing:-.03em;font-variant-numeric:tabular-nums}.cpu-inner span{font-size:11px;color:var(--muted)}.cpu-side h3{font-size:16px;line-height:1.4;margin:3px 0}.cpu-side p{font-size:11px;color:var(--muted);margin:0}.barline{display:flex;justify-content:space-between;gap:12px;font-size:12px;margin:10px 0 5px}.barline strong{font-weight:600}.track,.focus-track,.ota-progress{height:6px;background:#e5eadc;border-radius:4px;overflow:hidden}.track>div,.focus-track>div,.ota-progress>div{height:100%;background:#7d985c;border-radius:4px;transition:width .25s}.gpu-fill{background:#af835a!important}.ram-fill{background:#688669!important}.tiny-row{display:flex;justify-content:space-between;gap:10px;font-size:10px;color:var(--muted);margin-top:10px}
h2,h3{letter-spacing:-.02em}h3{font-size:20px;line-height:1.3;margin:0 0 14px}.panel-title,.lab-head,.history-top{display:flex;justify-content:space-between;align-items:center;gap:16px;margin-bottom:16px}.panel-title h3,.lab-head h3,.history-top h2{margin:0}.panel-title>span{color:var(--muted);font-size:11px}.act,.navlink,.composer-actions button,.theme-buttons button,.history-ranges button,.lab-head button,.ota-upload button,.settings-save,.wifi-studio button,.pair button,.scene-shortcuts button{padding:9px 13px;background:#f8faf2;border:1px solid #ccd7c2;border-radius:8px;color:#365a3b;font-size:12px;font-weight:600;min-height:36px;cursor:pointer}.act:hover,.navlink:hover,.composer-actions button:hover,.theme-buttons button:hover,.history-ranges button:hover{background:#e7efdc}.act.primary,.settings-save,.composer-actions .save-layout{background:#315b40;color:#fff;border-color:#315b40}.actions{display:flex;gap:8px;flex-wrap:wrap}.nav{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:7px}.navlink.active,.theme-buttons button.active,.range-btn.active{background:#dceacb;border-color:#789264;color:#284f32}.theme-buttons{display:flex;flex-wrap:wrap;gap:8px}.scene-selects{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:14px}.scene-selects label{font-size:11px;color:var(--muted)}select,input[type=text],input[type=password],input[type=number]{width:100%;min-width:0;background:#fffef9;border:1px solid #cbd6c2;border-radius:8px;color:#294d34;padding:9px 11px;margin:5px 0}.scene-tip,.scene-message,.wifi-note,.settings-note,.ota-result,.lab-panel p,.rule-editor p{font-size:12px;color:var(--muted);line-height:1.65}.scene-shortcuts{display:flex;gap:8px;flex-wrap:wrap}.motion-signal{display:flex;gap:6px;flex-wrap:wrap;margin-top:15px}.motion-signal span{background:#edf1e5;color:#53674a;border-radius:6px;padding:6px 8px;font-size:10px}.motion-help{margin-top:15px;font-size:12px}.motion-help summary{color:#405d3f;cursor:pointer}.ambient-metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;margin-top:18px}.ambient-metrics small{display:block;font-size:10px;color:var(--muted)}.ambient-metrics strong{display:block;font-size:16px;margin-top:4px}
.history-top h2{font-size:20px}.history-sub,.history-foot,.history-aside small{font-size:11px;color:var(--muted)}.history-ranges{display:flex;gap:5px}.history-ranges button{padding:6px 9px;min-height:30px;font-size:11px}.history-layout{display:grid;grid-template-columns:minmax(0,1fr) 100px;gap:18px;align-items:center}.graph-wrap{position:relative;min-width:0}#historyCanvas{display:block;width:100%;height:170px}.chart-tip{display:none;position:absolute;z-index:3;background:#294b3b;color:#fff;padding:6px 9px;border-radius:7px;font-size:11px;pointer-events:none;transform:translateX(-50%);white-space:nowrap}.chart-tip.visible{display:block}.history-aside{display:grid;gap:14px}.history-aside small{display:block}.history-aside strong{font-size:20px}.history-foot{display:flex;justify-content:space-between;gap:12px;margin-top:10px;flex-wrap:wrap}.legend{display:flex;gap:12px}.legend span:before{content:'';width:7px;height:7px;display:inline-block;border-radius:50%;margin-right:5px;background:#7d985c}.legend .gpu:before{background:#af835a}.legend .ram:before{background:#688669}
.system-details,.advanced-pack{background:var(--paper);border:1px solid var(--line);border-radius:12px;padding:0;overflow:hidden}.system-details>summary,.advanced-pack>summary{padding:15px 20px;cursor:pointer;font-size:13px;font-weight:600;color:#486344}.system-details .health-story,.advanced-pack .catpack-panel{border:0;background:transparent;margin:0;border-radius:0}.health-metrics{display:grid;grid-template-columns:repeat(3,1fr);gap:14px}.health-metrics small{display:block;color:var(--muted);font-size:11px}.health-metrics strong{display:block;font-size:18px}.events-heading{font-size:11px;color:var(--muted);margin:18px 0 8px}.events{list-style:none;padding:0;margin:0;display:grid;gap:7px}.events li{display:flex;align-items:start;gap:8px;font-size:12px;padding:7px 0;border-bottom:1px solid #edf0e7}.events time{margin-left:auto;font-size:10px;color:var(--muted);white-space:nowrap}.event-label{font-size:9px;font-weight:600;background:#ecf0e4;padding:2px 5px;border-radius:4px}.priority-3 .event-label{background:#f7d9cf;color:#773a2e}.signature{display:flex;justify-content:space-between;color:var(--muted);font-size:10px;margin-top:12px}
.device-details{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin:16px 0}.device-details dt,.ota-checks small{font-size:10px;color:var(--muted)}.device-details dd{font-size:13px;margin:4px 0 0}.pair{display:flex;align-items:center;gap:8px}.pair input{flex:1}.fps-history{margin:14px 0}.fps-history-head,.fps-history-legend{display:flex;justify-content:space-between;font-size:10px;color:var(--muted)}#fpsHistoryCanvas{width:100%;height:80px;display:block}.health-settings h3,.wifi-studio h2{font-size:22px;margin:0 0 12px}.setting{margin:14px 0}.setting label{display:flex;justify-content:space-between;gap:8px;font-size:12px}.setting input[type=range]{width:100%;accent-color:#6d8c55}.quiet-row{display:flex;gap:8px;font-size:12px;margin:16px 0}.settings-save{width:100%}.wifi-studio label{display:block;font-size:11px;color:var(--muted);margin-top:10px}.wifi-studio>button{margin:10px 6px 0 0}.lab-panel{margin-top:18px}.lab-metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:14px;margin:16px 0}.lab-metrics dt{font-size:10px;color:var(--muted)}.lab-metrics dd{font-size:17px;margin:3px 0 0}.diagnostics{margin-top:18px}.diagnostics summary{cursor:pointer;font-size:12px;color:#486344}
.focus-studio{grid-column:1/-1;background:#eee8dc;border-radius:14px;padding:28px;min-height:290px}.focus-clock{font-size:76px;line-height:1.2;letter-spacing:-.025em;font-weight:650;margin:8px 0;color:#584b37;font-variant-numeric:tabular-nums}.focus-sub{font-size:12px;color:#73634a;margin-bottom:20px}.focus-track{background:#d7cfba;margin-bottom:20px}.focus-track>div{background:#9b8458}.focus-config{display:flex;align-items:end;gap:12px;flex-wrap:wrap;margin-top:24px;padding-top:18px;border-top:1px solid #d4cbb6}.focus-config label{display:grid;gap:4px;font-size:11px;flex:1;min-width:85px}.focus-config input{margin:0}.focus-config button{min-height:40px}
.music-studio{grid-column:1/-1;background:#244535;color:#f8f9ef;border-radius:14px;padding:28px;min-height:390px}.music-hero{display:grid;grid-template-columns:220px minmax(0,1fr);gap:32px;align-items:center;margin-bottom:26px}.music-artwork{width:220px;aspect-ratio:1;background:#36573d;border-radius:12px;overflow:hidden;display:grid;place-items:center}.music-artwork>svg{width:50%;height:50%;color:#b2d993}.music-artwork:has(img:not([hidden]))>svg{display:none}.album-image{position:static;width:100%;height:100%;object-fit:cover}.music-studio h2{font:700 clamp(28px,3.6vw,44px)/1.3 'Segoe UI',system-ui,sans-serif;letter-spacing:-.025em;margin:0 0 14px;overflow-wrap:anywhere}.music-studio .artist{font-size:18px;line-height:1.6;color:#d6e3ca;overflow-wrap:anywhere}.music-studio .track{background:#52694e}.music-studio .track>div{background:#b2d993}.music-time{margin-top:10px;font-size:12px;color:#d6e3ca}.music-actions{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-top:20px}.music-actions button{display:grid;place-items:center;width:48px;height:44px;background:#365d42;color:#edf6e4;border:0;border-radius:9px}.music-actions button.main{background:#c5e1aa;color:#244535;width:60px;height:52px}.music-actions button:hover{background:#527849}.music-actions svg{width:23px;height:23px}.music-status{font-size:11px;color:#d6e3ca;margin-left:auto}.play-icon{display:none}.music-studio.paused .play-icon{display:block}.music-studio.paused .pause-icon{display:none}
.ota-panel{grid-column:1/-1;display:grid;grid-template-columns:minmax(0,1fr) minmax(280px,.85fr);gap:24px;background:var(--paper);border:1px solid var(--line);border-radius:14px;padding:24px}.ota-panel h3{font-size:24px;font-style:normal;margin:0 0 12px}.ota-panel p{color:var(--muted);font-size:12px;line-height:1.7}.ota-checks{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:18px}.ota-checks small{display:block}.ota-checks strong{font-size:13px;display:block;margin-top:3px}.ota-upload{background:#edf2e5;border-radius:10px;padding:18px}.ota-upload label{font-size:11px;color:#526948;display:block;margin-bottom:10px}.ota-upload input{max-width:100%;font-size:12px}.ota-upload input::file-selector-button{background:var(--paper);border:1px solid #c8d6bb;border-radius:7px;padding:8px;margin-right:8px;color:#315b40}.ota-upload button{width:100%;margin:14px 0 0;background:#315b40;color:white;border-color:#315b40}.ota-warning{font-size:11px!important}.ota-progress{margin-top:14px}.pack-preview{display:grid;grid-template-columns:repeat(8,1fr);gap:6px;margin-top:12px}.pack-preview img{width:100%;image-rendering:pixelated}.pack-preview small{font-size:9px}.mood-gallery{display:grid;grid-template-columns:repeat(auto-fit,minmax(75px,1fr));gap:8px}.mood-tile{background:#fffef9;border:1px solid var(--line);border-radius:10px;padding:10px 6px;cursor:pointer;text-align:center}.mood-tile img{width:54px;display:block;margin:0 auto 5px}.mood-tile small{font-size:10px;color:var(--muted)}
.composer-body{display:grid;grid-template-columns:324px minmax(180px,1fr);gap:24px;margin:18px 0}.composer-canvas{display:block;width:324px;max-width:100%;height:auto;aspect-ratio:27/20;background:#f7f2e9;border:1px solid #bbc8ae;touch-action:none;cursor:move}.composer-fields{display:grid;grid-template-columns:1fr 1fr;gap:8px}.composer-fields label{display:flex;flex-direction:column;gap:4px;font-size:11px;color:#4e6646}.composer-fields .wide{grid-column:1/-1}.composer-fields input,.composer-fields select{width:100%;margin:0}.composer-actions{display:flex;flex-wrap:wrap;gap:8px}.scene-composer>p{font-size:12px;color:var(--muted)}.rule-editor{grid-column:1/-1}.rule-editor summary{cursor:pointer;font-weight:600}.rule-editor textarea{width:100%;min-height:210px;background:#fbfcf7;border:1px solid #c8d6bd;border-radius:8px;padding:12px;color:#294b3b;font:12px/1.5 Consolas,monospace;resize:vertical}.rule-editor .composer-actions{margin-top:12px}#configFile{max-width:100%;font-size:12px}
.status-strip{grid-column:2;display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;border-top:1px solid var(--line);margin-top:24px;padding-top:14px;color:var(--muted);font-size:10px}.status-strip .small-square{display:none}#toast{position:fixed;bottom:22px;left:50%;transform:translate(-50%,20px);opacity:0;pointer-events:none;background:#294b3b;color:white;border-radius:9px;padding:12px 18px;font-size:13px;max-width:min(90vw,600px);z-index:30;transition:transform .2s,opacity .2s}#toast.show{transform:translate(-50%,0);opacity:1}
@media(max-width:1100px){.wrap{grid-template-columns:154px minmax(0,1fr);gap:20px}.studio-nav{padding-right:0}.main-grid{grid-template-columns:1fr}.metric-card .cpu-head{margin-bottom:12px}.workspace-panel{gap:16px}.intro p{max-width:260px}}
@media(max-width:850px){.wrap{display:block;padding:0 20px 24px}.topbar{height:76px;margin-bottom:18px}.intro{margin-bottom:18px}.intro h1{font-size:27px}.studio-nav{position:sticky;top:0;z-index:10;display:flex;flex-direction:row;overflow-x:auto;padding:8px 0;margin:0 0 18px;background:var(--bg);border-bottom:1px solid var(--line);gap:4px}.studio-nav button{white-space:nowrap;min-height:40px;padding:9px 12px}.studio-nav button svg{display:none}.workspace-panel{grid-template-columns:1fr}.workspace-panel>*{grid-column:1!important}.main-grid{grid-template-columns:1fr 1fr}.lab-metrics{grid-template-columns:1fr 1fr}.ota-panel{grid-template-columns:1fr}.ambient-metrics{grid-template-columns:1fr 1fr}}
@media(max-width:650px){.wrap{padding:0 14px 20px}.brandname{font-size:23px}.brand sub{display:none}.connection{font-size:10px;padding:7px 9px}.intro{display:block}.intro p{max-width:100%;margin-top:7px}.main-grid{grid-template-columns:1fr}.pet-card{min-height:250px}.pet-copy h2{font-size:27px}.pet-art{max-width:150px}.workspace-panel{gap:14px}.metric-card,.control-card,.link-card,.health-story,.health-settings,.living-info,.scene-studio,.wifi-studio,.history-panel,.lab-panel,.scene-composer,.rule-editor{padding:18px}.history-top{align-items:start;flex-direction:column;gap:12px}.history-layout{grid-template-columns:1fr}.history-aside{grid-template-columns:repeat(3,1fr)}.history-aside strong{font-size:17px}.composer-body{grid-template-columns:1fr}.composer-canvas{justify-self:center}.music-studio{padding:20px}.music-hero{grid-template-columns:1fr;gap:20px}.music-artwork{width:min(100%,230px);justify-self:center}.music-copy{text-align:center}.music-studio h2{font-size:28px}.music-status{width:100%;text-align:center;margin:0}.music-actions{justify-content:center}.focus-studio{padding:22px}.focus-clock{font-size:64px}.health-metrics{gap:9px}.events li{flex-wrap:wrap}.pack-preview{grid-template-columns:repeat(4,1fr)}}
@media(prefers-reduced-motion:reduce){*,*:before,*:after{scroll-behavior:auto!important;transition:none!important;animation:none!important}}

.history-aside .peak{display:flex;flex-direction:column;gap:4px}.history-aside .peak b{font-size:11px;font-weight:500;color:var(--muted)}
.policy-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.policy-grid label{display:flex;flex-direction:column;gap:6px;color:var(--muted);font-size:12px}.policy-grid input:not([type=checkbox]),.policy-grid select{width:100%;min-width:0;padding:10px;border:1px solid var(--line);border-radius:8px;background:var(--paper);color:var(--ink);font:inherit}.policy-grid .policy-url{grid-column:1/-1}.policy-grid .policy-defer{flex-direction:row;align-items:center}.policy-actions{display:flex;flex-wrap:wrap;gap:8px;margin-top:16px}.policy-actions button{padding:10px 13px}.release-notes{white-space:pre-wrap}.server-update progress{width:100%;accent-color:var(--dark);height:9px}.manual-tools{margin-top:16px;border:1px solid var(--line);border-radius:12px;background:var(--paper)}.manual-tools>summary{padding:15px 18px;cursor:pointer;font-weight:600}.manual-tools>.ota-panel{border:0;margin-top:0}.setup-actions{display:flex;gap:8px;flex-wrap:wrap}
@media(max-width:600px){.policy-grid{grid-template-columns:1fr}.policy-url{grid-column:auto}.policy-actions button{flex:1 1 45%}}
#view-device{align-items:start}.policy-actions button,#setupPair{background:var(--paper);border:1px solid #c9d5c1;border-radius:8px;color:#315b40;font-weight:600;min-height:40px;padding:10px 13px}.policy-actions button:hover,#setupPair:hover{background:#e7efdc}.policy-actions #pullStart{background:#315b40;border-color:#315b40;color:white}#setupPair{margin-top:10px}.server-update{scroll-margin-top:20px}.server-update progress{appearance:none;border:0;border-radius:4px;background:#e1e8d9;overflow:hidden}.server-update progress::-webkit-progress-bar{background:#e1e8d9}.server-update progress::-webkit-progress-value{background:#6d8c55}.manual-tools{margin-top:0}.policy-defer input{accent-color:#315b40}
@media(max-width:850px){.server-update{scroll-margin-top:80px}}
.toast{pointer-events:none}

  </style>
</head>
<body>
  <div class="wrap">
    <header class="topbar">
      <a class="brand" href="/" aria-label="mimi.desk · Trang chủ">
        <span class="brandmark"><svg viewBox="0 0 24 24" fill="none" stroke="#f0b09d" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M3 15V5l5 4 4-3 4 3 5-4v10a9 9 0 0 1-18 0Z"/><path d="M8 15h.01M16 15h.01M10 18q2 2 4 0"/></svg></span>
        <span><span class="brandname">mimi.desk</span><sub>Không gian của bạn</sub></span>
      </a>
      <div class="topright"><div class="headnote">SOFTWARE / 10
LOCAL-FIRST DEVICE</div><div class="connection"><span class="led" id="liveLed"></span><span id="liveText">Chờ máy tính</span></div></div>
    </header>

    <div class="intro"><h1 id="workspaceTitle">Tổng quan</h1><p id="workspaceDescription">Một góc bàn dễ chịu. Mọi thứ trong tầm tay.</p></div>
    <nav class="studio-nav" role="tablist" aria-label="Không gian mimi.desk">
      <button id="tab-overview" role="tab" aria-selected="true" aria-controls="view-overview" data-workspace="overview" tabindex="0"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m3 11 9-8 9 8M6 10v10h12V10m-8 10v-6h4v6"/></svg>Tổng quan</button>
      <button id="tab-music" role="tab" aria-selected="false" aria-controls="view-music" data-workspace="music" tabindex="-1"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M9 17V5l11-2v12M9 7l11-2"/><ellipse cx="6" cy="17" rx="3" ry="2"/><ellipse cx="17" cy="15" rx="3" ry="2"/></svg>Âm nhạc</button>
      <button id="tab-focus" role="tab" aria-selected="false" aria-controls="view-focus" data-workspace="focus" tabindex="-1"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><circle cx="12" cy="13" r="8"/><path d="M12 9v5l3 2M9 2h6"/></svg>Tập trung</button>
      <button id="tab-studio" role="tab" aria-selected="false" aria-controls="view-studio" data-workspace="studio" tabindex="-1"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="M10 4v16M10 11h11"/></svg>Studio</button>
      <button id="tab-device" role="tab" aria-selected="false" aria-controls="view-device" data-workspace="device" tabindex="-1"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="5" y="3" width="14" height="18" rx="3"/><path d="M9 17h6"/></svg>Thiết bị</button>
      <span class="sync-mode" id="syncMode" role="status">Đang kết nối…</span>
    </nav>

    <main class="main-grid">
      <section class="pet-card" aria-labelledby="petTitle">
        <div class="pet-top"><span class="eyebrow">Người bạn trên bàn</span><button class="round-icon pet-control" data-action="pet" title="Vuốt ve mèo · cần mã ghép nối" aria-label="Vuốt ve mèo"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M20.8 4.6a5.4 5.4 0 0 0-7.6 0L12 5.8l-1.2-1.2a5.4 5.4 0 0 0-7.6 7.6L12 21l8.8-8.8a5.4 5.4 0 0 0 0-7.6Z"/></svg></button></div>
        <div class="pet-copy"><h2 id="petTitle">Chào ngày mới.<span>Thảnh thơi một chút.</span></h2><p id="moodCopy">Kết nối app mimi.desk trên máy tính để bắt đầu.</p></div>
        <div class="pet-art"><img id="petImage" src="/api/cat.bmp?state=rescue" alt="Mèo mimi.desk"></div>
        <div class="pet-foot"><span class="mood"><span class="dot"></span><span id="moodText">Đang chờ kết nối</span></span><span class="pet-index">MEOW / 010 · <span id="petEnergy">—</span> ⚡</span></div>
      </section>

      <section class="metric-card" aria-label="PC performance data">
        <div class="metric-top"><span class="eyebrow">Thông số máy tính</span><span class="tag" id="dataTag">Chưa có dữ liệu</span></div>
        <div class="cpu-head"><div class="cpu-ring" id="cpuRing"><div class="cpu-inner"><strong id="cpuValue">—</strong><span>CPU</span></div></div><div class="cpu-side"><div class="metric-label">Máy tính Windows</div><h3 id="pcName">Chưa kết nối</h3><p id="cpuDescription">Mở app mimi.desk để đồng bộ thông số máy tính.</p></div></div>
        <div class="bars">
          <div class="barline"><span class="metric-label">GPU</span><strong id="gpuValue">N/A</strong></div><div class="track"><div class="fill gpu-fill" id="gpuBar"></div></div>
          <div class="barline"><span class="metric-label">Bộ nhớ RAM</span><strong id="ramValue">—</strong></div><div class="track"><div class="fill ram-fill" id="ramBar"></div></div>
          <div class="tiny-row"><span id="gpuDetail">Chưa có dữ liệu GPU</span><span id="ramDetail">— / — GB</span></div>
        </div>
      </section>
    </main>

    <section class="history-panel" aria-labelledby="historyTitle">
      <div class="history-top">
        <div><div class="eyebrow">03 — THE PERFORMANCE STORY</div><h2 id="historyTitle">Hoạt động gần đây</h2><p class="history-sub">CPU, GPU và RAM theo thời gian. Khoảng trống là dữ liệu bị thiếu.</p></div>
        <div class="history-ranges" role="group" aria-label="History time range"><button class="range-btn" data-range="60">1 phút</button><button class="range-btn" data-range="180">3 phút</button><button class="range-btn active" data-range="300">5 phút</button></div>
      </div>
      <div class="history-layout">
        <div class="graph-wrap"><canvas id="historyCanvas" role="img" aria-label="CPU GPU and RAM percent history"></canvas><div class="chart-tip" id="chartTip" aria-hidden="true"></div></div>
        <aside class="history-aside" aria-label="Peak percentages in selected time range"><div class="peak cpu"><span>CPU cao nhất</span><strong id="cpuPeak">—</strong></div><div class="peak gpu"><span>GPU cao nhất</span><strong id="gpuPeak">—</strong></div><div class="peak ram"><span>RAM cao nhất</span><strong id="ramPeak">—</strong></div></aside>
      </div>
      <div class="history-foot"><div class="legend"><b>CPU</b><b>GPU</b><b>RAM</b></div><span id="historyMeta" aria-live="polite">WAITING FOR HISTORICAL SAMPLES</span></div>
    </section>

    <div class="section-top"><h2>Make it yours.</h2><small>CONTROL / FROM THIS BROWSER</small></div>
    <div class="lower-grid">
      <section class="control-card" aria-labelledby="controlTitle">
        <h3 class="panel-title" id="controlTitle">Cảnh trên thiết bị</h3>
        <div class="nav" id="pages" aria-label="Choose display page">
          <button class="navlink" data-action="page" data-value="12">HOME ★</button>
          <button class="navlink active" data-action="page" data-value="0">01 / PC</button>
          <button class="navlink" data-action="page" data-value="1">Clock</button>
          <button class="navlink" data-action="page" data-value="2">Focus</button>
          <button class="navlink" data-action="page" data-value="3">Wi-Fi</button>
          <button class="navlink" data-action="page" data-value="4">ESP</button>
          <button class="navlink" data-action="page" data-value="5">Music</button>
          <button class="navlink" data-action="page" data-value="6">Print</button>
          <button class="navlink" data-action="page" data-value="7">Audio</button>
          <button class="navlink" data-action="page" data-value="8">Moto</button>
          <button class="navlink" data-action="page" data-value="9">Sleep</button>
          <button class="navlink" data-action="page" data-value="10">Health</button>
          <button class="navlink" data-action="page" data-value="11">Trends</button>
          <button class="navlink" data-action="page" data-value="13">Network</button>
        </div>
        <div class="actions">
          <button class="act primary" data-action="next" title="Cảnh tiếp theo">→ &nbsp; Cảnh tiếp theo</button>
          <button class="act" data-action="motion" id="motionBtn">◉ &nbsp; Motion ON</button>
          <button class="act" data-action="auto" id="autoBtn">↻ &nbsp; Auto OFF</button>
          <button class="act" data-action="focus" id="focusBtn">Ⅱ &nbsp; Focus</button>
          <button class="act" data-action="focus_reset">↺ &nbsp; Đặt lại Focus</button>
          <button class="act" data-action="focus_skip">⇥ &nbsp; Chuyển giai đoạn</button>
        </div>
      </section>

      <section class="link-card" aria-labelledby="linkTitle">
        <div><div class="eyebrow">03 — DEVICE LINK</div><div class="device-row"><div class="small-square">◈</div><div><h3 id="linkTitle">ESP32-S3 online</h3><small id="deviceSub">Local Wi-Fi bridge</small></div></div>
          <dl class="device-details"><div><dt>Device IP</dt><dd id="deviceIp">—</dd></div><div><dt>Wi-Fi network</dt><dd id="wifiName">—</dd></div><div><dt>Internal heap</dt><dd id="heap">—</dd></div><div><dt>Cat FPS / target</dt><dd id="catFps">—</dd></div><div><dt>PSRAM free</dt><dd id="psramFree">—</dd></div><div><dt>DMA strips</dt><dd id="dmaState">—</dd></div><div><dt>Max cat render</dt><dd id="renderMax">—</dd></div><div><dt>Skipped frames / 5 s</dt><dd id="frameSkipped">—</dd></div></dl>
          <div class="fps-history" aria-label="Measured cat frame rate over the last five minutes"><div class="fps-history-head"><span>FRAME RATE / LAST 5 phút</span><strong id="fpsHistorySummary">WAITING FOR SAMPLES</strong></div><canvas id="fpsHistoryCanvas" aria-label="Measured FPS in coral, target FPS in lilac" role="img"></canvas><div class="fps-history-legend"><span>MEASURED FPS</span><span>TARGET FPS</span></div></div>
        </div>
        <div><div class="eyebrow" style="margin-bottom:8px">Mã ghép nối</div><div class="pair"><input id="pairKey" type="password" autocomplete="off" spellcheck="false" placeholder="Nhập mã của thiết bị" aria-label="Mã ghép nối"><button id="pairSave">Lưu mã</button></div><button id="setupPair" type="button">Lấy mã khi dùng Wi-Fi thiết bị</button><p id="setupResult" role="status">Tìm thiết bị → nhập mã → lưu và kết nối.</p></div>
      </section>
    </div>

    <div class="section-top"><h2>Wellbeing for machines.</h2><small>03 / SMART HEALTH SYSTEM</small></div>
    <div class="health-grid">
      <section class="health-story" aria-labelledby="healthTitle">
        <span class="eyebrow">SENSOR JOURNAL / LIVE WHEN AVAILABLE</span>
        <h3 id="healthTitle">Thông tin<br>hệ thống</h3>
        <div class="health-metrics">
          <div><small>CPU TEMP</small><strong id="cpuTemp">N/A</strong></div>
          <div><small>GPU TEMP</small><strong id="gpuTemp">N/A</strong></div>
          <div><small>PC BATTERY</small><strong id="batteryValue">N/A</strong></div>
        </div>
        <div class="events-heading">LATEST DESK EVENTS / PRIORITIZED</div>
        <ol id="healthEvents" class="events"><li>Waiting for the first health signal...</li></ol>
        <div class="tiny-row"><span id="pcUptime">PC Thời gian hoạt động / —</span><span>REAL DATA OR N/A</span></div>
      </section>

      <section class="health-settings" aria-labelledby="settingsTitle">
        <div class="eyebrow">TUNE THE INSTINCTS</div>
        <h3 id="settingsTitle" class="settings-title">Tuỳ chọn cảnh báo</h3>
        <p class="settings-note">Điều chỉnh ngưỡng cảnh báo và chế độ yên tĩnh. Thiết bị giữ các tuỳ chọn sau khi khởi động lại.</p>
        <div class="setting"><label for="cpuLimit">Cảnh báo CPU</label><output id="cpuLimitOut">88%</output><input id="cpuLimit" type="range" min="50" max="99" value="88"></div>
        <div class="setting"><label for="gpuLimit">Cảnh báo GPU</label><output id="gpuLimitOut">92%</output><input id="gpuLimit" type="range" min="50" max="99" value="92"></div>
        <div class="setting"><label for="tempLimit">Cảnh báo nhiệt độ</label><output id="tempLimitOut">80°C</output><input id="tempLimit" type="range" min="55" max="110" value="80"></div>
        <div class="setting"><label for="batteryLimit">Cảnh báo pin yếu</label><output id="batteryLimitOut">20%</output><input id="batteryLimit" type="range" min="5" max="50" value="20"></div>
        <label class="quiet-row"><input id="quietMode" type="checkbox"> Chế độ yên tĩnh · vẫn giữ cảnh báo khẩn cấp</label>
        <button id="settingsSave" class="settings-save">Lưu tuỳ chọn</button>
      </section>
    </div>

    <div class="section-top"><h2>Find your rhythm.</h2><small>04 / FOCUS + AMBIENT MODE</small></div>
    <div class="living-grid">
      <section class="focus-studio" aria-labelledby="focusTitle">
        <span class="eyebrow">DEEP WORK / YOUR TINY TIMEKEEPER</span>
        <h2 id="focusTitle" class="focus-clock" aria-live="off">25:00</h2>
        <div class="focus-sub"><span id="focusPhase">READY TO FOCUS</span> · <span id="focusSessions">0 SESSIONS</span></div>
        <div class="focus-track"><div id="focusBar"></div></div>
        <div class="actions"><button class="act primary" id="focusHero" data-focus="toggle">Bắt đầu / tạm dừng</button><button class="act" data-focus="reset">Đặt lại</button><button class="act" data-focus="skip">Chuyển giai đoạn</button></div>
      </section>
      <section class="living-info" aria-labelledby="ambientTitle">
        <span class="eyebrow">SYSTEM / REAL-TIME OR N/A</span>
        <h3 id="ambientTitle">Giao diện và chuyển động</h3>
        <div class="ambient-metrics"><div><small>NETWORK DOWNLOAD</small><strong id="netDown">N/A</strong></div><div><small>NETWORK UPLOAD</small><strong id="netUp">N/A</strong></div><div><small>GPU VRAM USED</small><strong id="vramUsed">N/A</strong></div><div><small>GPU VRAM TOTAL</small><strong id="vramTotal">N/A</strong></div></div>
        <span class="eyebrow">Giao diện TFT</span>
        <div class="theme-buttons" role="group" aria-label="TFT color theme"><button data-action="theme" data-value="0">☀ Cozy Day</button><button data-action="theme" data-value="1">☾ Midnight</button><button data-action="theme" data-value="2">◴ AUTO</button><button data-action="theme" data-value="3">CYBERPUNK</button><button data-action="theme" data-value="4">SAKURA</button><button data-action="theme" data-value="5">MINIMAL</button></div>
        <p class="wifi-note" style="margin-top:11px">AUTO follows the device local time when NTP is available. Manual modes always work.</p>
        <span class="eyebrow">Độ mượt chuyển động</span>
        <div class="theme-buttons" role="group" aria-label="Cat animation target FPS"><button data-action="fps" data-value="0">Ổn định · 20</button><button data-action="fps" data-value="1">Cân bằng · 25</button><button data-action="fps" data-value="2">Mượt · 30</button><button data-action="fps" data-value="3">◈ Tự động · 20–25</button></div>
        <p class="wifi-note" style="margin-top:11px">AUTO adapts between 20 and 25 FPS after sustained load changes. It never switches to 30 FPS automatically. See actual FPS and missed frames in Device Link.</p>
        <span class="eyebrow">Tính cách chuyển động</span>
        <div class="theme-buttons" role="group" aria-label="Cat motion personality"><button data-action="motion_style" data-value="0">Êm dịu</button><button data-action="motion_style" data-value="1">Tự nhiên</button><button data-action="motion_style" data-value="2">Tinh nghịch</button></div>
        <div class="motion-signal" aria-live="off"><span class="motion-state" id="motionState">COZY / WAITING</span><span id="motionCel">CEL 0 / 3</span><span id="motionGuard">FPS GUARD / NORMAL</span><span id="motionSignal">REAL PC / N-A</span></div>
        <p class="wifi-note" style="margin-top:9px">Natural blinks, dance timing, quiet Focus and alert cues follow live Windows data. FPS Guard drops extra effects when rendering is overloaded. Frame 2 looks like a real blink only with authored closed-eye art.</p>
      </section>
    </div>

    <section class="scene-studio" aria-labelledby="sceneTitle">
      <div><div class="eyebrow">05 — YOUR FOUR FAVORITE SCENES</div><h3 id="sceneTitle">Cảnh yêu thích</h3><p>Chọn bốn cảnh khác nhau. Chế độ tự chuyển cảnh luân phiên các cảnh này.</p><span class="scene-tip">Lưu trực tiếp trên thiết bị</span></div>
      <div class="scene-selects"><label>Cảnh yêu thích<select id="sceneSlot0" aria-label="First favorite scene"></select></label><label>Cảnh yêu thích<select id="sceneSlot1" aria-label="Second favorite scene"></select></label><label>Cảnh yêu thích<select id="sceneSlot2" aria-label="Third favorite scene"></select></label><label>Cảnh yêu thích<select id="sceneSlot3" aria-label="Fourth favorite scene"></select></label></div>
    </section>

    <div class="mood-intro"><h2>Meet the eight moods.</h2><p>TAP TO PREVIEW ON YOUR TFT / 8 SECONDS</p></div>
    <div class="mood-gallery" aria-label="Preview the eight cat states">
      <button class="mood-tile" data-preview="0"><img loading="lazy" src="/api/cat.bmp?state=paused&size=72" alt="Paused cat"><b>PAUSED</b><small>quiet time</small></button>
      <button class="mood-tile" data-preview="1"><img loading="lazy" src="/api/cat.bmp?state=low_battery&size=72" alt="Low battery cat"><b>BATTERY</b><small>power low</small></button>
      <button class="mood-tile" data-preview="2"><img loading="lazy" src="/api/cat.bmp?state=rescue&size=72" alt="Searching cat"><b>RESCUE</b><small>PC offline</small></button>
      <button class="mood-tile" data-preview="3"><img loading="lazy" src="/api/cat.bmp?state=power&size=72" alt="Power cat"><b>POWER</b><small>on charger</small></button>
      <button class="mood-tile" data-preview="4"><img loading="lazy" src="/api/cat.bmp?state=sleep&size=72" alt="Sleeping cat"><b>SLEEP</b><small>low load</small></button>
      <button class="mood-tile" data-preview="5"><img loading="lazy" src="/api/cat.bmp?state=hot&size=72" alt="Feverish cat"><b>HOT</b><small>high temp</small></button>
      <button class="mood-tile" data-preview="6"><img loading="lazy" src="/api/cat.bmp?state=healthy&size=72" alt="Healthy cat"><b>HEALTHY</b><small>all good</small></button>
      <button class="mood-tile" data-preview="7"><img loading="lazy" src="/api/cat.bmp?state=hotspot&size=72" alt="Wi-Fi cat"><b>HOTSPOT</b><small>fallback AP</small></button>
    </div>

    <div class="scene-shortcuts" aria-label="Quick scene shortcuts">
      <button data-action="page" data-value="12">⌂ HOME</button>
      <button data-action="page" data-value="0">▥ PC MONITOR</button>
      <button data-action="page" data-value="5">♫ NOW PLAYING</button>
      <button data-action="page" data-value="2">◷ FOCUS</button>
      <button data-action="page" data-value="13">⌁ NETWORK</button>
    </div>

    <div class="feature-grid">
      <section class="music-studio" aria-labelledby="musicTitle">
        <div class="music-hero">
          <div class="music-artwork">
            <img class="album-image" id="albumCoverWeb" alt="Ảnh bìa từ Windows Media" hidden>
            <svg viewBox="0 0 100 100" fill="none" stroke="currentColor" stroke-width="3" aria-hidden="true"><circle cx="50" cy="50" r="38"/><circle cx="50" cy="50" r="25"/><circle cx="50" cy="50" r="7"/></svg>
          </div>
          <div class="music-copy"><h2 id="musicTitle">Chưa phát nhạc</h2>
          <div class="artist" id="musicArtist">Mở Spotify hoặc trình phát trên PC.</div></div>
        </div>
        <div class="track"><div id="musicBar"></div></div>
        <div class="music-time" id="musicTime">— / — · NOT AVAILABLE</div>
        <div class="music-actions">
          <button data-media="media_previous" title="Bài trước" aria-label="Bài trước"><svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M4 4h3v16H4zM20 4v16L8 12z"/></svg></button>
          <button class="main" data-media="media_play_pause" title="Phát / dừng" aria-label="Phát / dừng"><svg class="pause-icon" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M6 4h4v16H6zM14 4h4v16h-4z"/></svg><svg class="play-icon" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M7 3v18l15-9z"/></svg></button>
          <button data-media="media_next" title="Bài tiếp" aria-label="Bài tiếp"><svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M17 4h3v16h-3zM4 4v16l12-8z"/></svg></button>
          <span class="music-status" id="musicCommand">Điều khiển qua app trên PC</span>
        </div>
      </section>

      <section class="wifi-studio" aria-labelledby="wifiTitle">
        <div class="eyebrow">07 — NETWORK / DEVICE SETTINGS</div>
        <h2 id="wifiTitle">Kết nối Wi-Fi</h2>
        <p class="wifi-note">Thiết lập Wi-Fi 2.4 GHz trực tiếp trên ESP32. Hotspot dự phòng vẫn bật để sửa nếu nhập sai mạng.</p>
        <label for="newSsid">Tên mạng Wi-Fi</label>
        <input id="newSsid" type="text" maxlength="32" autocomplete="off" placeholder="Tên Wi-Fi 2.4 GHz">
        <label for="newPassword">Mật khẩu Wi-Fi</label>
        <input id="newPassword" type="password" autocomplete="new-password" maxlength="63" placeholder="Để trống nếu mạng mở">
        <button id="wifiSave">Lưu Wi-Fi</button>
        <button id="wifiScan" type="button" style="background:transparent;color:#5a5365;border:1px solid #cdbdd3">Tìm mạng Wi-Fi</button>
        <p class="wifi-note" id="wifiScanResult">Fallback AP vẫn hoạt động tại 192.168.4.1.</p>
      </section>
    </div>

    <section class="ota-panel server-update" aria-labelledby="serverUpdateTitle">
      <div><h3 id="serverUpdateTitle">Cập nhật từ server</h3><p>Thiết bị tự kiểm tra và tải firmware. Chính sách tiếp tục hoạt động khi đóng app hoặc tắt máy tính.</p><p id="pullNotice" role="status">Đọc chính sách từ thiết bị để bắt đầu.</p><p id="pullVersion">Chưa kiểm tra phiên bản mới.</p><p id="pullStatus" role="status"></p><p id="pullNotes" class="release-notes"></p><progress id="pullProgress" max="100" value="0" aria-label="Tiến độ tải firmware"></progress></div>
      <form id="pullForm"><div class="policy-grid">
        <label class="policy-url" for="pullUrl">Địa chỉ manifest<input id="pullUrl" type="url" maxlength="511" placeholder="https://…/channels/preview.json" autocomplete="off"></label>
        <label for="pullMode">Chế độ<select id="pullMode"><option value="notify">Kiểm tra và thông báo</option><option value="auto">Tự cập nhật</option></select></label>
        <label for="pullChannel">Kênh<select id="pullChannel"><option value="preview">Preview</option><option value="stable">Stable</option></select></label>
        <label for="pullInterval">Kiểm tra mỗi · giây<input id="pullInterval" type="number" min="60" max="86400" step="60" value="120"></label>
        <label class="policy-defer"><input id="pullDefer" type="checkbox" checked> Hoãn khi đang tập trung</label>
      </div><div class="policy-actions"><button id="pullSave" type="submit">Lưu chính sách</button><button id="pullReload" type="button">Đọc lại</button><button id="pullCheck" type="button">Kiểm tra ngay</button><button id="pullStart" type="button">Cập nhật</button></div></form>
    </section>
    <section class="ota-panel manual-ota" aria-label="Cập nhật firmware qua Wi-Fi">
      <div><span class="eyebrow">08 — OTA GUARDIAN</span><h3>Cập nhật<br>firmware thiết bị</h3><p>Chọn firmware đã tải về. Sau khi khởi động lại, thiết bị sẽ được kiểm tra trước khi báo hoàn tất.</p><div class="ota-checks"><div><small>Phiên bản hiện tại</small><strong id="otaVersion">CHECKING…</strong></div><div><small>Phân vùng cập nhật</small><strong id="otaSlot">CHECKING…</strong></div><div><small>Trạng thái</small><strong id="otaReady">CHECKING…</strong></div><div><small>Khả năng phục hồi</small><strong id="otaRollback">CHECKING…</strong></div><div><small>Kiểm tra khởi động</small><strong id="otaBootHealth">CHECKING…</strong></div><div><small>Thời gian hoạt động</small><strong id="otaUptime">CHECKING…</strong></div></div></div>
      <div class="ota-upload"><label for="otaFile">File firmware · .bin</label><input id="otaFile" type="file" accept=".bin,application/octet-stream"><button id="otaStart" disabled>Cập nhật firmware</button><div class="ota-progress"><div id="otaProgress"></div></div><p class="ota-result" id="otaResult" role="status">Đang chờ trạng thái thiết bị.</p><p class="ota-warning">Cập nhật qua Wi-Fi nội bộ. Giữ nguồn ổn định trong khi gửi và khởi động lại.</p></div>
    </section>

    <section class="ota-panel catpack-panel" aria-label="Animated cat sprite packs over local Wi-Fi">
      <div><span class="eyebrow">09 — MOTION PACK STUDIO</span><h3>Bộ chuyển động<br>của mèo</h3><p>16 cat moods · up to 3 hand-drawn animation frames each. Keep your existing 648 KiB collections, or install a 1,944 KiB Motion Pack. Both stay in PSRAM and use the original internal-RAM DMA strips; interrupted uploads leave the previous bank intact.</p><div class="ota-checks"><div><small>Bộ đang dùng</small><strong id="catPackSource">CHECKING…</strong></div><div><small>Bản sao lưu</small><strong id="catPackGen">CHECKING…</strong></div><div><small>Bộ nhớ thiết bị</small><strong id="catPackReady">CHECKING…</strong></div><div><small>Chuyển động</small><strong id="catPackMotion">CHECKING…</strong></div></div></div>
      <div class="ota-upload"><label for="catPackFile">STATIC 648 KiB / MOTION 1,944 KiB · .MEOCAT</label><input id="catPackFile" type="file" accept=".meocat,application/octet-stream"><button id="catPackStart" disabled>✦ INSTALL CAT MOTION PACK</button><div class="ota-progress"><div id="catPackProgress"></div></div><p class="ota-result" id="catPackResult" role="status">Checking your cat collection…</p><p class="ota-warning">Use make_catpack.py --motion-demo for a ready-to-test animated collection, or --motion with your own 48 PNG frames. Reboots once; requires private LAN + pairing key.</p></div>
    </section>

    <footer class="status-strip"><span class="signature">mimi.desk</span><span> · <span id="footerStatus">WAITING FOR SIGNAL</span> • mimi.desk</span><span>Phiên bản • <span id="firmwareBuild">DEV</span></span></footer>
  </div>
  <div id="toast" role="status" aria-live="polite"></div>
  <script>
    (function(){
      'use strict';
      const $=(id)=>document.getElementById(id);
      const workspaceGroups={overview:['.main-grid','.history-panel','.health-story'],studio:['.control-card','.living-info','.scene-studio','.mood-gallery','.catpack-panel'],music:['.music-studio'],focus:['.focus-studio'],device:['.link-card','.health-settings','.wifi-studio','.ota-panel:not(.catpack-panel)']};
      for(const [name,selectors] of Object.entries(workspaceGroups)){
        const panel=document.createElement('div');panel.id='view-'+name;panel.className='workspace-panel';panel.setAttribute('role','tabpanel');panel.setAttribute('aria-labelledby','tab-'+name);panel.hidden=name!=='overview';
        document.querySelector('.wrap').insertBefore(panel,document.querySelector('.status-strip'));
        selectors.forEach(selector=>document.querySelectorAll(selector).forEach(node=>panel.append(node)));
        panel.setAttribute('tabindex','0');
      }
      document.querySelectorAll('.section-top,.mood-intro,.lower-grid,.health-grid,.living-grid,.feature-grid,[aria-label="Quick scene shortcuts"]').forEach(node=>node.remove());
      document.querySelector('.health-story').append(document.querySelector('.ambient-metrics'));
      const systemDetails=document.createElement('details');systemDetails.className='system-details';const systemSummary=document.createElement('summary');systemSummary.textContent='Sự kiện và cảm biến';systemDetails.append(systemSummary);const systemStory=document.querySelector('.health-story');systemStory.before(systemDetails);systemDetails.append(systemStory);
      const motionHelp=document.createElement('details');motionHelp.className='motion-help';
      const helpSummary=document.createElement('summary');helpSummary.textContent='Giao diện, chuyển động và hiệu năng';motionHelp.append(helpSummary);
      document.querySelectorAll('.living-info .wifi-note').forEach(note=>motionHelp.append(note));document.querySelector('.living-info').append(motionHelp);
      document.querySelectorAll('.eyebrow').forEach(label=>{if(label.closest('.pet-top')||(label.nextElementSibling&&/^H[1-6]$/.test(label.nextElementSibling.tagName)))label.remove();});
      const labPanel=document.createElement('section');labPanel.className='lab-panel';labPanel.setAttribute('aria-labelledby','labTitle');
      labPanel.innerHTML='<div class="lab-head"><h3 id="labTitle">Performance Lab</h3><button id="labToggle" class="act" data-action="perf_trace" data-value="1" disabled>Bắt đầu đo</button></div><p id="labStatus">Waiting for a compatible firmware.</p><dl class="lab-metrics"><div><dt>Strip CPU / cat frame</dt><dd id="labCpu">N/A</dd></div><div><dt>DMA submit / strip</dt><dd id="labSubmit">N/A</dd></div><div><dt>DMA wait / strip</dt><dd id="labWait">N/A</dd></div><div><dt>Cat wall time / frame</dt><dd id="labWall">N/A</dd></div><div><dt>Card wall / call</dt><dd id="labCard">N/A</dd></div><div><dt>Bridge max / loop</dt><dd id="labBridge">N/A</dd></div><div><dt>Tracked pixel payload</dt><dd id="labBytes">N/A</dd></div><div><dt>Window duration</dt><dd id="labWindow">N/A</dd></div></dl><p>DMA wait measures CPU stalls, not total SPI bus activity. Payload counts cat strips, buffered cards and album tiles only; direct text, commands and protocol overhead are excluded. Tracing is optional and resets when toggled.</p>';
      const diagnostics=document.createElement('details');diagnostics.className='diagnostics';const diagSummary=document.createElement('summary');diagSummary.textContent='Chẩn đoán kết nối và hiệu năng';diagnostics.append(diagSummary,$('syncMode'),labPanel);const fpsBlock=document.querySelector('.fps-history');if(fpsBlock)diagnostics.append(fpsBlock);document.querySelector('.link-card').append(diagnostics);
      const configPanel=document.createElement('section');configPanel.className='lab-panel';
      configPanel.innerHTML='<h3>Sao lưu cấu hình</h3><p>Sao lưu cảnh, giao diện, Focus và cảnh báo. Không bao gồm mật khẩu Wi-Fi hoặc mã ghép nối.</p><div class="actions"><button class="act" id="configExport">Xuất cấu hình</button><input id="configFile" type="file" accept=".json,application/json" aria-label="Configuration JSON"><button class="act" id="configRestore">Khôi phục cấu hình</button></div><p id="configResult" role="status">File được kiểm tra trước khi lưu. Đổi thời lượng Focus sẽ đặt lại phiên hiện tại.</p><details><summary>Lịch sử cập nhật</summary><button class="act" id="otaHistoryRefresh">Read device history</button><ol id="otaHistoryList"><li>No history loaded.</li></ol></details>';
      $('view-device').append(configPanel);
      const notifyPanel=document.createElement('details');notifyPanel.className='lab-panel';
      notifyPanel.innerHTML='<summary>Thông báo thiết bị</summary><p>Send a short notice for eight seconds. Quiet mode suppresses notices; critical device alerts keep priority.</p><div class="composer-fields"><label>Notice · ASCII<input id="noticeText" maxlength="60" value="Desk reminder"></label><label><input id="noticeHidden" type="checkbox" checked>Hide content · send a generic notice</label></div><div class="composer-actions"><button id="noticeSend" disabled>Send notice</button></div><p id="noticeResult" role="status">Requires compatible firmware.</p>';
      $('view-device').append(notifyPanel);
      // NOTIFICATION_CONTRACT_BEGIN
      function prepareNotification(text,hidden){
        if(!hidden&&(typeof text!=='string'||text.length<1||text.length>60||/[^ -~]/.test(text)))throw Error('Use 1–60 printable ASCII characters for the TFT label.');
        return {source:'web',text:hidden?'New notification':text,hidden,ttl_s:8,priority:1};
      }
      // NOTIFICATION_CONTRACT_END
      $('noticeSend').onclick=async()=>{try{const result=await keyedJson('/api/v1/notifications',prepareNotification($('noticeText').value,$('noticeHidden').checked));$('noticeResult').textContent=result.suppressed?'Quiet mode suppressed this notice.':'Notice accepted · eight-second TTL.';}catch(e){$('noticeResult').textContent=e.message;}};

      const focusConfig=document.createElement('form');focusConfig.className='focus-config';
      focusConfig.innerHTML='<label>Tập trung · phút<input id="focusWorkMin" type="number" min="1" max="180" value="25" required></label><label>Nghỉ · phút<input id="focusBreakMin" type="number" min="1" max="60" value="5" required></label><label>Số phiên<input id="focusGoal" type="number" min="1" max="24" value="4" required></label><button class="act" type="submit">Lưu kế hoạch</button>';
      document.querySelector('.focus-studio').append(focusConfig);
      let focusConfigDirty=false;
      focusConfig.addEventListener('input',()=>focusConfigDirty=true);
      focusConfig.addEventListener('submit',async event=>{event.preventDefault();try{await keyedJson('/api/v1/focus/config',{work_min:Number($('focusWorkMin').value),break_min:Number($('focusBreakMin').value),goal:Number($('focusGoal').value)});focusConfigDirty=false;toast('Đã lưu thời lượng Focus. Phiên hiện tại được đặt lại.');streamSeenAt=0;poll();}catch(error){toast(error.message);}});

      const composer=document.createElement('section');composer.className='scene-composer';
      composer.innerHTML=`<h3>Bố cục widget</h3><p>Kéo widget hoặc nhập vị trí để sắp xếp vùng nội dung TFT. Mèo có vùng riêng; đây là bản xem trước bố cục.</p><div class="composer-body"><canvas id="sceneCanvas" class="composer-canvas" width="324" height="240" tabindex="0" aria-label="Widget layout, 162 by 120 pixels. Select a widget and use arrow keys to move it."></canvas><div class="composer-fields"><label class="wide">Widget<select id="widgetSelect"></select></label><label>Type<select id="widgetKind"></select></label><label>Nhãn · không dấu<input id="widgetLabel" maxlength="31"></label><label>X<input id="widgetX" type="number" min="0" max="161"></label><label>Y<input id="widgetY" type="number" min="0" max="119"></label><label>Rộng<input id="widgetW" type="number" min="32" max="162"></label><label>Cao<input id="widgetH" type="number" min="20" max="120"></label></div></div><div class="composer-actions"><button id="widgetAdd">Thêm widget</button><button id="widgetRemove">Xoá</button><button id="sceneDefault">Mặc định</button><button id="sceneClock">Đồng hồ lớn</button><button id="sceneImport">Nhập JSON</button><button id="sceneExport">Xuất JSON</button><button id="sceneReload">Đọc lại thiết bị</button><button id="sceneApply" class="save-layout">Lưu lên thiết bị</button></div><input id="sceneFile" type="file" accept=".json,application/json" hidden><p id="sceneMessage" class="scene-message" role="status">Local draft · not applied.</p>`;
      $('view-studio').prepend(composer);
      const widgetSceneButton=document.createElement('button');widgetSceneButton.className='navlink';widgetSceneButton.dataset.action='page';widgetSceneButton.dataset.value='14';widgetSceneButton.textContent='14 / Widgets';$('pages').append(widgetSceneButton);
      const widgetKinds=['Clock','CPU','GPU','RAM','Focus','Music','Network','Text','Data 1','Data 2','Data 3'];
      const defaultLayout={schema:1,widgets:[{kind:0,x:0,y:0,w:162,h:40,label:'Clock'},{kind:1,x:0,y:42,w:80,h:36,label:'CPU'},{kind:3,x:82,y:42,w:80,h:36,label:'RAM'},{kind:4,x:0,y:80,w:162,h:40,label:'Focus'}]};
      // SCENE_CONTRACT_BEGIN
      function validateScene(data){
        if(!data||data.schema!==1||!Array.isArray(data.widgets)||data.widgets.length<1||data.widgets.length>6)throw Error('Choose 1–6 widgets.');
        const widgets=[];
        for(const w of data.widgets){
          if(!w||Object.keys(w).sort().join(',')!=='h,kind,label,w,x,y')throw Error('Invalid widget fields.');
          for(const [key,lo,hi] of [['kind',0,10],['x',0,161],['y',0,119],['w',32,162],['h',20,120]])if(!Number.isInteger(w[key])||w[key]<lo||w[key]>hi)throw Error('Invalid '+key+'.');
          if(w.x+w.w>162||w.y+w.h>120)throw Error('Keep widgets inside 162 × 120.');
          if(typeof w.label!=='string'||w.label.length>31||/[^ -~]/.test(w.label))throw Error('Use up to 31 printable ASCII characters for TFT labels.');
          for(const a of widgets)if(w.x<a.x+a.w&&a.x<w.x+w.w&&w.y<a.y+a.h&&a.y<w.y+w.h)throw Error('Widgets overlap. Move or resize one.');
          widgets.push({...w});
        }
        return {schema:1,widgets};
      }
      // SCENE_CONTRACT_END
      let sceneDraft=structuredClone(defaultLayout),sceneIndex=0,sceneDirty=false,sceneBusy=false,sceneSeen='',sceneRetryAt=0,sceneDrag=null,studioCas=false,sceneBase=null;
      widgetKinds.forEach((name,index)=>{const option=document.createElement('option');option.value=index;option.textContent=name;$('widgetKind').append(option);});
      function drawScene(){
        const g=$('sceneCanvas').getContext('2d');g.clearRect(0,0,324,240);g.fillStyle='#f7f2e9';g.fillRect(0,0,324,240);
        sceneDraft.widgets.forEach((w,index)=>{g.save();g.beginPath();g.rect(w.x*2+1,w.y*2+1,w.w*2-2,w.h*2-2);g.clip();g.fillStyle=index===sceneIndex?'#d8f1e2':'#fffefa';g.fillRect(w.x*2,w.y*2,w.w*2,w.h*2);g.strokeStyle=index===sceneIndex?'#315a46':'#bcb7ac';g.lineWidth=index===sceneIndex?2:1;g.strokeRect(w.x*2+1,w.y*2+1,w.w*2-2,w.h*2-2);g.fillStyle='#314438';g.font='13px sans-serif';g.fillText(w.label||widgetKinds[w.kind],w.x*2+8,w.y*2+19);g.restore();});
      }
      function sceneCheck(){try{validateScene(sceneDraft);$('sceneApply').disabled=false;$('sceneMessage').textContent=sceneDirty?'Valid local draft · not saved.':'Đã đọc bố cục · 162 × 120.';return true;}catch(e){$('sceneApply').disabled=true;$('sceneMessage').textContent=e.message;return false;}}
      function selectWidget(){const w=sceneDraft.widgets[sceneIndex];$('widgetKind').value=w.kind;$('widgetLabel').value=w.label;for(const key of ['x','y','w','h'])$('widget'+key.toUpperCase()).value=w[key];drawScene();}
      function refreshScene(){const select=$('widgetSelect');select.replaceChildren();sceneDraft.widgets.forEach((w,i)=>{const option=document.createElement('option');option.value=i;option.textContent=(i+1)+'. '+(w.label||widgetKinds[w.kind]);select.append(option);});sceneIndex=Math.min(sceneIndex,sceneDraft.widgets.length-1);select.value=sceneIndex;selectWidget();sceneCheck();}
      $('widgetSelect').onchange=()=>{sceneIndex=Number($('widgetSelect').value);selectWidget();};
      for(const name of ['Kind','Label','X','Y','W','H'])$('widget'+name).addEventListener('input',()=>{const w=sceneDraft.widgets[sceneIndex];w.kind=Number($('widgetKind').value);w.label=$('widgetLabel').value;for(const key of ['x','y','w','h']){const input=$('widget'+key.toUpperCase());w[key]=input.value===''?NaN:Number(input.value);}$('widgetSelect').selectedOptions[0].textContent=(sceneIndex+1)+'. '+(w.label||widgetKinds[w.kind]);sceneDirty=true;drawScene();sceneCheck();});
      function sceneMove(x,y){const candidate=structuredClone(sceneDraft),w=candidate.widgets[sceneIndex];w.x=Math.min(162-w.w,Math.max(0,Math.round(x/2)*2));w.y=Math.min(120-w.h,Math.max(0,Math.round(y/2)*2));try{validateScene(candidate);}catch(_){return;}sceneDraft=candidate;sceneDirty=true;selectWidget();sceneCheck();}
      function scenePoint(e){const r=$('sceneCanvas').getBoundingClientRect();return {x:(e.clientX-r.left)*162/r.width,y:(e.clientY-r.top)*120/r.height};}
      $('sceneCanvas').onpointerdown=e=>{const p=scenePoint(e),i=sceneDraft.widgets.findIndex(w=>p.x>=w.x&&p.x<w.x+w.w&&p.y>=w.y&&p.y<w.y+w.h);if(i<0)return;sceneIndex=i;$('widgetSelect').value=i;selectWidget();sceneDrag={x:p.x-sceneDraft.widgets[i].x,y:p.y-sceneDraft.widgets[i].y};$('sceneCanvas').setPointerCapture(e.pointerId);};
      $('sceneCanvas').onpointermove=e=>{if(!sceneDrag)return;const p=scenePoint(e);sceneMove(p.x-sceneDrag.x,p.y-sceneDrag.y);};
      $('sceneCanvas').onpointerup=$('sceneCanvas').onpointercancel=()=>{sceneDrag=null;};
      $('sceneCanvas').onkeydown=e=>{const delta={ArrowLeft:[-2,0],ArrowRight:[2,0],ArrowUp:[0,-2],ArrowDown:[0,2]}[e.key];if(!delta)return;e.preventDefault();const w=sceneDraft.widgets[sceneIndex];sceneMove(w.x+delta[0],w.y+delta[1]);};
      $('widgetAdd').onclick=()=>{if(sceneDraft.widgets.length>=6){$('sceneMessage').textContent='Maximum 6 widgets.';return;}for(let y=0;y<=100;y+=2)for(let x=0;x<=130;x+=2){const candidate=structuredClone(sceneDraft);candidate.widgets.push({kind:7,x,y,w:32,h:20,label:'Text'});try{validateScene(candidate);}catch(_){continue;}sceneDraft=candidate;sceneIndex=candidate.widgets.length-1;sceneDirty=true;refreshScene();return;}$('sceneMessage').textContent='No free 32 × 20 area. Resize or remove a widget first.';};
      $('widgetRemove').onclick=()=>{if(sceneDraft.widgets.length<=1)return;sceneDraft.widgets.splice(sceneIndex,1);sceneDirty=true;refreshScene();};
      $('sceneDefault').onclick=()=>{sceneDraft=structuredClone(defaultLayout);sceneDirty=true;refreshScene();};
      $('sceneClock').onclick=()=>{sceneDraft={schema:1,widgets:[{kind:0,x:0,y:0,w:162,h:120,label:'Clock'}]};sceneIndex=0;sceneDirty=true;refreshScene();};
      $('sceneImport').onclick=()=>$('sceneFile').click();
      $('sceneFile').onchange=async()=>{const file=$('sceneFile').files[0];if(!file)return;try{if(file.size>8192)throw Error('Scene file exceeds 8 KiB.');sceneDraft=validateScene(JSON.parse((await file.text()).replace(/^\uFEFF/,'')));sceneIndex=0;sceneDirty=true;refreshScene();}catch(e){$('sceneMessage').textContent=e.message;}finally{$('sceneFile').value='';}};
      $('sceneExport').onclick=()=>{if(!sceneCheck())return;const url=URL.createObjectURL(new Blob([JSON.stringify(validateScene(sceneDraft),null,2)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='meodesk-scene.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
      async function loadScene(force=false){if(sceneBusy)return;sceneBusy=true;const before=JSON.stringify(sceneDraft);try{const raw=await keyedJson('/api/v1/scene-layout'),data=validateScene(raw);if(sceneDirty&&(!force||before!==JSON.stringify(sceneDraft))){$('sceneMessage').textContent='Bố cục đã đổi trên thiết bị. Bản nháp được giữ; bấm Đọc lại thiết bị để thay thế.';return;}sceneBase=Number.isInteger(raw.revision)&&typeof raw.state_boot_id==='string'?{expected_revision:raw.revision,expected_boot_id:raw.state_boot_id}:null;sceneDraft=data;sceneIndex=0;sceneDirty=false;refreshScene();}catch(e){sceneRetryAt=Date.now()+5000;sceneSeen='';$('sceneMessage').textContent=e.message;}finally{sceneBusy=false;}}
      $('sceneReload').onclick=()=>loadScene(true);
      $('sceneApply').onclick=async()=>{if(sceneBusy||!sceneCheck())return;sceneBusy=true;$('sceneApply').disabled=true;const submitted=validateScene(sceneDraft);try{if(studioCas&&!sceneBase)throw Error('Đọc lại bố cục từ thiết bị trước khi lưu.');const result=await keyedJson('/api/v1/scene-layout',studioCas?{...submitted,...sceneBase}:submitted);sceneBase=Number.isInteger(result.revision)?{expected_revision:result.revision,expected_boot_id:result.state_boot_id}:null;if(JSON.stringify(submitted)===JSON.stringify(sceneDraft))sceneDirty=false;await control('page',14);$('sceneMessage').textContent=sceneDirty?'Đã lưu. Các thay đổi mới trong bản nháp chưa được gửi.':'Đã lưu bố cục trên thiết bị.';}catch(e){$('sceneMessage').textContent=e.message;}finally{sceneBusy=false;$('sceneApply').disabled=false;}};
      function syncScene(data){studioCas=data.capabilities?.includes('studio_cas_v1')||false;if(!pair.value.trim()||!data.capabilities?.includes('widget_scene_v1')||sceneBusy||Date.now()<sceneRetryAt)return;const revision=String(data.state_boot_id)+':'+data.scene_layout_revision;if(revision!==sceneSeen){sceneSeen=revision;loadScene();}}
      refreshScene();

      const rulesPanel=document.createElement('details');rulesPanel.className='rule-editor';
      rulesPanel.innerHTML='<summary>Quy tắc tự động · JSON nâng cao</summary><p>Up to six rules. Conditions use real CPU/GPU percentages, idle seconds, local hour, Focus completion or music. Actions select a scene (0–14), theme (0–5), celebrate or notify. Changes run only after Save.</p><label for="rulesDraft">Rule JSON</label><textarea id="rulesDraft" spellcheck="false"></textarea><div class="composer-actions"><button id="rulesPreset">Load example</button><button id="rulesEmpty">Disable all</button><button id="rulesImport">Nhập JSON</button><button id="rulesExport">Xuất JSON</button><button id="rulesReload">Đọc lại thiết bị</button><button id="rulesApply" class="save-layout">Save rules</button></div><input id="rulesFile" type="file" accept=".json,application/json" hidden><p id="rulesResult" role="status">Local draft · not applied.</p>';
      $('view-studio').append(rulesPanel);
      const rulesPreset={schema:1,rules:[{enabled:true,when:'cpu_above',threshold:90,hold_s:30,cooldown_s:300,do:'notify',value:0,label:'CPU busy'},{enabled:true,when:'hour',threshold:23,hold_s:0,cooldown_s:3600,do:'theme',value:1,label:'Night desk'}]};
      // RULE_CONTRACT_BEGIN
      function validateRules(data){
        if(!data||data.schema!==1||!Array.isArray(data.rules)||data.rules.length>6)throw Error('Use schema 1 and at most six rules.');
        const rules=[];
        for(const r of data.rules){
          if(!r||Object.keys(r).sort().join(',')!=='cooldown_s,do,enabled,hold_s,label,threshold,value,when')throw Error('Invalid rule fields.');
          if(typeof r.enabled!=='boolean'||!['cpu_above','gpu_above','hour','focus_complete','idle_above','music_playing'].includes(r.when)||!['scene','theme','celebrate','notify'].includes(r.do))throw Error('Unknown condition or action.');
          for(const [key,low,high] of [['threshold',0,7200],['hold_s',0,600],['cooldown_s',5,86400],['value',0,14]])if(!Number.isInteger(r[key])||r[key]<low||r[key]>high)throw Error('Invalid '+key+'.');
          if((['cpu_above','gpu_above'].includes(r.when)&&(r.threshold<1||r.threshold>100))||(r.when==='hour'&&r.threshold>23)||(r.when==='idle_above'&&r.threshold<1)||(['focus_complete','music_playing'].includes(r.when)&&r.threshold!==0)||(r.when==='focus_complete'&&r.hold_s!==0))throw Error('Threshold or hold time does not fit this condition.');
          if((r.do==='theme'&&r.value>5)||(['notify','celebrate'].includes(r.do)&&r.value!==0))throw Error('Invalid action value.');
          if(typeof r.label!=='string'||r.label.length<1||r.label.length>32||/[^ -~]/.test(r.label))throw Error('Use 1–32 printable ASCII characters for the rule label.');
          rules.push({...r});
        }
        return {schema:1,rules};
      }
      // RULE_CONTRACT_END
      let rulesDirty=false,rulesBusy=false,rulesSeen='',rulesRetryAt=0,rulesBase=null;
      $('rulesDraft').value=JSON.stringify({schema:1,rules:[]},null,2);
      function readRulesDraft(){if(new TextEncoder().encode($('rulesDraft').value).length>8192)throw Error('Rule JSON exceeds 8 KiB.');return validateRules(JSON.parse($('rulesDraft').value));}
      function checkRules(){try{readRulesDraft();$('rulesApply').disabled=false;$('rulesResult').textContent=rulesDirty?'Valid local draft · not saved.':'Rules loaded.';return true;}catch(e){$('rulesApply').disabled=true;$('rulesResult').textContent=e.message;return false;}}
      $('rulesDraft').oninput=()=>{rulesDirty=true;checkRules();};
      $('rulesPreset').onclick=()=>{$('rulesDraft').value=JSON.stringify(rulesPreset,null,2);rulesDirty=true;checkRules();};
      $('rulesEmpty').onclick=()=>{$('rulesDraft').value=JSON.stringify({schema:1,rules:[]},null,2);rulesDirty=true;checkRules();};
      async function loadRules(force=false){if(rulesBusy)return;rulesBusy=true;const before=$('rulesDraft').value;try{const raw=await keyedJson('/api/v1/rules'),data=validateRules(raw);if(rulesDirty&&(!force||before!==$('rulesDraft').value)){$('rulesResult').textContent='Quy tắc đã đổi trên thiết bị. Bản nháp được giữ; bấm Đọc lại thiết bị để thay thế.';return;}rulesBase=Number.isInteger(raw.revision)&&typeof raw.state_boot_id==='string'?{expected_revision:raw.revision,expected_boot_id:raw.state_boot_id}:null;$('rulesDraft').value=JSON.stringify(data,null,2);rulesDirty=false;checkRules();}catch(e){rulesSeen='';rulesRetryAt=Date.now()+5000;$('rulesResult').textContent=e.message;}finally{rulesBusy=false;}}
      $('rulesReload').onclick=()=>loadRules(true);
      $('rulesApply').onclick=async()=>{if(rulesBusy||!checkRules())return;rulesBusy=true;$('rulesApply').disabled=true;const data=readRulesDraft(),before=$('rulesDraft').value;try{if(studioCas&&!rulesBase)throw Error('Đọc lại quy tắc từ thiết bị trước khi lưu.');const result=await keyedJson('/api/v1/rules',studioCas?{...data,...rulesBase}:data);rulesBase=Number.isInteger(result.revision)?{expected_revision:result.revision,expected_boot_id:result.state_boot_id}:null;if(before===$('rulesDraft').value)rulesDirty=false;$('rulesResult').textContent=result.mode==='demo'?'DEMO · quy tắc lưu cục bộ.':rulesDirty?'Đã lưu. Các thay đổi mới trong bản nháp chưa được gửi.':'Đã lưu quy tắc trên thiết bị.';}catch(e){$('rulesResult').textContent=e.message;}finally{rulesBusy=false;$('rulesApply').disabled=false;}};
      $('rulesImport').onclick=()=>$('rulesFile').click();
      $('rulesFile').onchange=async()=>{const file=$('rulesFile').files[0];if(!file)return;try{if(file.size>8192)throw Error('Rule file exceeds 8 KiB.');const data=validateRules(JSON.parse((await file.text()).replace(/^\uFEFF/,'')));$('rulesDraft').value=JSON.stringify(data,null,2);rulesDirty=true;checkRules();}catch(e){$('rulesResult').textContent=e.message;}finally{$('rulesFile').value='';}};
      $('rulesExport').onclick=()=>{if(!checkRules())return;const url=URL.createObjectURL(new Blob([JSON.stringify(readRulesDraft(),null,2)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='meodesk-rules.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
      function syncRules(data){if(!pair.value.trim()||!data.capabilities?.includes('automation_rules_v1')||rulesBusy||Date.now()<rulesRetryAt)return;const rev=String(data.state_boot_id)+':'+data.rules_revision;if(rev!==rulesSeen){rulesSeen=rev;loadRules();}}
      async function keyedJson(path,payload){const key=pair.value.trim();if(!key)throw Error('Nhập mã ghép nối trong mục Thiết bị.');const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),3000);try{const response=await fetch(path,{method:payload?'POST':'GET',headers:{'X-MEO-KEY':key,...(payload?{'Content-Type':'application/json'}:{})},body:payload?JSON.stringify(payload):undefined,cache:'no-store',signal:controller.signal});const result=await response.json();if(!response.ok||result.ok===false)throw Error(['scene_conflict','rules_conflict'].includes(result.error)?'Nội dung đã đổi ở giao diện khác hoặc thiết bị vừa khởi động lại. Bản nháp được giữ; đọc lại trước khi lưu.':result.error||'Thiết bị chưa nhận thay đổi.');return result;}finally{clearTimeout(timeout);}}
      $('configExport').addEventListener('click',async()=>{try{const data=await keyedJson('/api/v1/config');const link=document.createElement('a'),url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));link.href=url;link.download='meodesk-profile.json';link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);$('configResult').textContent='Đã xuất cấu hình, không kèm thông tin Wi-Fi hoặc mã ghép nối.';}catch(error){$('configResult').textContent=error.message;}});
      $('configRestore').addEventListener('click',async()=>{try{const file=$('configFile').files[0];if(!file||file.size>8192)throw Error('Choose a configuration JSON file up to 8 KiB.');const data=JSON.parse(await file.text());if(data.schema!==1||data.board!=='meodesk-esp32s3-n16r8'||!data.profile)throw Error('Incompatible configuration file.');await keyedJson('/api/v1/config',data);$('configResult').textContent='Đã khôi phục và lưu cấu hình trên thiết bị.';focusConfigDirty=false;streamSeenAt=0;poll();}catch(error){$('configResult').textContent=error.message;}});
      $('otaHistoryRefresh').addEventListener('click',async()=>{const root=$('otaHistoryList');root.replaceChildren();try{const data=await keyedJson('/api/v1/ota/history');for(const update of (Array.isArray(data.updates)?data.updates:[]).slice().reverse()){const row=document.createElement('li');row.textContent=[update.phase,update.version_before,update.version_after,update.target_slot].filter(Boolean).join(' · ');root.append(row);}if(!root.children.length){const row=document.createElement('li');row.textContent='Chưa có lịch sử cập nhật.';root.append(row);}}catch(error){const row=document.createElement('li');row.textContent=error.message;root.append(row);}});
      function chooseWorkspace(name){
        if(!workspaceGroups[name])name='overview';
        const labels={overview:['Tổng quan','Máy tính và góc bàn của bạn, kết nối cùng nhau.'],music:['Âm nhạc','Đang nghe gì hôm nay?'],focus:['Tập trung','Dành một khoảng thời gian cho việc quan trọng.'],studio:['Studio','Cảnh, giao diện và người bạn nhỏ trên bàn.'],device:['Thiết bị','Kết nối, cấu hình và cập nhật mimi.desk.']};$('workspaceTitle').textContent=labels[name][0];$('workspaceDescription').textContent=labels[name][1];
        document.querySelectorAll('[data-workspace]').forEach(button=>{const active=button.dataset.workspace===name;button.setAttribute('aria-selected',String(active));button.tabIndex=active?0:-1;});
        Object.keys(workspaceGroups).forEach(key=>$('view-'+key).hidden=key!==name);
        try{sessionStorage.setItem('meodesk-workspace',name);}catch(_){}
        window.scrollTo(0,0);
        const selected=document.querySelector('[data-workspace="'+name+'"]');if(selected&&innerWidth<=850)selected.scrollIntoView({block:'nearest',inline:'nearest'});
        requestAnimationFrame(()=>{drawHistory();drawFpsHistory();});
      }
      document.querySelectorAll('[data-workspace]').forEach(button=>{
        button.addEventListener('click',()=>chooseWorkspace(button.dataset.workspace));
        button.addEventListener('keydown',event=>{const tabs=Array.from(document.querySelectorAll('[data-workspace]')),index=tabs.indexOf(button);let next;if(event.key==='ArrowRight')next=(index+1)%tabs.length;if(event.key==='ArrowLeft')next=(index+tabs.length-1)%tabs.length;if(event.key==='Home')next=0;if(event.key==='End')next=tabs.length-1;if(next!==undefined){event.preventDefault();chooseWorkspace(tabs[next].dataset.workspace);tabs[next].focus();}});
      });
      let stateSocket=null,streamSeenAt=0,stateBoot='',stateRevision=-1,pollInFlight=false;
      // BEGIN MIMI UPDATE SCRIPT
      // Device-owned server OTA. Auth stays in the existing pairing field/session.
      const pull = {revision:0,dirty:false,pending:false,supported:false,loaded:false,state:{},demo:false};
      const pullMessages = {
        idle:'Chưa có lượt cập nhật đang chạy.',checking:'Đang kiểm tra bản mới…',up_to_date:'Đang dùng bản mới nhất trong kênh đã chọn.',
        release_available:'Có phiên bản mới sẵn sàng.',waiting_for_focus:'Chờ kết thúc phiên tập trung.',waiting_for_device:'Chờ thiết bị sẵn sàng.',
        reboot_notice:'Sắp bắt đầu cập nhật.',downloading:'Thiết bị đang tải firmware…',verified_rebooting:'Đã xác minh. Đang khởi động lại…',
        update_confirmed:'Đã xác nhận firmware mới hoạt động.',returned_previous_firmware:'Thiết bị đang dùng bản trước; đã tạm ngừng tự thử bản vừa cài.',
        policy_conflict:'Chính sách vừa thay đổi ở giao diện khác. Đọc lại trước khi lưu.',release_changed:'Bản mới đã thay đổi. Kiểm tra lại trước khi cập nhật.',
        server_not_configured:'Nhập địa chỉ manifest rồi lưu để kiểm tra.',update_busy:'Thiết bị đang xử lý lượt cập nhật khác.',
        invalid_policy:'Kiểm tra địa chỉ manifest, kênh và chu kỳ.',incompatible_manifest:'Manifest không phù hợp với thiết bị hoặc kênh.',
        incompatible_release:'Bản phát hành không phù hợp; kiểm tra phiên bản cầu nối và dung lượng.',release_mutated:'Nội dung release đã đổi. Cần mã release mới.',
        stale_manifest:'Server đang trả manifest cũ. Thiết bị sẽ không hạ phiên bản.',http_connect_failed:'Chưa kết nối được server. Kiểm tra Wi-Fi, địa chỉ và giờ thiết bị.',
        http_status_failed:'Server chưa cung cấp file cần tải.',content_size_mismatch:'Dung lượng khác manifest. Thiết bị đã giữ firmware hiện tại.',
        image_verification_failed:'Firmware không qua xác minh. Thiết bị đã giữ bản hiện tại.',download_interrupted:'Tải bị gián đoạn; sẽ thử lại theo lịch.'
      };
      const pullFields=['pullUrl','pullMode','pullChannel','pullInterval','pullDefer'];
      function pullControls(){
        const allowed=pull.supported&&!pull.pending;
        pullFields.forEach(id=>$(id).disabled=!allowed);
        $('pullReload').disabled=pull.pending;
        $('pullSave').disabled=!allowed||!pull.dirty;
        $('pullCheck').disabled=!allowed||pull.dirty;
        $('pullStart').disabled=!allowed||pull.dirty||!pull.state.release_id||!['available','error'].includes(pull.state.phase);
      }
      function pullRender(state){
        pull.state={...state};
        const phase=state.phase||'idle',active=state.manual_request||['downloading','rebooting'].includes(phase);
        const target=(active?state.active_version:state.available_version)||'chưa có bản mới';
        $('pullVersion').textContent=(state.current_version||'—')+' → '+target;
        if(active&&state.available_version&&state.available_version!==target)$('pullVersion').textContent+=' · Đang chờ thêm bản '+state.available_version;
        let message=pullMessages[state.code]||'Đang chờ trạng thái cập nhật từ thiết bị.';
        if(pull.demo)message='DEMO · '+message;
        if(phase==='countdown')message+=' Bắt đầu sau '+(state.countdown_s||0)+' giây.';
        if(state.quarantined)message+=' Đã ngừng thử tự động; có thể chủ động thử lại.';
        $('pullStatus').textContent=message;$('pullNotes').textContent=state.notes||'';
        $('pullProgress').hidden=!['checking','downloading','rebooting'].includes(phase);
        if(['checking','rebooting'].includes(phase))$('pullProgress').removeAttribute('value');
        else $('pullProgress').value=state.expected?Math.round(100*Math.min(state.written||0,state.expected)/state.expected):0;
        pullControls();
      }
      function pullReceive(data){
        if(data.demo||data.device==='MEO DESK / MOCK')pull.demo=true;
        if(data.update_state)pullRender(data.update_state);
        const policy=data.update_policy;
        if(!policy)return;
        const capable=(data.capabilities||[]).includes('ota_pull_v1')||pull.supported;
        if(!capable){$('pullNotice').textContent='Thiết bị cần firmware cầu nối cho OTA server. Dùng cập nhật bằng file bên dưới.';pull.supported=false;pullControls();return;}
        if(policy.revision!==pull.revision){
          if(pull.dirty)$('pullNotice').textContent='Chính sách đã đổi ở giao diện khác. Bản nháp được giữ; đọc lại trước khi lưu.';
          else if(!pull.pending&&pair.value.trim())pullRequest('load',{},false);
        }
      }
      function pullCommand(){return 'mimi-'+Date.now().toString(16)+'-'+Math.random().toString(16).slice(2);}
      async function pullRequest(operation,payload={},force=true){
        if(pull.pending)return;
        const key=pair.value.trim();if(!key){$('pullNotice').textContent='Nhập và lưu mã ghép nối để quản lý cập nhật.';return;}
        const paths={load:'/api/v1/update-policy',configure:'/api/v1/update-policy',check:'/api/v1/update/check',start:'/api/v1/update/start'};
        if(!paths[operation])return;
        if(['check','start'].includes(operation))payload={...payload,command_id:pullCommand()};
        pull.pending=true;pullControls();$('pullNotice').textContent='Đang trao đổi với thiết bị…';
        const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);
        try{
          const response=await fetch(paths[operation],{method:operation==='load'?'GET':'POST',cache:'no-store',signal:controller.signal,
            headers:{'Content-Type':'application/json','X-MEO-KEY':key},...(operation==='load'?{}:{body:JSON.stringify(payload)})});
          if(response.status===404){pull.supported=false;$('pullNotice').textContent='Thiết bị cần firmware cầu nối hỗ trợ OTA server. Dùng cập nhật bằng file bên dưới.';const details=document.querySelector('.manual-tools');if(details)details.open=true;return;}
          const data=await response.json();
          if(!response.ok)throw Error(pullMessages[data.error]||(response.status===401?'Kiểm tra lại mã ghép nối.':'Thiết bị chưa nhận thay đổi. Kiểm tra kết nối rồi thử lại.'));
          if(data.schema!==1||!data.policy||!data.update)throw Error('Phản hồi cập nhật chưa hợp lệ.');
          pull.supported=true;pull.loaded=true;pull.demo=pull.demo||data.demo===true;
          if(force||!pull.dirty){
            const p=data.policy;$('pullUrl').value=p.manifest_url||'';$('pullMode').value=p.mode;$('pullChannel').value=p.channel;
            $('pullInterval').value=p.interval_s;$('pullDefer').checked=p.defer_focus;pull.revision=p.revision;pull.dirty=false;
          }
          pullRender(data.update);$('pullNotice').textContent=pull.demo?'DEMO · chính sách và tiến độ mô phỏng.':operation==='configure'?'Đã lưu chính sách trên thiết bị.':'Chính sách lấy từ thiết bị.';
        }catch(error){$('pullNotice').textContent=error.name==='AbortError'?'Thiết bị chưa phản hồi. Đọc lại để kiểm tra kết quả trước khi thử tiếp.':error.message;}
        finally{clearTimeout(timer);pull.pending=false;pullControls();}
      }
      $('pullForm').addEventListener('submit',event=>{
        event.preventDefault();const seconds=Number($('pullInterval').value),url=$('pullUrl').value.trim();
        if(!Number.isInteger(seconds)||seconds<60||seconds>86400){$('pullNotice').textContent='Chu kỳ kiểm tra từ 60 đến 86.400 giây.';return;}
        pullRequest('configure',{expected_revision:pull.revision,manifest_url:url,mode:$('pullMode').value,channel:$('pullChannel').value,
          interval_s:seconds,defer_focus:$('pullDefer').checked});
      });
      pullFields.forEach(id=>$(id).addEventListener('input',()=>{pull.dirty=true;pullControls();}));
      $('pullReload').addEventListener('click',()=>pullRequest('load'));
      $('pullCheck').addEventListener('click',()=>pullRequest('check'));
      $('pullStart').addEventListener('click',()=>pullRequest('start',{release_id:pull.state.release_id,expected_revision:pull.revision}));
      pullControls();
      let otaExpected=null,otaResuming=false;
      try{const saved=JSON.parse(localStorage.getItem('mimi-ota-job')||'null');if(saved&&['app0','app1'].includes(saved.target_slot))otaExpected=saved;}catch(_){}
      function saveOtaExpectation(targetSlot,phase){
        otaExpected={schema:1,target_slot:targetSlot,device_id:otaDevice?.device_id||'',previous_boot_id:otaDevice?.state_boot_id||'',phase};
        try{localStorage.setItem('mimi-ota-job',JSON.stringify(otaExpected));}catch(_){}
      }
      function finishOtaExpectation(){otaExpected=null;try{localStorage.removeItem('mimi-ota-job');}catch(_){} }
      async function resumeWebOta(){
        if(!otaExpected||otaResuming||otaUploading)return;
        otaResuming=true;$('otaResult').textContent='Đang kiểm tra lượt cập nhật trước; không gửi lại firmware.';
        try{await verifyOtaBoot(otaExpected.target_slot,otaExpected);finishOtaExpectation();$('otaResult').textContent='Đã xác nhận firmware mới hoạt động.';}
        catch(error){$('otaResult').textContent='Chưa xác nhận được lượt trước. Kiểm tra nguồn, kết nối và địa chỉ thiết bị.';$('otaResult').title=error.message;}
        finally{otaResuming=false;}
      }
      const resumeButton=document.createElement('button');resumeButton.type='button';resumeButton.textContent='Kiểm tra lượt cập nhật trước';resumeButton.addEventListener('click',resumeWebOta);$('otaResult').after(resumeButton);
      if(otaExpected)setTimeout(resumeWebOta,1500);
      $('setupPair').addEventListener('click',async()=>{
        $('setupResult').textContent='Đang lấy mã. Máy tính cần kết nối Wi-Fi thiết lập của thiết bị.';
        const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);
        try{const response=await fetch('/api/v1/pairing',{signal:controller.signal,cache:'no-store'});const data=await response.json();
          if(!response.ok||typeof data.pairing_key!=='string'||!data.pairing_key)throw Error('setup not available');
          pair.value=data.pairing_key;$('setupResult').textContent='Đã lấy mã từ thiết bị. Bấm Lưu mã để hoàn tất.';
        }catch(_){$('setupResult').textContent='Chưa lấy được mã. Kết nối Wi-Fi thiết lập của thiết bị hoặc nhập mã đang có.';}
        finally{clearTimeout(timer);}
      });
      // END MIMI UPDATE SCRIPT
      function receiveState(data){
        if(data.device!=='MEO DESK'&&data.device!=='MEO DESK / MOCK')return;
        if(typeof pullReceive==='function')pullReceive(data);
        if(typeof data.state_boot_id==='string'&&Number.isInteger(data.state_revision)){
          if(data.state_boot_id===stateBoot&&data.state_revision<stateRevision)return;
          stateBoot=data.state_boot_id;stateRevision=data.state_revision;
        }
        update(data);if(data.media)renderMedia(data.media);
        if(Array.isArray(data.capabilities)&&data.capabilities.includes('state_websocket_v1'))connectState(data.state_ws_port||81);
      }
      function connectState(port){
        if(stateSocket||!window.WebSocket)return;
        const socket=new WebSocket('ws://'+location.hostname+':'+port+'/');stateSocket=socket;
        socket.onopen=()=>socket.send(JSON.stringify({type:'subscribe',key:pair.value.trim()}));
        socket.onmessage=event=>{try{const data=JSON.parse(event.data);if(data.type==='update'&&data.state_boot_id===stateBoot){if(typeof pullReceive==='function')pullReceive(data);return;}if(data.type!=='state'||!['MEO DESK','MEO DESK / MOCK'].includes(data.device))return;streamSeenAt=performance.now();$('syncMode').textContent='Đồng bộ trực tiếp';$('syncMode').dataset.live='true';receiveState(data);}catch(_){} };
        socket.onerror=()=>socket.close();
        socket.onclose=()=>{if(stateSocket!==socket)return;stateSocket=null;streamSeenAt=0;$('syncMode').textContent='Đang kết nối lại…';$('syncMode').dataset.live='false';setTimeout(()=>connectState(port),2000);};
      }
      const streamFresh=()=>streamSeenAt>0&&performance.now()-streamSeenAt<2500;
      const packPoses=['boot','clock','paused','sleep','storage','network','offline','music','bambu','audio','health','healthy','hotspot','power','rescue','shutdown'];
      let packPixels=null,packFrames=1,packTick=0,packSelection=0;
      const preview=document.createElement('div');preview.className='pack-preview';
      preview.innerHTML='<canvas id="packPreviewCanvas" width="144" height="144" aria-label="Selected local Cat Pack preview" role="img"></canvas><label>Local pack preview<select id="packPreviewPose" aria-label="Preview cat pose"></select><small id="packPreviewHint">Choose a .meocat file. Nothing is uploaded until Install.</small></label>';
      document.querySelector('.catpack-panel .ota-upload').prepend(preview);
      packPoses.forEach((name,index)=>{const option=document.createElement('option');option.value=index;option.textContent=name;$('packPreviewPose').append(option);});
      function packFramePixels(bytes,pose,cel,frames){
        const offset=(pose*frames+cel)*144*144*2,view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),rgba=new Uint8ClampedArray(144*144*4);
        for(let i=0;i<144*144;i++){const pixel=view.getUint16(offset+i*2,true),r=(pixel>>>11)&31,g=(pixel>>>5)&63,b=pixel&31;rgba[i*4]=(r<<3)|(r>>>2);rgba[i*4+1]=(g<<2)|(g>>>4);rgba[i*4+2]=(b<<3)|(b>>>2);rgba[i*4+3]=255;}return rgba;
      }
      function paintPackPreview(){if(!packPixels)return;const cel=packFrames===3?[0,1,2,1][packTick++%4]:0;$('packPreviewCanvas').getContext('2d').putImageData(new ImageData(packFramePixels(packPixels,Number($('packPreviewPose').value),cel,packFrames),144,144),0,0);}
      $('packPreviewPose').addEventListener('change',()=>{packTick=0;paintPackPreview();});
      $('catPackFile').addEventListener('change',async()=>{const selected=++packSelection,file=$('catPackFile').files[0];packPixels=null;packTick=0;$('packPreviewCanvas').getContext('2d').clearRect(0,0,144,144);if(!file)return;if(![663552,1990656].includes(file.size)){$('packPreviewHint').textContent='Invalid pack size. Choose a static v1 or motion v2 pack.';return;}const bytes=new Uint8Array(await file.arrayBuffer());if(selected!==packSelection)return;packPixels=bytes;packFrames=file.size/663552;$('packPreviewHint').textContent=packFrames+' frame(s) per pose · local preview only';paintPackPreview();});
      setInterval(()=>{if(!$('view-studio').hidden&&!document.hidden)paintPackPreview();},170);
      // SHA-256 fallback for plain HTTP LAN pages, where Web Crypto SubtleCrypto
      // is intentionally unavailable in most browsers (insecure context).
      function sha256Fallback(bytes){
        const K=[0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
        const H=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];
        const n=bytes.length,padded=Math.ceil((n+9)/64)*64;
        const block=new Uint8Array(padded);block.set(bytes);block[n]=128;
        const view=new DataView(block.buffer);view.setUint32(padded-4,(n*8)>>>0,false);
        view.setUint32(padded-8,Math.floor(n/536870912)>>>0,false);
        const rotr=(v,b)=>(v>>>b)|(v<<(32-b));const w=new Uint32Array(64);
        for(let at=0;at<padded;at+=64){
          for(let i=0;i<16;i++)w[i]=view.getUint32(at+i*4,false);
          for(let i=16;i<64;i++){
            const a=w[i-15],b=w[i-2];
            w[i]=(w[i-16]+(rotr(a,7)^rotr(a,18)^(a>>>3))+w[i-7]+(rotr(b,17)^rotr(b,19)^(b>>>10)))>>>0;
          }
          let [a,b,c,d,e,f,g,h]=H;
          for(let i=0;i<64;i++){
            const t1=(h+(rotr(e,6)^rotr(e,11)^rotr(e,25))+((e&f)^(~e&g))+K[i]+w[i])>>>0;
            const t2=((rotr(a,2)^rotr(a,13)^rotr(a,22))+((a&b)^(a&c)^(b&c)))>>>0;
            h=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0;
          }
          [a,b,c,d,e,f,g,h].forEach((v,i)=>H[i]=(H[i]+v)>>>0);
        }
        return H.map(v=>v.toString(16).padStart(8,'0')).join('');
      }
      async function firmwareSha256(buffer){
        if(typeof crypto!=='undefined'&&crypto.subtle){
          const digest=await crypto.subtle.digest('SHA-256',buffer);
          return Array.from(new Uint8Array(digest),b=>b.toString(16).padStart(2,'0')).join('');
        }
        return sha256Fallback(new Uint8Array(buffer));
      }
      let otaDevice=null,otaUploading=false,otaResultPinnedUntil=0;
      function updateOtaButton(){
        $('otaStart').disabled=otaUploading||!otaDevice||!otaDevice.ready||!$('otaFile').files.length;
      }
      async function pollOtaStatus(){
        if(otaUploading)return;
        try{
          const res=await fetch('/api/v1/ota/status',{cache:'no-store'});if(!res.ok)throw Error('HTTP '+res.status);
          otaDevice=await res.json();
          $('otaVersion').textContent=otaDevice.version||'—';
          $('otaSlot').textContent=(otaDevice.inactive_slot||'N/A')+' / '+(Math.round((otaDevice.inactive_slot_bytes||0)/1048576*10)/10)+' MiB';
          $('otaReady').textContent=otaDevice.ready?'A/B READY':'USB BOOTSTRAP NEEDED';
          $('otaRollback').textContent=otaDevice.bootloader_rollback_configured?'SUPPORTED':'NOT VERIFIED';
          $('otaBootHealth').textContent=(otaDevice.boot_health||'unknown').toUpperCase();
          $('otaUptime').textContent=Math.floor((otaDevice.uptime_ms||0)/1000)+' sec';
          if(Date.now()>otaResultPinnedUntil){
            if(!otaDevice.ready)$('otaResult').textContent='OTA not available: verify partition table and flash size.';
            else if(otaDevice.phase==='error')$('otaResult').textContent=otaDevice.message||'Update failed';
            else if(otaDevice.phase==='idle')$('otaResult').textContent='Sẵn sàng kiểm tra file firmware của thiết bị.';
          }
          updateOtaButton();
        }catch(e){otaDevice=null;$('otaReady').textContent='OFFLINE';updateOtaButton();}
      }
      async function verifyOtaBoot(targetSlot,expected={}){
        let sawNewSlot=false;
        const deadline=Date.now()+85000;
        while(Date.now()<deadline){
          await new Promise(resolve=>setTimeout(resolve,2000));
          const ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),3500);
          let data=null;
          try{
            const response=await fetch('/api/v1/ota/status',{cache:'no-store',signal:ctrl.signal});
            if(response.ok)data=await response.json();
          }catch(e){}finally{clearTimeout(timer);}
          if(!data){$('otaResult').textContent='Waiting for ESP32 to reconnect…';continue;}
          if(data.board!=='meodesk-esp32s3-n16r8')throw Error('Connected device board changed');
          if(expected.device_id&&data.device_id!==expected.device_id){$('otaResult').textContent='Đang chờ đúng thiết bị đã cập nhật…';continue;}
          if(expected.previous_boot_id&&(!data.state_boot_id||data.state_boot_id===expected.previous_boot_id)){$('otaResult').textContent='Đang chờ lượt khởi động mới…';continue;}
          if(data.running_slot===targetSlot){
            sawNewSlot=true;
            if(data.boot_health==='failed')throw Error('New firmware failed device startup health check');
            if(data.boot_health==='passed'&&data.ready){if(expected.version&&data.version!==expected.version)throw Error('Firmware version mismatch');otaDevice=data;return data;}
            $('otaResult').textContent='New slot active. Waiting for startup self-check (~25 s)…';
          }else if(sawNewSlot){throw Error('Device returned to previous firmware slot');}
          else{$('otaResult').textContent='Waiting for ESP32 to boot into '+targetSlot+'…';}
        }
        throw Error('Boot not confirmed within 85 s. Check USB Serial before retrying.');
      }
      async function uploadOta(){
        const file=$('otaFile').files[0],key=pair.value.trim();
        if(!file||!otaDevice||!otaDevice.ready)return;
        if(!key){toast('Nhập mã ghép nối của thiết bị trước.');pair.focus();return;}
        if(!file.name.toLowerCase().endsWith('.bin')||file.size<4096||file.size>otaDevice.inactive_slot_bytes){toast('Choose a .bin within the inactive slot size');return;}
        if(otaDevice.board!=='meodesk-esp32s3-n16r8'){toast('Board type mismatch: OTA blocked');return;}
        if(!confirm('Update MèoDesk with '+file.name+' ('+(file.size/1048576).toFixed(2)+' MiB)? Keep power connected.'))return;
        const targetSlot=otaDevice.inactive_slot;
        if(targetSlot!=='app0'&&targetSlot!=='app1'){toast('Invalid inactive OTA slot');return;}
        saveOtaExpectation(targetSlot,'uploading');
        otaUploading=true;updateOtaButton();$('otaProgress').style.width='0%';
        let accepted=false;
        try{
          $('otaResult').textContent='Đang kiểm tra file firmware…';
          const data=await file.arrayBuffer();
          const header=new Uint8Array(data);
          if(header[0]!==0xE9||header[12]!==9||header[13]!==0)throw Error('Firmware is not compiled for ESP32-S3');
          const hash=await firmwareSha256(data);
          $('otaResult').textContent='Uploading into inactive flash slot…';
          const form=new FormData();form.append('firmware',file,file.name);
          const result=await new Promise((resolve,reject)=>{
            const xhr=new XMLHttpRequest();xhr.open('POST','/api/v1/ota/upload');xhr.timeout=240000;
            xhr.setRequestHeader('X-MEO-KEY',key);
            xhr.setRequestHeader('X-MEO-OTA-BOARD',otaDevice.board);
            xhr.setRequestHeader('X-MEO-OTA-SIZE',String(file.size));
            xhr.setRequestHeader('X-MEO-OTA-SHA256',hash);
            xhr.upload.onprogress=e=>{if(e.lengthComputable)$('otaProgress').style.width=Math.min(99,Math.round(e.loaded/e.total*100))+'%';};
            xhr.onerror=()=>reject(Error('Network interrupted; active firmware should remain bootable'));
            xhr.ontimeout=()=>reject(Error('Firmware upload timed out'));
            xhr.onload=()=>{let response;try{response=JSON.parse(xhr.responseText);}catch(e){return reject(Error('Invalid device response'));}if(xhr.status!==200||!response.ok)return reject(Error(response.error||'Firmware rejected'));resolve(response);};
            xhr.send(form);
          });
          $('otaProgress').style.width='100%';
          if(!result.verified)throw Error('Upload was not verified');
          accepted=true;
          saveOtaExpectation(targetSlot,'uploaded');
          $('otaResult').textContent='Đã xác minh firmware. Đang chờ thiết bị khởi động lại…';
          await verifyOtaBoot(targetSlot,otaExpected||{});
          finishOtaExpectation();$('otaResult').textContent='Đã xác nhận firmware mới hoạt động.';
          toast('Thiết bị đã cập nhật xong');
        }catch(e){$('otaResult').textContent=accepted?'Chưa xác nhận được khởi động. Kiểm tra lại lượt cập nhật trước khi thử tiếp.':'Chưa cập nhật được firmware. Kiểm tra kết nối, file và mã ghép nối.';$('otaResult').title=e.message;toast($('otaResult').textContent);}
        finally{otaResultPinnedUntil=Date.now()+30000;otaUploading=false;pollOtaStatus();updateOtaButton();}
      }
      let catPackDevice=null,catPackUploading=false,catPackResultPinnedUntil=0;
      function catPackAllowed(file){
        if(!file||!catPackDevice)return false;
        const sizes=catPackDevice.accepted_pack_bytes||[catPackDevice.pack_bytes];
        return file.name.toLowerCase().endsWith('.meocat')&&sizes.includes(file.size);
      }
      function updateCatPackButton(){
        $('catPackStart').disabled=catPackUploading||!catPackDevice||!catPackDevice.ready||!catPackAllowed($('catPackFile').files[0]);
      }
      async function pollCatPackStatus(){
        if(catPackUploading)return;
        try{
          const res=await fetch('/api/v1/catpack/status',{cache:'no-store'});
          if(!res.ok)throw Error('HTTP '+res.status);
          catPackDevice=await res.json();
          $('catPackSource').textContent=catPackDevice.source==='custom'?'CUSTOM / PSRAM':'ORIGINAL / FLASH';
          $('catPackGen').textContent=String(catPackDevice.generation||0);
          $('catPackReady').textContent=catPackDevice.ready?'READY':'UNAVAILABLE';
          $('catPackMotion').textContent=(catPackDevice.frames_per_pose||1)+' FRAME'+(catPackDevice.frames_per_pose===3?'S / POSE':' / POSE');
          if(Date.now()>catPackResultPinnedUntil)
            $('catPackResult').textContent=catPackDevice.ready?'Choose a .meocat collection to update all cat scenes.':'Reserved assets Flash or PSRAM unavailable.';
        }catch(e){catPackDevice=null;$('catPackReady').textContent='OFFLINE';}
        updateCatPackButton();
      }
      async function verifyCatPackBoot(expectedHash){
        const deadline=Date.now()+55000;
        while(Date.now()<deadline){
          await new Promise(resolve=>setTimeout(resolve,1700));
          const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),3500);
          let data=null;
          try{
            const res=await fetch('/api/v1/catpack/status',{cache:'no-store',signal:controller.signal});
            if(res.ok)data=await res.json();
          }catch(e){}finally{clearTimeout(timeout);}
          if(data&&data.source==='custom'&&data.sha256===expectedHash){catPackDevice=data;return data;}
          $('catPackResult').textContent=data?'Waiting for new cat sprites to load into PSRAM…':'Waiting for the cat desk to restart…';
        }
        throw Error('New pack not confirmed after restart. Built-in or previous cats remain available.');
      }
      async function uploadCatPack(){
        const file=$('catPackFile').files[0],key=pair.value.trim();
        if(!file||!catPackDevice||!catPackDevice.ready)return;
        if(!key){toast('Nhập mã ghép nối của thiết bị.');return;}
        if(!catPackAllowed(file)){toast('Use a supported 648 / 1,944 KiB .meocat pack (check PSRAM)');return;}
        if(!confirm('Install '+file.name+' ('+(file.size===1990656?'48 motion frames':'16 static frames')+')? Your desk restarts once.'))return;
        catPackUploading=true;updateCatPackButton();$('catPackProgress').style.width='0%';
        let accepted=false;
        try{
          $('catPackResult').textContent='Verifying SHA-256 of the entire sprite atlas…';
          const hash=await firmwareSha256(await file.arrayBuffer());
          const form=new FormData();form.append('catpack',file,file.name);
          $('catPackResult').textContent='Streaming new cat frames to the inactive bank…';
          const result=await new Promise((resolve,reject)=>{
            const xhr=new XMLHttpRequest();xhr.open('POST','/api/v1/catpack/upload');xhr.timeout=180000;
            xhr.setRequestHeader('X-MEO-KEY',key);
            xhr.setRequestHeader('X-MEO-CAT-SIZE',String(file.size));
            xhr.setRequestHeader('X-MEO-CAT-SHA256',hash);
            xhr.upload.onprogress=e=>{if(e.lengthComputable)$('catPackProgress').style.width=Math.min(99,Math.round(e.loaded/e.total*100))+'%';};
            xhr.onerror=()=>reject(Error('Cat pack upload interrupted'));
            xhr.ontimeout=()=>reject(Error('Cat pack upload timed out'));
            xhr.onload=()=>{let data;try{data=JSON.parse(xhr.responseText);}catch(e){return reject(Error('Invalid response'));}if(xhr.status!==200||!data.verified)return reject(Error(data.error||'Pack rejected'));resolve(data);};
            xhr.send(form);
          });
          accepted=result.verified;
          $('catPackProgress').style.width='100%';
          $('catPackResult').textContent='Verified. Rebooting to load new sprites into PSRAM…';
          await verifyCatPackBoot(hash);
          $('catPackResult').textContent='✓ MOTION PACK READY: '+(catPackDevice.frames_per_pose||1)+' frames per pose, loaded from PSRAM.';
          toast('Đã cài bộ chuyển động mới.');
        }catch(e){$('catPackResult').textContent=(accepted?'NOT CONFIRMED: ':'UPLOAD FAILED: ')+e.message;toast(e.message);}
        finally{catPackResultPinnedUntil=Date.now()+30000;catPackUploading=false;pollCatPackStatus();updateCatPackButton();}
      }
      let lastEmotion='',toastTimer=null,settingsDirty=false;
      const pair=$('pairKey');
      try{pair.value=sessionStorage.getItem('meodesk-key')||'';}catch(e){}
      function toast(msg){let node=$('toast');node.textContent=msg;node.classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>node.classList.remove('show'),2900);}
      function percent(n){return Number.isFinite(n)?Math.round(n)+'%':'—';}
      function bar(id,n){$(id).style.width=(Number.isFinite(n)?Math.max(0,Math.min(100,n)):0)+'%';}
      const mood={rescue:['Đang chờ máy tính','Chưa nhận dữ liệu mới. Kiểm tra app và kết nối Wi-Fi.'],sleep:['Một chút nghỉ ngơi','CPU và GPU đang ít việc. Mèo có thể chợp mắt một lát.'],healthy:['Mọi thứ đang ổn','Máy tính chưa chạm các ngưỡng cảnh báo đã đặt.'],stressed:['Máy tính đang bận','CPU hoặc GPU vượt ngưỡng đã chọn. Kiểm tra tác vụ đang chạy.'],hot:['Máy hơi nóng','Cảm biến nhiệt độ đã chạm ngưỡng cảnh báo. Kiểm tra tản nhiệt.'],low_battery:['Sắp hết pin','Laptop đang dùng pin và cần được cắm sạc.'],paused:['Đang tập trung','Focus hoặc chế độ yên tĩnh đang bật. Cảnh báo quan trọng vẫn được ưu tiên.'],power:['Đã cắm nguồn','Máy tính đang dùng nguồn điện ngoài.'],hotspot:['Wi-Fi thiết lập','Thiết bị đang dùng điểm truy cập Wi-Fi dự phòng.'],music:['Đang nghe nhạc','Trình phát trên Windows đang phát nhạc.'],demo:['DEMO / Mô phỏng','Đây là dữ liệu mẫu để xem giao diện. Mở app thật để đọc dữ liệu máy tính.']};
      const previews=['paused','low_battery','rescue','power','sleep','hot','healthy','hotspot'];
      const personaArt={"looking for PC":"rescue",cozy:"healthy",sleepy:"sleep",dancing:"music","deep focus":"paused",busy:"stressed","too warm":"hot","need charge":"low_battery",celebrating:"healthy"};
      const scenes=[[12,'Home'],[0,'Máy tính'],[1,'Đồng hồ'],[2,'Focus'],[3,'Wi-Fi'],[4,'Thiết bị'],[5,'Âm nhạc'],[6,'Máy in'],[7,'Âm thanh'],[8,'Moto'],[9,'Ngủ'],[10,'Sức khoẻ'],[11,'Xu hướng'],[13,'Mạng PC'],[14,'Widgets']];
      const defaultScenes=[12,0,5,2];
      scenes.forEach(([id,name])=>{for(let i=0;i<4;i++){const option=document.createElement('option');option.value=String(id);option.textContent=name;$('sceneSlot'+i).append(option);}});
      defaultScenes.forEach((id,i)=>{$('sceneSlot'+i).value=String(id);});
      let coverBlobUrl='',coverShownId='',coverPendingId='';
      const settingIds=['cpuLimit','gpuLimit','tempLimit','batteryLimit'];
      const chartState={history:null,range:300,hover:-1};
      const chartColors={cpu:'#66834f',gpu:'#af835a',ram:'#688669'};
      function rangeSeries(name){const data=chartState.history&&chartState.history[name];return Array.isArray(data)?data.slice(-chartState.range):[];}
      function drawHistory(){
        const canvas=$('historyCanvas'),rect=canvas.getBoundingClientRect(),w=Math.max(1,rect.width),h=Math.max(1,rect.height),ratio=Math.min(2,window.devicePixelRatio||1);
        const pw=Math.round(w*ratio),ph=Math.round(h*ratio);
        if(canvas.width!==pw||canvas.height!==ph){canvas.width=pw;canvas.height=ph;}
        const ctx=canvas.getContext('2d');if(!ctx)return;ctx.setTransform(ratio,0,0,ratio,0,0);ctx.clearRect(0,0,w,h);
        const l=38,r=15,t=15,b=30,plotW=Math.max(1,w-l-r),plotH=Math.max(1,h-t-b);
        ctx.font='10px Consolas,monospace';ctx.textAlign='right';ctx.lineWidth=1;
        for(const pct of [0,25,50,75,100]){const y=t+plotH*(1-pct/100);ctx.strokeStyle='#e7e4df';ctx.beginPath();ctx.moveTo(l,y);ctx.lineTo(w-r,y);ctx.stroke();ctx.fillStyle='#9d9aa3';ctx.fillText(pct+'%',l-7,y+3);}
        const cpu=rangeSeries('cpu'),gpu=rangeSeries('gpu'),ram=rangeSeries('ram');const n=cpu.length;
        ctx.textAlign='left';ctx.fillStyle='#99959f';ctx.fillText('-'+Math.min(chartState.range,Math.max(0,n-1))+'s',l,h-8);ctx.textAlign='right';ctx.fillText('NOW',w-r,h-8);
        if(n<2){ctx.fillStyle='#a29da8';ctx.textAlign='center';ctx.font='12px Segoe UI,sans-serif';ctx.fillText('Waiting for PC history…',l+plotW/2,t+plotH/2);return;}
        const xFor=i=>l+i*plotW/(n-1),yFor=v=>t+(1-v/100)*plotH;
        const series=[['ram',ram],['gpu',gpu],['cpu',cpu]];
        for(const [name,values] of series){ctx.beginPath();ctx.lineWidth=name==='cpu'?2.4:2;ctx.strokeStyle=chartColors[name];ctx.lineJoin='round';ctx.lineCap='round';let active=false;
          for(let i=0;i<n;i++){const v=values[i];if(typeof v!=='number'||!Number.isFinite(v)){active=false;continue;}const x=xFor(i),y=yFor(v);if(!active)ctx.moveTo(x,y);else ctx.lineTo(x,y);active=true;}ctx.stroke();}
        if(chartState.hover>=0&&chartState.hover<n){const i=chartState.hover,x=xFor(i);ctx.strokeStyle='#9e9aa8';ctx.setLineDash([3,4]);ctx.beginPath();ctx.moveTo(x,t);ctx.lineTo(x,t+plotH);ctx.stroke();ctx.setLineDash([]);
          for(const [name,values] of series){if(typeof values[i]==='number'){ctx.fillStyle=chartColors[name];ctx.beginPath();ctx.arc(x,yFor(values[i]),4,0,Math.PI*2);ctx.fill();}}}
      }
      function renderHistory(data){if(!data||!Array.isArray(data.cpu)||!Array.isArray(data.gpu)||!Array.isArray(data.ram))return;chartState.history=data;const all=rangeSeries('cpu');
        for(const name of ['cpu','gpu','ram']){const available=rangeSeries(name).filter(n=>typeof n==='number'&&Number.isFinite(n));$(name+'Peak').textContent=available.length?Math.max(...available)+'%':'N/A';}
        $('historyMeta').textContent=(data.mode==='demo'?'Mô phỏng / ':data.mode==='offline'?'OFFLINE / ':'Dữ liệu thực / ')+all.length+' mẫu · '+(chartState.range/60)+' phút gần nhất';drawHistory();}
      function moveHistoryHover(event){const rect=$('historyCanvas').getBoundingClientRect(),n=rangeSeries('cpu').length;if(n<2)return;const left=38,width=rect.width-left-15,i=Math.max(0,Math.min(n-1,Math.round((event.clientX-rect.left-left)*((n-1)/Math.max(1,width)))));chartState.hover=i;const values=['cpu','gpu','ram'].map(name=>{const v=rangeSeries(name)[i];return name.toUpperCase()+' '+(typeof v==='number'?v+'%':'N/A');});const tip=$('chartTip');tip.textContent=(n-1-i)+' giây trước · '+values.join(' / ');tip.style.left=Math.max(80,Math.min(rect.width-80,event.clientX-rect.left))+'px';tip.style.top='53px';tip.classList.add('visible');drawHistory();}
      async function pollHistory(){try{const res=await fetch('/api/history',{cache:'no-store'});if(!res.ok)throw new Error('Chưa tải được lịch sử');renderHistory(await res.json());}catch(e){$('historyMeta').textContent='Chưa tải được lịch sử hoạt động';}}
      function renderSettings(settings){if(!settings||settingsDirty)return;for(const id of settingIds){const key=id.replace(/[A-Z]/g,m=>'_'+m.toLowerCase());if(Number.isFinite(settings[key]))$(id).value=settings[key];} $('quietMode').checked=!!settings.quiet;showSettingValues();}
      function showSettingValues(){for(const id of settingIds){$(id+'Out').textContent=$(id).value+(id==='tempLimit'?'°C':'%');}}
      function renderEvents(entries){const root=$('healthEvents');root.replaceChildren();if(!Array.isArray(entries)||entries.length===0){const li=document.createElement('li');li.textContent='Chưa có sự kiện mới.';root.append(li);return;}for(const event of entries.slice(0,5)){const li=document.createElement('li'),span=document.createElement('span'),ago=document.createElement('time'),level=document.createElement('b');const pri=Number(event.priority)||0;li.className='priority-'+pri;level.className='event-label';level.textContent=['INFO','NOTE','WARN','ALERT'][Math.min(3,Math.max(0,pri))];span.textContent=event.message||'Trạng thái thay đổi';ago.textContent=(event.age_s||0)+'s ago';li.append(level,span,ago);root.append(li);}}
      function displayMetric(value,suffix){return Number.isFinite(value)?Number(value).toFixed(1)+suffix:'N/A';}
      let focusAnchor=null;
      function updateFocus(data){focusAnchor={...data,at:performance.now()};paintFocus(data,0);}
      function paintFocus(data,elapsed){const seconds=Math.max(0,(Number(data.focus_remaining_s)||0)-(data.focus_running?Math.floor(elapsed/1000):0)),work=data.focus_phase!=='break',duration=Number(data.focus_duration_s)||(work?1500:300);
        $('focusTitle').textContent=String(Math.floor(seconds/60)).padStart(2,'0')+':'+String(seconds%60).padStart(2,'0');
        $('focusPhase').textContent=(work?'FOCUS':'BREAK')+' / '+(data.focus_running?'IN PROGRESS':'PAUSED / READY');
        $('focusSessions').textContent=(Number(data.focus_sessions)||0)+' SESSIONS';
        $('focusBar').style.width=(Math.round(100*(duration-Math.min(duration,seconds))/duration))+'%';
        $('focusHero').textContent=data.focus_running?'Tạm dừng':'Bắt đầu / tiếp tục';}
      function update(data){
        $('noticeSend').disabled=!data.capabilities?.includes('notifications_v1');
        if(!$('noticeSend').disabled&&$('noticeResult').textContent==='Requires compatible firmware.')$('noticeResult').textContent='Ready · hidden content is removed before sending.';
        syncScene(data);syncRules(data);
        $('firmwareBuild').textContent=data.version||'unknown';
        const labSupported=Array.isArray(data.capabilities)&&data.capabilities.includes('performance_lab_v1');
        $('labToggle').disabled=!labSupported;$('labToggle').dataset.value=data.lab_enabled?'0':'1';$('labToggle').textContent=data.lab_enabled?'Dừng đo':'Bắt đầu đo';
        $('labStatus').textContent=!labSupported?'Install a Performance Lab build to measure this device.':!data.lab_enabled?'Tracing is off. Enable for a comparison run; settings are not saved to Flash.':data.lab_valid?'Device-reported measurement window. Compare matched theme / style / pack sessions.':'Collecting the first complete window (at least 5 seconds). DEMO previews have no hardware measurements.';
        const labMetric=(id,key,unit)=>{$(id).textContent=data.lab_valid&&Number.isFinite(data[key])?data[key].toFixed(2)+unit:'N/A';};
        labMetric('labCpu','lab_cpu_frame_ms',' ms');labMetric('labSubmit','lab_dma_submit_ms',' ms');labMetric('labWait','lab_dma_wait_ms',' ms');labMetric('labWall','lab_cat_wall_ms',' ms');labMetric('labCard','lab_card_wall_ms',' ms');labMetric('labBridge','lab_bridge_max_ms',' ms');
        $('labBytes').textContent=data.lab_valid&&Number.isFinite(data.lab_payload_bytes_s)?(data.lab_payload_bytes_s/1024).toFixed(1)+' KiB/s':'N/A';$('labWindow').textContent=data.lab_valid?(data.lab_window_ms/1000).toFixed(2)+' s':'N/A';
        const ready=!!data.pc_connected;
        if(!focusConfigDirty){$('focusWorkMin').value=data.focus_work_min||25;$('focusBreakMin').value=data.focus_break_min||5;$('focusGoal').value=data.focus_goal||4;}
        const persona=data.cat_persona||'';
        const emo=data.preview_active?(data.emotion||'rescue'):(personaArt[persona]||data.emotion||'rescue');
        $('liveLed').classList.toggle('live',ready);
        $('liveText').textContent=ready?(data.demo?'Dữ liệu mô phỏng':'PC STREAM LIVE'):'Chờ máy tính';
        $('dataTag').textContent=ready?(data.demo?'Mô phỏng':data.preview_active?'MASCOT PREVIEW':'REAL SENSOR DATA'):'Chưa có dữ liệu';
        $('pcName').textContent=ready?(data.pc_name||'Windows PC'):'Chưa kết nối';
        $('cpuValue').textContent=ready?percent(data.cpu_percent):'—';
        $('gpuValue').textContent=ready&&data.gpu_percent!=null?percent(data.gpu_percent):'N/A';
        $('ramValue').textContent=ready?percent(data.ram_percent):'—';
        $('cpuDescription').textContent=ready?(data.demo?'Đang xem dữ liệu mô phỏng.':'Measurements update every second from your Windows machine.'):'Mở app mimi.desk để đồng bộ thông số máy tính.';
        const cpu=ready&&Number.isFinite(data.cpu_percent)?Math.min(100,Math.max(0,data.cpu_percent)):0;
        $('cpuRing').style.background=`conic-gradient(#f08271 0% ${cpu}%, #4a4b58 ${cpu}% 100%)`;
        bar('gpuBar',ready?data.gpu_percent:null);bar('ramBar',ready?data.ram_percent:null);
        $('gpuDetail').textContent=ready?(data.gpu_temp_c!=null?'GPU '+Math.round(data.gpu_temp_c)+'°C':(data.gpu_name||'GPU unavailable')):'Chưa có dữ liệu GPU';
        $('ramDetail').textContent=ready&&data.ram_total_gb!=null?`${Number(data.ram_used_gb).toFixed(1)} / ${Number(data.ram_total_gb).toFixed(1)} GB`:'— / — GB';
        $('netDown').textContent=ready?displayMetric(data.net_down_kbps,' KB/s'):'N/A';
        $('netUp').textContent=ready?displayMetric(data.net_up_kbps,' KB/s'):'N/A';
        $('vramUsed').textContent=ready?displayMetric(data.vram_used_gb,' GB'):'N/A';
        $('vramTotal').textContent=ready?displayMetric(data.vram_total_gb,' GB'):'N/A';
        updateFocus(data);
        if(Array.isArray(data.scene_slots)&&data.scene_slots.length===4){data.scene_slots.forEach((page,i)=>{const el=$('sceneSlot'+i);if(document.activeElement!==el)el.value=String(page);});}
        $('petEnergy').textContent=Number.isFinite(data.cat_energy)?data.cat_energy+'%':'—';
        document.querySelectorAll('[data-action="theme"]').forEach(b=>{const active=Number(b.dataset.value)===data.theme;b.classList.toggle('active',active);b.setAttribute('aria-pressed',active?'true':'false');});
        document.querySelectorAll('[data-action="fps"]').forEach(b=>{const active=Number(b.dataset.value)===data.fps_profile;b.classList.toggle('active',active);b.setAttribute('aria-pressed',active?'true':'false');});
        document.querySelectorAll('[data-action="motion_style"]').forEach(b=>{const active=Number(b.dataset.value)===data.motion_style;b.classList.toggle('active',active);b.setAttribute('aria-pressed',active?'true':'false');});
        $('motionState').textContent=(data.motion_state||'still').toUpperCase()+(data.motion_blink?' / BLINK':'');
        $('motionCel').textContent='CEL '+(Number.isFinite(data.motion_cel)?data.motion_cel:0)+' / '+(data.cat_motion_frames||1);
        $('motionGuard').textContent=data.motion_budget_limited?'FPS GUARD / LITE':'FPS GUARD / NORMAL';
        $('motionSignal').textContent=ready&&!data.demo?'REAL PC / '+(data.motion_pc_intensity||0)+'%':(data.demo?'DEMO / NOT REAL':'PC / OFFLINE');
        $('deviceIp').textContent=data.ip||'—';$('wifiName').textContent=data.wifi_ssid||'—';$('heap').textContent=(data.esp_heap_kb??'—')+' KB';
        if(!document.activeElement||document.activeElement!==$('newSsid'))$('newSsid').placeholder=data.wifi_ssid||'Tên Wi-Fi 2.4 GHz';
        $('wifiScanResult').textContent=data.wifi_scan_active?'SCANNING 2.4 GHz...':(Number.isFinite(data.wifi_network_count)&&data.wifi_network_count>=0?'Đã thấy '+data.wifi_network_count+' mạng · mạnh nhất: '+(data.wifi_strongest_ssid||'N/A'):'Fallback AP: '+(data.ap_ip||'192.168.4.1'));
        $('catFps').textContent=Number.isFinite(data.cat_fps)?data.cat_fps.toFixed(1)+' / '+(data.fps_target||20):'—';
        $('psramFree').textContent=Number.isFinite(data.psram_free_kb)?Math.round(data.psram_free_kb)+' KB':'N/A';
        $('renderMax').textContent=Number.isFinite(data.cat_render_max_ms)?data.cat_render_max_ms.toFixed(1)+' ms':'—';
        $('frameSkipped').textContent=Number.isFinite(data.cat_frames_skipped)?String(data.cat_frames_skipped):'—';
        $('dmaState').textContent=data.dma?'ACTIVE':'FALLBACK';
        $('deviceSub').textContent=data.wifi_connected?'Đã kết nối Wi-Fi':'Fallback AP active';
        $('cpuTemp').textContent=ready&&data.cpu_temp_c!=null?Math.round(data.cpu_temp_c)+'°C':'N/A';
        $('gpuTemp').textContent=ready&&data.gpu_temp_c!=null?Math.round(data.gpu_temp_c)+'°C':'N/A';
        $('batteryValue').textContent=ready&&data.battery_percent!=null?Math.round(data.battery_percent)+'%':'N/A';
        $('pcUptime').textContent=ready&&data.pc_uptime_s!=null?'PC Thời gian hoạt động / '+Math.floor(data.pc_uptime_s/3600)+'H '+Math.floor(data.pc_uptime_s%3600/60)+'M':'PC Thời gian hoạt động / —';
        renderEvents(data.events);renderSettings(data.settings);
        $('footerStatus').textContent=ready?(data.demo?'Dữ liệu mô phỏng':'Đang đồng bộ PC'):'Chờ kết nối';
        $('motionBtn').textContent=(data.motion?'Chuyển động: bật':'Chuyển động: tắt');
        $('autoBtn').textContent=(data.auto_pages?'Tự chuyển cảnh: bật':'Tự chuyển cảnh: tắt');
        $('focusBtn').textContent=(data.focus_running?'Ⅱ  Focus':'▶  Resume');
        document.querySelectorAll('[data-media]').forEach(b=>b.disabled=!data.pc_connected||!data.media_available);
        if(data.command_pending)$('musicCommand').textContent='Đang chờ máy tính phản hồi…';
        document.querySelectorAll('[data-action="page"]').forEach(b=>{const active=Number(b.dataset.value)===data.page;b.classList.toggle('active',active);b.setAttribute('aria-current',active?'true':'false');});
        if(emo!==lastEmotion){lastEmotion=emo;$('petImage').src='/api/cat.bmp?state='+encodeURIComponent(emo);}
        $('moodText').textContent=(data.preview_active?'PREVIEW / ':'')+(persona&&!data.preview_active?({cozy:'Thảnh thơi',sleepy:'Đang nghỉ',dancing:'Đang nghe nhạc','deep focus':'Đang tập trung',busy:'Đang bận','too warm':'Nhiệt độ cao','need charge':'Cần sạc',celebrating:'Hoàn thành phiên'}[persona]||persona):(mood[emo]?mood[emo][0]:'STANDBY'));
        const active=Array.isArray(data.events)?data.events.find(e=>e.code===data.active_event):null;
        $('moodCopy').textContent=active&&active.message?active.message:(mood[emo]?mood[emo][1]:mood.rescue[1]);
        document.querySelectorAll('[data-preview]').forEach(b=>{b.classList.toggle('active',!!data.preview_active&&previews[Number(b.dataset.preview)]===emo);});
      }
      async function poll(){if(pollInFlight||streamFresh())return;pollInFlight=true;const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),3000);try{const res=await fetch('/api/status',{cache:'no-store',signal:controller.signal});if(!res.ok)throw new Error('HTTP '+res.status);const data=await res.json();if(!streamFresh())receiveState(data);if(!streamFresh()){$('syncMode').textContent='Đồng bộ định kỳ';$('syncMode').dataset.live='false';}}catch(e){if(!streamFresh()){$('liveLed').classList.remove('live');$('liveText').textContent='Thiết bị mất kết nối';$('footerStatus').textContent='RECONNECTING';$('syncMode').textContent='Offline · reconnecting';}}finally{clearTimeout(timeout);pollInFlight=false;}}
      let fpsHistory=null;
      function drawFpsHistory(){
        const canvas=$('fpsHistoryCanvas');const rect=canvas.getBoundingClientRect();
        const w=Math.max(1,rect.width),h=Math.max(1,rect.height),dpr=Math.min(window.devicePixelRatio||1,2);
        canvas.width=Math.round(w*dpr);canvas.height=Math.round(h*dpr);
        const g=canvas.getContext('2d');if(!g)return;
        g.setTransform(dpr,0,0,dpr,0,0);g.clearRect(0,0,w,h);
        const left=24,right=w-5,top=7,bottom=h-17;
        const y=v=>bottom-Math.max(0,Math.min(35,Number(v)||0))/35*(bottom-top);
        g.font='10px Consolas,monospace';g.textAlign='left';g.lineWidth=1;
        [10,20,30].forEach(v=>{g.strokeStyle='#ded6eb';g.beginPath();g.moveTo(left,y(v));g.lineTo(right,y(v));g.stroke();g.fillStyle='#8b829b';g.fillText(String(v),1,y(v)+3);});
        g.fillStyle='#90869d';g.fillText('−5m',left,bottom+13);g.textAlign='right';g.fillText('NOW',right,bottom+13);
        if(!fpsHistory)return;
        const fps=fpsHistory.fps,target=fpsHistory.target;
        const count=Math.min(60,fps.length,target.length);if(!count)return;
        const x=i=>right-(count-1-i)*(right-left)/59;
        function series(values,color,dashed){g.beginPath();g.strokeStyle=color;g.lineWidth=dashed?1.5:2.4;g.setLineDash(dashed?[5,4]:[]);g.lineJoin='round';for(let i=0;i<count;i++){const xx=x(i),yy=y(values[i]);if(i===0)g.moveTo(xx,yy);else g.lineTo(xx,yy);}g.stroke();g.setLineDash([]);}
        series(target,'#8675c6',true);series(fps,'#da766a',false);
        g.beginPath();g.arc(x(count-1),y(fps[count-1]),3,0,Math.PI*2);g.fillStyle='#da766a';g.fill();
      }
      async function pollFpsHistory(){
        try{
          const res=await fetch('/api/v1/performance',{cache:'no-store'});if(!res.ok)throw new Error('HTTP '+res.status);
          const data=await res.json();if(!Array.isArray(data.fps)||!Array.isArray(data.target))throw new Error('Invalid data');
          fpsHistory=data;const n=Math.min(data.fps.length,data.target.length);
          $('fpsHistorySummary').textContent=n?Number(data.fps[n-1]).toFixed(1)+' / '+data.target[n-1]+' FPS':'COLLECTING…';
          drawFpsHistory();
        }catch(e){$('fpsHistorySummary').textContent='Chưa tải được lịch sử';drawFpsHistory();}
      }
      async function control(action,value=0){const key=pair.value.trim();if(!key){toast('Nhập mã ghép nối để điều khiển thiết bị.');pair.focus();return;}try{const res=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({action,value})});const body=await res.json();if(!res.ok||!body.ok)throw new Error(body.error||'Request failed');toast(action==='preview'?'Đang xem trước mèo trong tám giây':action==='page'?'Đã chuyển cảnh':'Đã áp dụng thay đổi');poll();}catch(e){toast('Chưa gửi được thao tác: '+e.message);}}
      async function mediaControl(action){
        const key=pair.value.trim();if(!key){toast('Nhập mã ghép nối ở mục Thiết bị để điều khiển nhạc.');return;}
        try{
          const res=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({action})});
          const result=await res.json();if(!res.ok||!result.pending)throw new Error(result.error||'Media command rejected');
          $('musicCommand').textContent='COMMAND #'+result.seq+' PENDING';
          for(let attempt=0;attempt<10;attempt++){
            await new Promise(resolve=>setTimeout(resolve,650));
            const response=await fetch('/api/status',{cache:'no-store'});if(!response.ok)continue;
            const status=await response.json();
            if(status.command_seq!==result.seq)break;
            if(status.command_acknowledged){const message=status.command_ok?'WINDOWS ACK / SUCCESS':'WINDOWS DID NOT ACCEPT';$('musicCommand').textContent=message;toast(message);return;}
          }
          $('musicCommand').textContent='Chưa nhận phản hồi · Thử lại';toast('Windows agent did not acknowledge the command');
        }catch(error){$('musicCommand').textContent='UNAVAILABLE';toast(error.message);}
      }
      async function focusControl(action){
        const key=pair.value.trim();if(!key){toast('Enter your pairing key to control Focus');pair.focus();return;}
        try{const res=await fetch('/api/v1/focus',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({action})});
          const result=await res.json();if(!res.ok||!result.ok)throw new Error(result.error||'Focus command failed');updateFocus(result);toast('Focus clock updated on ESP32');}
        catch(error){toast('Focus: '+error.message);}
      }
      async function pollMedia(){
        const key=pair.value.trim();if(!key)return;
        try{
          const res=await fetch('/api/media',{headers:{'X-MEO-KEY':key},cache:'no-store'});if(!res.ok)return;
          renderMedia(await res.json());
        }catch(e){}
      }
      // MEDIA_TIMELINE_BEGIN
      function projectMedia(data,elapsed){
        if(!data.available||elapsed>=3500||(data.source_age_ms||0)+elapsed>=10000)return null;
        const position=Math.max(0,data.position_ms||0)+(data.playing?Math.max(0,elapsed):0);
        return Math.min(position,data.duration_ms||86400000);
      }
      // MEDIA_TIMELINE_END
      let mediaAnchor=null;
      function paintMediaTime(){
        if(!mediaAnchor)return;
        const data=mediaAnchor.data,position=projectMedia(data,performance.now()-mediaAnchor.at),duration=data.duration_ms||0;
        if(position===null){$('musicTime').textContent=data.available?'SYNC INTERRUPTED · waiting for media':'NOT AVAILABLE / N-A';return;}
        $('musicBar').style.width=duration?Math.min(100,Math.round(position*100/duration))+'%':'0%';
        const fmt=n=>Math.floor(n/60000)+':'+String(Math.floor(n/1000)%60).padStart(2,'0');
        $('musicTime').textContent=(data.playing?'PLAYING · ':'PAUSED · ')+fmt(position)+(duration?' / '+fmt(duration):' · DURATION N/A');
      }
      setInterval(paintMediaTime,250);
      function renderMedia(data){
        mediaAnchor={data,at:performance.now()};
        $('musicTitle').closest('.music-studio').classList.toggle('paused',!data.playing);
        $('musicTitle').textContent=data.available?(data.title||'Không có tên bài'):'Chưa phát nhạc';
        $('musicArtist').textContent=data.available?(data.artist||'Không rõ nghệ sĩ'):'Chưa có phiên phát nhạc từ PC';
        if(!data.available)$('musicBar').style.width='0%';
        paintMediaTime();renderArtwork(data);
      }
      function clearArtwork(){const img=$('albumCoverWeb');img.hidden=true;img.removeAttribute('src');img.closest('.music-studio').classList.remove('art-active');if(coverBlobUrl)URL.revokeObjectURL(coverBlobUrl);coverBlobUrl='';coverShownId='';coverPendingId='';}
      async function renderArtwork(data){
        const id=data.artwork_ready&&data.artwork_id?String(data.artwork_id):'';
        if(!id){if(coverShownId||coverPendingId)clearArtwork();return;}
        if(id===coverShownId||id===coverPendingId)return;
        const key=pair.value.trim();if(!key)return;
        coverPendingId=id;
        try{
          const res=await fetch('/api/v1/artwork.bmp',{headers:{'X-MEO-KEY':key},cache:'no-store'});
          if(!res.ok)throw new Error('Artwork unavailable');
          const blob=await res.blob();if(blob.size<54||blob.size>16000)throw new Error('Invalid cover');
          if(coverPendingId!==id)return;
          const url=URL.createObjectURL(blob),old=coverBlobUrl;
          const img=$('albumCoverWeb');img.src=url;img.hidden=false;
          img.closest('.music-studio').classList.add('art-active');
          coverShownId=id;coverBlobUrl=url;coverPendingId='';
          if(old)URL.revokeObjectURL(old);
        }catch(error){if(coverPendingId===id)coverPendingId='';}
      }
      async function saveWifi(){
        const key=pair.value.trim(),ssid=$('newSsid').value.trim(),password=$('newPassword').value;
        if(!key||!ssid){toast('Nhập mã ghép nối and Wi-Fi SSID');return;}
        if(!confirm('Change ESP32 Wi-Fi to "'+ssid+'"? The fallback AP stays available.'))return;
        try{
          const res=await fetch('/api/v1/wifi/config',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({ssid,password})});
          const data=await res.json();if(!res.ok||!data.ok)throw new Error(data.error||'Unable to save');
          $('newPassword').value='';toast('Saved. ESP32 reconnecting; fallback AP remains active.');
        }catch(error){toast('Wi-Fi: '+error.message);}
      }
      async function saveSettings(){const key=pair.value.trim();if(!key){toast('Nhập mã ghép nối trước.');pair.focus();return;}const body={cpu_limit:Number($('cpuLimit').value),gpu_limit:Number($('gpuLimit').value),temp_limit:Number($('tempLimit').value),battery_limit:Number($('batteryLimit').value),quiet:$('quietMode').checked};try{const res=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify(body)});const result=await res.json();if(!res.ok||!result.ok)throw new Error(result.error||'Could not save');settingsDirty=false;toast(result.persisted?'Thresholds saved to ESP32 memory':'Thresholds applied (memory persistence unavailable)');poll();}catch(e){toast('Settings not saved: '+e.message);}}
      document.querySelectorAll('[data-action]').forEach(b=>b.addEventListener('click',()=>control(b.dataset.action,Number(b.dataset.value)||0)));
      for(let i=0;i<4;i++){$('sceneSlot'+i).addEventListener('change',()=>{const chosen=Number($('sceneSlot'+i).value);const slots=[0,1,2,3].map(j=>Number($('sceneSlot'+j).value));if(new Set(slots).size!==4){toast('Chọn bốn cảnh khác nhau.');poll();return;}control('scene_'+i,chosen);});}
      document.querySelectorAll('[data-media]').forEach(b=>b.addEventListener('click',()=>mediaControl(b.dataset.media)));
      document.querySelectorAll('[data-focus]').forEach(b=>b.addEventListener('click',()=>focusControl(b.dataset.focus)));
      $('wifiSave').addEventListener('click',saveWifi);
      $('wifiScan').addEventListener('click',()=>control('scan'));
      $('otaStart').addEventListener('click',uploadOta);
      $('otaFile').addEventListener('change',updateOtaButton);
      $('catPackStart').addEventListener('click',uploadCatPack);
      $('catPackFile').addEventListener('change',updateCatPackButton);
      document.querySelectorAll('[data-preview]').forEach(b=>b.addEventListener('click',()=>control('preview',Number(b.dataset.preview))));
      for(const id of settingIds){$(id).addEventListener('input',()=>{settingsDirty=true;showSettingValues();});}
      $('quietMode').addEventListener('change',()=>{settingsDirty=true;});
      $('settingsSave').addEventListener('click',saveSettings);
      $('pairSave').addEventListener('click',()=>{try{sessionStorage.setItem('meodesk-key',pair.value.trim());toast('Đã lưu mã ghép nối cho phiên này');}catch(e){toast('Mã ghép nối dùng cho đến khi đóng tab');}if(stateSocket&&stateSocket.readyState===WebSocket.OPEN)stateSocket.send(JSON.stringify({type:'subscribe',key:pair.value.trim()}));pollMedia();});
      pair.addEventListener('keydown',e=>{if(e.key==='Enter')$('pairSave').click();});
      document.querySelectorAll('[data-range]').forEach(b=>b.addEventListener('click',()=>{chartState.range=Number(b.dataset.range);chartState.hover=-1;document.querySelectorAll('[data-range]').forEach(el=>el.classList.toggle('active',el===b));if(chartState.history)renderHistory(chartState.history);$('chartTip').classList.remove('visible');}));
      $('historyCanvas').addEventListener('pointermove',moveHistoryHover);
      $('historyCanvas').addEventListener('pointerleave',()=>{chartState.hover=-1;$('chartTip').classList.remove('visible');drawHistory();});
      window.addEventListener('resize',drawHistory);
      window.addEventListener('resize',()=>{const selected=document.querySelector('[data-workspace][aria-selected="true"]');if(selected&&innerWidth<=850)selected.scrollIntoView({block:'nearest',inline:'nearest'});});
      window.addEventListener('resize',drawFpsHistory);
      const packDetails=document.createElement('details');packDetails.className='advanced-pack';const packSummary=document.createElement('summary');packSummary.textContent='Bộ chuyển động · nâng cao';packDetails.append(packSummary);const packSection=document.querySelector('.catpack-panel');packSection.before(packDetails);packDetails.append(packSection);
      const manualDetails=document.createElement('details');manualDetails.className='manual-tools';const manualSummary=document.createElement('summary');manualSummary.textContent='Cập nhật bằng file · công cụ nâng cao';manualDetails.append(manualSummary);const manualSection=document.querySelector('.manual-ota');manualSection.before(manualDetails);manualDetails.append(manualSection);
      document.querySelectorAll('.navlink,.act,.theme-buttons button').forEach(node=>{if(!node.querySelector('svg'))node.textContent=node.textContent.replace(/^[^\p{L}\p{N}]+/u,'').replace(/^\d+\s*\/\s*/,'');});
      try{chooseWorkspace(sessionStorage.getItem('meodesk-workspace')||'overview');}catch(_){chooseWorkspace('overview');}
      setInterval(()=>{if(focusAnchor){const age=performance.now()-focusAnchor.at;if(age<3000)paintFocus(focusAnchor,age);}},200);
      poll();pollHistory();pollMedia();pollFpsHistory();pollOtaStatus();pollCatPackStatus();setInterval(poll,1100);setInterval(pollHistory,3000);setInterval(()=>{if(!streamFresh())pollMedia();},2200);setInterval(pollFpsHistory,5000);setInterval(pollOtaStatus,15000);setInterval(pollCatPackStatus,16000);
    })();
  </script>
</body>
</html>
)MEODESK";
