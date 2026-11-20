#pragma once
#include <Arduino.h>

// Entire web app is served from firmware flash: no CDN, no filesystem mount.
static const char WEB_UI_HTML[] PROGMEM = R"MEODESK(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="theme-color" content="#f3efe7">
  <title>MèoDesk — Personal Studio</title>
  <style>
    :root{--ink:#232631;--muted:#767b86;--paper:#f3efe7;--line:#ddd9d0;--coral:#f08271;--mint:#d0f2db;--green:#2e896b;--lilac:#d6c5ff;--dark:#272934;--white:#fffefa}
    *{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;color:var(--ink);background:var(--paper);font:15px/1.5 "Segoe UI Variable","Segoe UI",Arial,sans-serif;-webkit-font-smoothing:antialiased}
    button,input{font:inherit}button{cursor:pointer}button:focus-visible,input:focus-visible{outline:3px solid #7e9af7;outline-offset:3px}
    .mono,.eyebrow,.counter,.unit,.meta,.metric-label,.pill,.navlink,.num{font-family:Consolas,"Courier New",monospace}
    .wrap{max-width:1240px;margin:auto;padding:0 26px 52px}
    .topbar{display:flex;align-items:center;justify-content:space-between;gap:20px;padding:27px 0 25px;border-bottom:1px solid var(--line)}
    .brand{display:flex;align-items:center;gap:13px;text-decoration:none;color:var(--ink)}
    .brandmark{width:42px;height:42px;border-radius:15px;background:var(--dark);display:grid;place-items:center;position:relative;transform:rotate(-6deg);box-shadow:4px 4px 0 #c7c9d0}
    .brandmark svg{width:24px;height:24px;transform:rotate(6deg)}.brandname{font-size:27px;font-weight:850;letter-spacing:-1.7px;line-height:1}.brandname span{color:#dd786a}
    .brand sub{display:block;font:10px/1.6 Consolas,monospace;letter-spacing:2.3px;color:var(--muted);text-transform:uppercase}
    .topright{display:flex;align-items:center;gap:14px}.connection{display:flex;gap:10px;align-items:center;padding:10px 15px;border:1px solid var(--line);border-radius:30px;background:rgba(255,255,255,.42);font:11px/1.3 Consolas,monospace;font-weight:700;letter-spacing:1px}
    .led{width:8px;height:8px;border-radius:50%;background:#e2aa76;box-shadow:0 0 0 4px #e2aa7620}.led.live{background:#4bc69a;box-shadow:0 0 0 4px #4bc69a20;animation:pulse 2s infinite}.headnote{font:11px/1.5 Consolas,monospace;color:var(--muted);text-transform:uppercase;text-align:right}
    .intro{display:flex;align-items:flex-end;justify-content:space-between;gap:16px;padding:32px 0 28px}.eyebrow{font-size:11px;letter-spacing:2.3px;font-weight:bold;color:#82838a;text-transform:uppercase}.intro h1{margin:4px 0 0;font-weight:820;font-size:clamp(34px,4.2vw,57px);line-height:1.07;letter-spacing:-2.8px}.intro h1 em{font-style:normal;color:#df806e}
    .intro p{max-width:260px;font-size:13px;line-height:1.65;color:#7d7d86;margin:0 2px 5px 0}
    .main-grid{display:grid;grid-template-columns:minmax(0,1.1fr) minmax(370px,.9fr);gap:18px;align-items:stretch}
    .pet-card{position:relative;min-height:385px;background:var(--mint);border-radius:29px 12px 29px 12px;overflow:hidden;padding:30px 30px 25px;isolation:isolate}
    .pet-card:before{content:"";position:absolute;inset:-120px -50px auto auto;width:330px;height:330px;border:1px solid rgba(46,137,107,.19);border-radius:50%;box-shadow:0 0 0 35px rgba(46,137,107,.045),0 0 0 90px rgba(46,137,107,.04);z-index:-1}
    .pet-card:after{content:"";position:absolute;left:-40px;bottom:-100px;width:350px;height:200px;background:radial-gradient(ellipse,#8fcbad33,transparent 68%);z-index:-1}
    .pet-top{display:flex;align-items:center;justify-content:space-between}.pet-top .eyebrow{color:#47856c}.round-icon{width:39px;height:39px;border:1px solid #84b9a1;border-radius:50%;display:grid;place-items:center;color:#398565;font-size:18px}
    .pet-control{background:#ddf8e5;transition:transform .2s,background .2s;font-weight:700}.pet-control:hover{background:#fffdf8;transform:scale(1.1)}
    .pet-copy{position:relative;z-index:2;max-width:53%;padding-top:51px}.pet-copy h2{font-size:clamp(30px,3.3vw,46px);line-height:1.05;letter-spacing:-2px;margin:0 0 16px;font-weight:820}.pet-copy h2 span{display:block;font-family:Georgia,serif;font-size:1.13em;font-style:italic;font-weight:500;letter-spacing:-1.5px;color:#2b7961}
    .pet-copy p{font-size:13px;line-height:1.7;color:#5e8475;max-width:240px;margin:0}.pet-art{position:absolute;width:44%;max-width:260px;min-width:175px;right:20px;bottom:28px;transform:rotate(2deg);filter:drop-shadow(0 19px 13px #387b5b2c);animation:float 4s ease-in-out infinite}
    .pet-art img{width:100%;height:auto;aspect-ratio:1;object-fit:contain;border-radius:23%;mix-blend-mode:multiply;display:block}
    .pet-foot{display:flex;justify-content:space-between;align-items:flex-end;gap:12px;position:absolute;bottom:26px;left:30px;right:30px;z-index:3;pointer-events:none}.mood{display:inline-flex;align-items:center;gap:9px;background:#f3fff4e5;border:1px solid #abdebc;border-radius:25px;padding:8px 12px;color:#257058;font:700 11px Consolas,monospace;letter-spacing:.7px}
    .mood .dot{width:7px;height:7px;background:#65bb8e;border-radius:50%}.pet-index{font:12px Consolas,monospace;color:#5b957b}
    .metric-card{background:var(--dark);color:#f7f5f0;border-radius:12px 29px 12px 29px;padding:24px 27px 22px;display:flex;flex-direction:column;min-height:385px;box-shadow:0 8px 0 #dad4c9}
    .metric-top{display:flex;align-items:center;justify-content:space-between;gap:20px}.metric-top .eyebrow{color:#a7a6b6}.tag{border:1px solid #555868;padding:7px 11px;border-radius:30px;font:10px Consolas,monospace;letter-spacing:.8px;color:#c4c3cd;white-space:nowrap}
    .cpu-head{display:flex;align-items:center;gap:19px;flex:1;padding:21px 0 17px}.cpu-ring{width:148px;height:148px;flex:none;padding:12px;border-radius:50%;background:conic-gradient(var(--coral) 0%,#4a4b58 0%);transition:background .6s;box-shadow:inset 0 0 0 1px #555764}
    .cpu-inner{width:100%;height:100%;background:var(--dark);border-radius:50%;display:flex;align-items:center;justify-content:center;flex-direction:column}
    .cpu-inner strong{font:700 36px/1 Consolas,monospace;letter-spacing:-2px}.cpu-inner span{font:11px Consolas,monospace;letter-spacing:2px;color:#a8a9b6;margin-top:6px}
    .cpu-side{min-width:0}.cpu-side .metric-label{color:#a3a6b5;font-size:10px;letter-spacing:1.5px}.cpu-side h3{font-size:23px;letter-spacing:-.8px;line-height:1.2;margin:8px 0 10px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.cpu-side p{font-size:12px;color:#abaeba;line-height:1.5;margin:0}
    .bars{border-top:1px solid #50515e;padding-top:15px}.barline{display:flex;justify-content:space-between;align-items:center;margin:9px 0 7px}.barline strong{font:700 16px Consolas,monospace}.metric-label{font-size:11px;letter-spacing:1.5px;color:#bcbeca;font-weight:bold;text-transform:uppercase}.track{height:7px;border-radius:9px;background:#525560;overflow:hidden}.fill{height:100%;width:0%;border-radius:9px;transition:width .55s cubic-bezier(.22,.8,.3,1)}.gpu-fill{background:var(--lilac)}.ram-fill{background:#9de0c0}
    .tiny-row{display:flex;justify-content:space-between;gap:8px;color:#999eac;font:11px Consolas,monospace;padding-top:10px;min-height:30px}
    .history-panel{margin-top:19px;background:#fffdfa;border:1px solid #e4dfd5;border-radius:13px 29px 13px 29px;padding:24px 28px 20px;box-shadow:0 6px 0 #e5e0d6}
    .history-top{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;flex-wrap:wrap}.history-top .eyebrow{color:#b0796a}.history-top h2{font-size:25px;letter-spacing:-1px;line-height:1.15;margin:7px 0 8px}.history-sub{margin:0;color:#83828a;font-size:12px}
    .history-ranges{display:flex;align-items:center;gap:4px;background:#f1ede7;border:1px solid #e8e3dc;border-radius:12px;padding:4px}.range-btn{border:0;background:transparent;color:#86858f;font:700 11px Consolas,monospace;padding:10px 13px;border-radius:9px}.range-btn.active{background:#30333e;color:white;box-shadow:0 3px 8px #33333b20}
    .history-layout{display:grid;grid-template-columns:minmax(0,1fr) 178px;gap:23px;margin-top:22px}.graph-wrap{position:relative;min-width:0;background:linear-gradient(180deg,#fcfaf5,#f8f5ef);border:1px solid #f0ebe4;border-radius:16px;overflow:hidden;padding:7px 5px 0}.graph-wrap canvas{display:block;width:100%;height:232px;touch-action:pan-y}.chart-tip{position:absolute;z-index:2;background:#292d37;color:#fff;padding:8px 11px;border-radius:8px;pointer-events:none;white-space:nowrap;font:11px/1.65 Consolas,monospace;box-shadow:0 5px 15px #292b351f;transform:translate(-50%,-110%);visibility:hidden}.chart-tip.visible{visibility:visible}
    .history-aside{display:flex;flex-direction:column;justify-content:center;gap:12px}.peak{position:relative;border-bottom:1px solid #e9e4de;padding:0 0 13px 13px}.peak:before{position:absolute;left:0;top:4px;content:"";width:5px;height:27px;background:#ed8576;border-radius:6px}.peak.gpu:before{background:#a58bdc}.peak.ram:before{background:#57b69a}.peak span{display:block;font:10px Consolas,monospace;color:#898891;letter-spacing:.8px}.peak strong{display:block;font:700 27px/1.25 Consolas,monospace;color:#353744;margin-top:3px;letter-spacing:-1.1px}.history-foot{display:flex;justify-content:space-between;gap:10px;align-items:center;flex-wrap:wrap;border-top:1px solid #eeeae3;margin-top:17px;padding-top:13px;font:10px Consolas,monospace;color:#908c91;letter-spacing:.5px}.legend{display:flex;gap:15px;flex-wrap:wrap}.legend b{display:flex;gap:6px;align-items:center;font-weight:600}.legend b:before{content:"";width:10px;height:4px;border-radius:4px;background:var(--coral)}.legend b:nth-child(2):before{background:#a58bdc}.legend b:nth-child(3):before{background:#57b69a}
    .section-top{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:37px 0 15px}.section-top h2{font-size:21px;letter-spacing:-.6px;margin:0}.section-top small{font:11px Consolas,monospace;letter-spacing:.8px;color:#92919a}
    .lower-grid{display:grid;grid-template-columns:minmax(0,1.2fr) minmax(310px,.8fr);gap:18px}
    .control-card,.link-card{border-radius:24px;background:#fffdfa;border:1px solid #e3dfd7;padding:23px 25px}
    .control-card{position:relative}.control-card:before{content:"05 / CONTROL";position:absolute;right:22px;top:25px;font:10px Consolas,monospace;color:#a5a2a1;letter-spacing:1px}
    .panel-title{font-size:13px;letter-spacing:.4px;font-weight:750;margin:0 0 18px}.nav{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:20px}.navlink{font-size:11px;font-weight:700;letter-spacing:.5px;border-radius:40px;padding:10px 12px;border:1px solid #dedcd7;background:#f8f7f3;color:#676b75;transition:.2s}.navlink:hover{border-color:#f0a394;color:#aa4f45}.navlink.active{background:var(--dark);color:white;border-color:var(--dark)}
    .actions{display:flex;gap:9px;flex-wrap:wrap}.act{border:1px solid #dcd9d3;background:#fff;border-radius:12px;padding:10px 15px;font-size:12px;font-weight:750;color:#55545c;display:flex;align-items:center;gap:9px}.act:hover{border-color:#ee9c8f;transform:translateY(-1px)}.act.primary{background:#ed8576;border-color:#ed8576;color:#231e22}
    .link-card{background:#ece6f7;border-color:#ece6f7;display:flex;flex-direction:column;justify-content:space-between}.link-card .eyebrow{color:#76708a}.device-row{display:flex;align-items:center;gap:11px;margin:8px 0}.small-square{width:38px;height:38px;border-radius:11px;background:#ded3f2;display:grid;place-items:center;font-size:21px}.device-row h3{margin:0;font-size:17px;letter-spacing:-.5px}.device-row small{font:11px Consolas,monospace;color:#777389}
    .device-details{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin:17px 0}.device-details dt{font:10px Consolas,monospace;letter-spacing:1px;color:#817c91;text-transform:uppercase}.device-details dd{font:700 14px Consolas,monospace;margin:4px 0 0;color:#383447;overflow-wrap:anywhere}
    .fps-history{margin:2px 0 18px;padding:14px 15px 11px;background:#f8f4ff;border:1px solid #d8cfea;border-radius:15px}.fps-history-head{display:flex;justify-content:space-between;gap:10px;align-items:center;font:700 10px Consolas,monospace;letter-spacing:.7px;color:#726789}.fps-history-head strong{font-size:12px;letter-spacing:0;color:#383447}.fps-history canvas{display:block;width:100%;height:94px;margin-top:11px}.fps-history-legend{display:flex;gap:13px;flex-wrap:wrap;margin-top:5px;font:10px Consolas,monospace;color:#777087}.fps-history-legend span:before{content:"";display:inline-block;width:12px;height:3px;margin-right:6px;vertical-align:middle;border-radius:3px;background:#da766a}.fps-history-legend span:last-child:before{background:#8675c6}
    .pair{display:flex;gap:8px;align-items:center;background:#fff9;border:1px solid #d9d0ec;padding:8px;border-radius:13px}.pair input{width:100%;min-width:0;border:none;background:transparent;font:12px Consolas,monospace;padding:6px 8px;color:#383447;outline:none}.pair button{border:0;border-radius:9px;background:var(--dark);color:white;font:11px Consolas,monospace;font-weight:700;padding:10px 13px;white-space:nowrap}
    .health-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:18px}.health-story{background:#20483f;color:#e9faed;border-radius:29px 12px 29px 12px;padding:26px 29px;position:relative;overflow:hidden}.health-story:after{content:"";position:absolute;width:280px;height:280px;right:-160px;top:-150px;border-radius:50%;box-shadow:0 0 0 45px #97d8ad12,0 0 0 105px #97d8ad0b}.health-story .eyebrow{color:#9edbc0}.health-story h3{font:italic 30px/1.1 Georgia,serif;letter-spacing:-.7px;margin:14px 0 23px}.health-metrics{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;border-bottom:1px solid #ffffff31;padding-bottom:19px}.health-metrics small{display:block;color:#b3d4c8;font:10px Consolas,monospace;letter-spacing:.7px}.health-metrics strong{display:block;font:700 22px/1.3 Consolas,monospace;margin-top:5px}.events-heading{margin:21px 0 10px;font:11px Consolas,monospace;letter-spacing:1.5px;color:#aed0bf}.events{list-style:none;padding:0;margin:0;display:grid;gap:9px}.events li{display:flex;gap:9px;align-items:baseline;color:#dcf0e5;font-size:12px}.events li:before{content:"";width:6px;height:6px;flex:none;background:#eeb28d;border-radius:50%}.events time{margin-left:auto;white-space:nowrap;color:#b0d1c0;font:10px Consolas,monospace}
    .health-settings{background:#fff9f2;border-radius:12px 29px 12px 29px;border:1px solid #eedbcb;padding:26px 29px}.settings-title{font-size:24px;font-weight:800;letter-spacing:-.8px;margin:8px 0 7px}.settings-note{font-size:12px;color:#85818a;margin:0 0 19px}.setting{display:grid;grid-template-columns:1fr auto;gap:2px 10px;margin:14px 0}.setting label{font-size:12px;font-weight:750;color:#555362}.setting output{font:700 12px Consolas,monospace;color:#b65e50}.setting input[type=range]{grid-column:1/-1;width:100%;accent-color:#d77668;cursor:pointer}.quiet-row{display:flex;gap:11px;align-items:center;padding:14px 0 13px;border-top:1px solid #f0dacc;font-size:12px;color:#686474}.quiet-row input{accent-color:#cb786c;width:17px;height:17px}.settings-save{background:#292b35;border:0;border-radius:13px;padding:12px 19px;color:#fff;font-weight:750;font-size:12px;width:100%}.settings-save:hover{background:#4a4652}
    .mood-intro{display:flex;justify-content:space-between;gap:20px;align-items:center;margin:37px 0 14px}.mood-intro h2{font-size:21px;letter-spacing:-.6px;margin:0}.mood-intro p{font:11px Consolas,monospace;color:#89828d;margin:0}.mood-gallery{display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:10px}.mood-tile{border:1px solid #e6e0d7;background:#fffdfa;border-radius:19px 8px 19px 8px;padding:12px 6px 11px;min-width:0;transition:transform .2s,border-color .2s;display:flex;flex-direction:column;align-items:center;gap:6px}.mood-tile:hover{border-color:#ed8e7d;transform:translateY(-4px)}.mood-tile.active{border:2px solid #d98273;background:#fff4ef;padding:11px 5px 10px}.mood-tile img{width:78px;max-width:100%;aspect-ratio:1;object-fit:contain;mix-blend-mode:multiply}.mood-tile b{font:700 10px Consolas,monospace;letter-spacing:.2px;color:#69626c}.mood-tile small{font:9px Consolas,monospace;color:#99929a}
    .status-strip{display:flex;align-items:center;gap:11px;border-top:1px solid var(--line);padding-top:19px;margin-top:25px;color:#8a8a91;font:11px Consolas,monospace}.status-strip span:nth-child(2){flex:1}.signature{font-family:Georgia,serif;font-style:italic;font-size:17px;color:#c37c70}
    #toast{position:fixed;z-index:100;bottom:24px;left:50%;transform:translate(-50%,110px);background:#242630;color:#fff;padding:13px 20px;border-radius:14px;box-shadow:0 12px 40px #20202545;font:13px "Segoe UI",sans-serif;transition:transform .3s;max-width:calc(100vw - 32px)}#toast.show{transform:translate(-50%,0)}
    @keyframes float{0%,100%{transform:translateY(0) rotate(2deg)}50%{transform:translateY(-9px) rotate(-1deg)}}@keyframes pulse{50%{box-shadow:0 0 0 7px #4bc69a08}}
    @media (max-width:850px){.main-grid,.lower-grid,.health-grid{grid-template-columns:1fr}.intro h1{font-size:40px}.intro p{max-width:220px}.pet-card{min-height:340px}.metric-card{min-height:auto}.cpu-head{justify-content:center}.lower-grid{gap:12px}.mood-gallery{grid-template-columns:repeat(4,minmax(0,1fr))}.history-layout{grid-template-columns:minmax(0,1fr) 140px;gap:14px}}
    @media (max-width:540px){.wrap{padding:0 15px 28px}.topbar{padding:18px 0}.brandname{font-size:23px}.brandmark{width:37px;height:37px}.headnote{display:none}.connection{font-size:9px;padding:9px}.intro{padding:28px 2px 20px;display:block}.intro h1{font-size:38px;letter-spacing:-2px}.intro p{max-width:100%;margin-top:10px}.main-grid{gap:12px}.pet-card{padding:22px;min-height:320px}.pet-copy{max-width:51%;padding-top:44px}.pet-copy h2{font-size:31px}.pet-copy p{font-size:12px}.pet-art{width:49%;min-width:145px;right:7px;bottom:42px}.pet-foot{left:20px;right:20px;bottom:19px}.metric-card{padding:20px 22px}.cpu-ring{width:128px;height:128px}.cpu-inner strong{font-size:30px}.section-top{margin:30px 0 13px}.control-card,.link-card,.health-story,.health-settings{padding:20px}.navlink{font-size:10px;padding:9px 11px}.health-metrics strong{font-size:19px}.mood-gallery{grid-template-columns:repeat(4,minmax(0,1fr));gap:7px}.mood-tile{padding:8px 3px}.mood-tile img{width:63px}.mood-tile b{font-size:9px}.mood-intro{align-items:flex-start;flex-direction:column;gap:5px}.status-strip{font-size:10px;flex-wrap:wrap}}
    @media (max-width:540px){.history-panel{padding:21px 16px 17px}.history-top h2{font-size:23px}.history-ranges{margin-top:4px}.history-layout{grid-template-columns:1fr;gap:14px;margin-top:17px}.graph-wrap canvas{height:205px}.history-aside{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:7px}.peak{padding:0 0 4px 9px;border:0}.peak:before{width:3px;height:23px}.peak strong{font-size:20px}.peak span{font-size:8px}.history-foot{font-size:9px}}
    @media(prefers-reduced-motion:reduce){*,*:before,*:after{animation:none!important;transition:none!important;scroll-behavior:auto!important}}
    .feature-grid{display:grid;grid-template-columns:minmax(0,1.12fr) minmax(0,.88fr);gap:18px;margin:33px 0 9px;align-items:stretch}
    .music-studio{position:relative;background:#303340;color:#fdf8f2;border-radius:30px 13px 30px 13px;padding:27px 29px;overflow:hidden;isolation:isolate}
    .music-studio:after{content:"♫";position:absolute;right:13px;top:-50px;color:#ffffff09;font:260px Georgia,serif;z-index:-1}
    .music-studio .eyebrow{color:#c9b5ff}.music-studio h2{font:italic 35px/1.1 Georgia,serif;letter-spacing:-.6px;margin:15px 0 6px}
    .music-studio .artist{color:#b9bac8;font-size:13px;min-height:21px}.music-studio .music-time{font:11px Consolas,monospace;color:#c5c1ce;margin:11px 0 17px}
    .music-studio .track{background:#585567;height:7px;border-radius:7px;margin-top:22px}.music-studio .track div{background:#cbb6fa;height:100%;border-radius:7px;width:0%;transition:width .4s}
    .music-actions{display:flex;gap:9px;align-items:center}.music-actions button{width:48px;height:47px;background:#4b4a59;border:1px solid #656475;color:#fff;border-radius:15px;font-size:18px}
    .music-actions button.main{background:#cfb8fb;color:#2d2938;border-color:#cfb8fb;width:61px}.music-actions button:disabled{opacity:.45;cursor:not-allowed}
    .music-status{font:10px Consolas,monospace;color:#bcb3cf;margin-left:auto;max-width:165px}
    .wifi-studio{background:#e9dfef;border-radius:13px 29px 13px 29px;padding:27px 28px;display:flex;flex-direction:column;gap:12px}
    .wifi-studio .eyebrow{color:#796587}.wifi-studio h2{font-size:25px;letter-spacing:-1px;line-height:1.2;margin:1px 0 4px}
    .wifi-studio label{display:block;color:#625a6e;font:700 10px Consolas,monospace;letter-spacing:1px}
    .wifi-studio input{width:100%;border:1px solid #d4c4db;border-radius:11px;background:#fefbff;padding:12px;color:#403749}
    .wifi-studio button{background:#34313d;color:#fff;border:0;border-radius:11px;font-weight:700;font-size:12px;padding:13px 16px}
    .wifi-note{font-size:11px;line-height:1.6;color:#80738c;margin:0}
    .scene-shortcuts{display:flex;flex-wrap:wrap;gap:9px;margin:28px 0 4px}.scene-shortcuts button{border-radius:999px;border:1px solid #d8d4cb;background:#fffdfa;padding:10px 16px;color:#56565a;font:700 12px Consolas,monospace}
    .scene-shortcuts button:hover{background:#d6eedc;border-color:#afd9bf}
    .living-grid{display:grid;grid-template-columns:minmax(0,.95fr) minmax(0,1.05fr);gap:18px;margin-top:19px}
    .focus-studio{background:#e2d7f3;color:#332f48;border-radius:29px 12px 29px 12px;padding:26px 29px;position:relative;overflow:hidden}
    .focus-studio:after{content:"◷";position:absolute;top:-56px;right:-5px;font:240px Georgia,serif;color:#a48abc20;pointer-events:none}
    .focus-studio .eyebrow{color:#75648c}.focus-clock{font:800 clamp(58px,6vw,87px)/1.1 Consolas,monospace;letter-spacing:-5px;margin:12px 0 0;position:relative}
    .focus-sub{font:700 12px Consolas,monospace;letter-spacing:.5px;color:#77668a;margin-bottom:18px}.focus-track{height:7px;background:#c6b7dc;border-radius:8px;overflow:hidden;margin-bottom:16px}.focus-track div{width:0;height:100%;background:#735fa1;transition:width .4s}
    .focus-studio .actions{position:relative}.focus-studio .act{background:#fcfaff;border-color:#c9bae0}.focus-studio .act.primary{background:#373047;color:#fff;border-color:#373047}
    .living-info{background:#fffefa;border:1px solid #e6e1d9;border-radius:12px 29px 12px 29px;padding:26px 29px}
    .living-info h3{font-size:23px;letter-spacing:-.8px;margin:8px 0 14px}.ambient-metrics{display:grid;grid-template-columns:1fr 1fr;gap:13px;margin-bottom:20px}
    .ambient-metrics div{padding:12px 14px;border-radius:12px;background:#f7f3ed}.ambient-metrics small{display:block;font:10px Consolas,monospace;letter-spacing:1px;color:#84808b}.ambient-metrics strong{display:block;font:700 21px Consolas,monospace;color:#38343c;margin-top:5px}
    .theme-buttons{display:flex;flex-wrap:wrap;gap:8px}.theme-buttons button{border-radius:10px;border:1px solid #dfd7cf;background:#fffefa;padding:10px 15px;font:700 11px Consolas,monospace;color:#686371}.theme-buttons button.active{background:#33323e;border-color:#33323e;color:white}
    .motion-signal{display:flex;flex-wrap:wrap;align-items:center;gap:8px;margin-top:12px}.motion-signal span{border:1px solid #dcd1e2;border-radius:9px;padding:8px 10px;background:#f8f5fb;color:#716182;font:700 10px Consolas,monospace;letter-spacing:.3px}.motion-signal .motion-state{background:#e3d7f2;color:#49355d;border-color:#e3d7f2}
    .ota-panel{display:grid;grid-template-columns:minmax(0,1fr) minmax(320px,.85fr);gap:22px;margin-top:22px;padding:26px 29px;background:#273a34;color:#f2faf3;border-radius:13px 29px 13px 29px;position:relative;overflow:hidden}.ota-panel .eyebrow{color:#b4d7c2}.ota-panel h3{font:italic 31px/1.16 Georgia,serif;letter-spacing:-.5px;margin:11px 0 12px}.ota-panel p{font-size:12px;line-height:1.75;color:#bdd0c5;max-width:490px}.ota-checks{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:20px}.ota-checks small{display:block;font:10px Consolas,monospace;color:#91b2a1;letter-spacing:.5px}.ota-checks strong{display:block;font:700 14px Consolas,monospace;color:#f1f9f2;margin-top:5px}.ota-upload{background:#f8faf5;border-radius:17px;padding:20px;color:#303e39}.ota-upload label{display:block;font:700 10px Consolas,monospace;letter-spacing:1px;margin-bottom:9px;color:#607367}.ota-upload input{width:100%;color:#45554b;font:11px Consolas,monospace}.ota-upload input::file-selector-button{border:1px solid #cadccd;background:white;padding:10px 12px;border-radius:9px;color:#30493c;margin-right:12px;font-weight:700}.ota-upload button{display:block;width:100%;margin-top:13px;padding:13px 16px;background:#337b5f;color:white;border:0;border-radius:11px;font:700 12px Consolas,monospace}.ota-upload button:disabled{opacity:.5;cursor:not-allowed}.ota-progress{height:9px;border-radius:7px;background:#deede1;overflow:hidden;margin-top:17px}.ota-progress div{height:100%;width:0;background:#db8e7e;transition:width .25s}.ota-result{font:11px/1.5 Consolas,monospace;color:#536a5d;margin-top:10px;min-height:16px}.ota-upload .ota-warning{font:10px/1.5 Consolas,monospace;color:#8a6b61;margin-top:12px}.catpack-panel{background:#393348;border-radius:29px 13px 29px 13px}.catpack-panel .eyebrow{color:#e3b5d9}.catpack-panel .ota-checks small{color:#c1b4cd}.catpack-panel .ota-checks strong{color:#fff5fd}.catpack-panel .ota-upload button{background:#7957a5}.catpack-panel .ota-progress div{background:#b68be0}.catpack-panel p{color:#d5c8de}
    .event-label{display:inline-block;font:700 9px Consolas,monospace;color:#372f44;background:#e1d6f4;padding:3px 6px;border-radius:5px;margin-right:8px}
    .events li.priority-3:before{background:#fa8876}.events li.priority-2:before{background:#e6b568}.events li.priority-1:before{background:#a1d9c1}
    .scene-studio{display:grid;grid-template-columns:minmax(0,.8fr) minmax(0,1.2fr);gap:20px;margin-top:18px;border-radius:13px 29px 13px 29px;background:#e9f2e9;border:1px solid #d3e4d5;padding:25px 29px}
    .scene-studio h3{margin:9px 0 8px;font-size:27px;line-height:1.12;letter-spacing:-1px}.scene-studio p{font-size:12px;color:#6c8578;line-height:1.7;margin:0}
    .scene-selects{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
    .scene-selects label{font:700 10px Consolas,monospace;color:#5c7d6d;letter-spacing:.8px}
    .scene-selects select{display:block;width:100%;background:#fffefa;color:#34423b;border:1px solid #cbdcd0;border-radius:11px;padding:11px;margin-top:5px;font:700 12px Consolas,monospace;cursor:pointer}
    .scene-tip{display:block;margin-top:11px;font:10px Consolas,monospace;color:#669179}
    .music-studio.art-active h2,.music-studio.art-active .artist{max-width:calc(100% - 125px)}
    .album-image{position:absolute;top:61px;right:28px;width:108px;height:108px;object-fit:cover;border-radius:16px;box-shadow:0 10px 22px #1717227c;border:3px solid #ffffff18}
    .album-image[hidden]{display:none}
    @media(max-width:850px){.feature-grid,.living-grid,.scene-studio,.ota-panel{grid-template-columns:1fr}}
    @media(max-width:540px){.scene-studio{padding:20px}.scene-selects{grid-template-columns:repeat(2,minmax(0,1fr))}.album-image{width:84px;height:84px;top:59px;right:19px}.music-studio.art-active h2,.music-studio.art-active .artist{max-width:calc(100% - 98px)}}
    /* Operate mode: compact workspace, one task at a time. */
    :root{--space:16px;--muted:#626975;scrollbar-color:#b8b3a9 var(--paper)}
    ::selection{background:#bce7d2;color:#173e30}input{caret-color:var(--green)}
    [hidden]{display:none!important}.wrap{max-width:1360px;padding:0 24px 24px}
    .topbar{padding:16px 0}.brandmark{box-shadow:none;transform:none;border-radius:12px}.brandmark svg{transform:none}
    .brand sub{letter-spacing:1px}.headnote{display:none}.intro{padding:20px 0 16px;align-items:center}
    .intro h1{font-size:30px;letter-spacing:-.8px}.intro p{max-width:340px;margin:0;color:#626975}
    .studio-nav{display:flex;align-items:center;gap:4px;border-bottom:1px solid var(--line);margin-bottom:16px;padding-bottom:8px;flex-wrap:wrap}
    .studio-nav button{border:0;border-radius:8px;padding:10px 16px;background:transparent;color:#565d66;font-weight:650;font-size:13px;min-height:40px}
    .studio-nav button[aria-selected=true]{background:#263e34;color:#fff}.studio-nav button:hover{background:#dce8de;color:#203f32}
    .sync-mode{margin-left:auto;font:11px Consolas,monospace;color:#52645a;padding:8px}.sync-mode[data-live=true]{color:#236447}
    .workspace-panel{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px;align-items:start}
    .workspace-panel>*{min-width:0;margin:0}.workspace-panel>.main-grid,.workspace-panel>.history-panel,.workspace-panel>.scene-studio,.workspace-panel>.catpack-panel,.workspace-panel>.ota-panel,.workspace-panel>.mood-gallery,.workspace-panel>.mood-intro{grid-column:1/-1}
    .workspace-panel .main-grid{grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:16px}
    .pet-card,.metric-card{min-height:250px;border-radius:14px;padding:20px;box-shadow:none}.pet-card:before,.pet-card:after{display:none}
    .pet-copy{padding-top:16px;max-width:57%}.pet-copy h2{font-size:29px;letter-spacing:-.7px;margin-bottom:10px}.pet-copy p{font-size:12px;line-height:1.5;color:#38624f}
    .pet-art{max-width:164px;min-width:120px;width:36%;right:16px;bottom:35px;filter:none;animation:none}.pet-foot{left:20px;right:20px;bottom:16px}.pet-index{font-size:10px}.mood{padding:5px 9px}
    .metric-card{gap:8px}.cpu-head{padding:0;gap:16px}.cpu-ring{width:96px;height:96px;padding:8px;box-shadow:none}.cpu-inner strong{font-size:29px}.cpu-inner span{font-size:9px;letter-spacing:.5px}.cpu-side h3{font-size:18px;margin:4px 0}.cpu-side p{font-size:11px}.bars{padding-top:6px}.barline{margin:3px 0}.tiny-row{padding-top:6px;min-height:20px}
    .control-card,.link-card,.health-story,.health-settings,.focus-studio,.living-info,.scene-studio,.music-studio,.wifi-studio,.ota-panel{padding:20px;border-radius:14px;box-shadow:none}
    .control-card:before,.health-story:after,.music-studio:after,.focus-studio:after{display:none}.panel-title{margin-bottom:12px}.nav{gap:6px;margin-bottom:12px}.navlink{padding:8px 10px;border-radius:7px;min-height:36px}.act{padding:8px 12px;min-height:36px;border-radius:8px}
    .history-panel{padding:16px 20px;border-radius:14px;box-shadow:none}.history-top h2{font-size:19px;margin:0 0 4px;letter-spacing:-.4px}.history-layout{margin-top:12px;gap:16px}.graph-wrap{border:0;background:#f8f5ef;border-radius:8px}.graph-wrap canvas{height:150px}.history-foot{margin-top:10px;padding-top:8px}.history-sub{color:#626975}.peak strong{font-size:23px}.peak:before{width:3px}
    .focus-clock{font-size:56px!important;margin:8px 0!important}.focus-studio,.music-studio{min-height:250px}.music-studio h2{font-size:25px;margin:8px 0 12px}.music-studio .artist{margin-bottom:20px}.music-actions{margin-top:16px}.health-story h3,.living-info h3,.wifi-studio h2,.settings-title{font-size:22px;line-height:1.2;margin:0 0 14px}.settings-note{color:#626975}
    .theme-buttons{gap:6px;flex-wrap:wrap;margin:8px 0 14px}.theme-buttons button{padding:9px 10px;min-height:36px;border-radius:8px}.wifi-note{line-height:1.5;color:#626975}.ambient-metrics{margin:0 0 16px}.motion-signal{gap:8px}.living-info .wifi-note{font-size:11px}
    .scene-studio{grid-template-columns:1fr 1fr;gap:16px}.scene-studio h3{font-size:20px;margin:0 0 8px}.scene-studio p{color:#52645a;margin:0}.scene-tip{margin-top:8px}.scene-selects select{min-height:38px}
    .mood-intro{display:none}.mood-tile{border-radius:10px;padding:8px}.mood-tile img{width:60px}.mood-tile small{color:#626975}.mood-tile:hover{transform:none}.mood-gallery{gap:8px}.page-shortcuts{display:none}
    .ota-panel{grid-template-columns:1fr 1fr;gap:24px}.ota-panel h3{font-size:24px;line-height:1.15;margin:0 0 12px}.ota-panel p{font-size:12px;line-height:1.55}.ota-checks{gap:10px}.ota-upload{padding:0;background:transparent}.ota-upload input{max-width:100%}.ota-result{min-height:0}.status-strip{margin-top:20px;padding-top:12px;font-size:10px}.signature{font-size:15px}
    .pack-preview{display:flex;align-items:center;gap:16px;margin-bottom:14px}.pack-preview canvas{width:112px;height:112px;border-radius:8px;background:#fff8f1;image-rendering:pixelated}.pack-preview label{display:grid;gap:6px;font-size:12px}.pack-preview select{max-width:160px;padding:8px;border:1px solid #c9c0d0;border-radius:8px;background:#fffefa;color:#28312c}
    .motion-help{border-top:1px solid var(--line);padding-top:12px;margin-top:12px;color:#52645a;font-size:12px}.motion-help summary{cursor:pointer;min-height:28px}.health-story .ambient-metrics{margin:16px 0 0;gap:12px}.health-story .ambient-metrics>div{background:transparent;padding:0}.health-story .ambient-metrics small{color:#b3d4c8}.health-story .ambient-metrics strong{color:#e9faed;font-size:16px}
    .lab-panel{grid-column:1/-1;background:#fffefa;border:1px solid var(--line);border-radius:14px;padding:20px}.lab-head{display:flex;justify-content:space-between;align-items:center;gap:16px}.lab-head h3{margin:0;font-size:21px}.lab-panel p{color:#52645a;font-size:12px;max-width:75ch}.lab-metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:16px;margin:18px 0 0}.lab-metrics dt{font-size:11px;color:#626975}.lab-metrics dd{font:700 19px Consolas,monospace;margin:4px 0 0;color:#263e34}.lab-panel button:disabled{opacity:.6;cursor:not-allowed}@media(max-width:540px){.lab-head{align-items:flex-start;flex-direction:column}.lab-metrics{grid-template-columns:repeat(2,minmax(0,1fr))}}
    .focus-config{display:flex;flex-wrap:wrap;gap:10px;margin-top:18px;padding-top:16px;border-top:1px solid #c6b4df;align-items:end}.focus-config label{display:grid;gap:4px;font-size:11px;flex:1;min-width:70px}.focus-config input{width:100%;background:#fffefa;border:1px solid #cbbdd8;border-radius:7px;padding:8px;color:#332e42}#configFile{max-width:100%;font-size:12px}
    .ota-panel .ota-upload label,.ota-panel .ota-upload input,.ota-panel .ota-result{color:#e1eee6}.ota-panel .ota-upload .ota-warning{color:#c5d9cd}.catpack-panel .ota-upload label,.catpack-panel .ota-upload input,.catpack-panel .ota-result,.catpack-panel .pack-preview small{color:#eee3f4}.catpack-panel .ota-upload .ota-warning{color:#d5c8de}
    @media(max-width:850px){.workspace-panel,.workspace-panel .main-grid{grid-template-columns:1fr}.workspace-panel>*{grid-column:1!important}.pet-card{min-height:230px}.pet-art{max-width:180px}.scene-studio,.ota-panel{grid-template-columns:1fr}.workspace-panel>.main-grid{grid-column:1}.intro p{max-width:260px}.living-info .ambient-metrics{grid-template-columns:repeat(2,1fr)}}
    @media(max-width:540px){.wrap{padding:0 12px 20px}.topbar{gap:10px}.brand sub{display:none}.intro{display:block;padding:16px 0}.intro h1{font-size:26px;letter-spacing:-.6px}.intro p{max-width:100%;font-size:12px;margin-top:6px}.studio-nav{gap:0}.studio-nav button{flex:1;padding:10px 7px;font-size:12px}.sync-mode{width:100%;margin:0;padding:8px 0 0}.pet-card,.metric-card,.control-card,.link-card,.health-story,.health-settings,.focus-studio,.living-info,.scene-studio,.music-studio,.wifi-studio,.ota-panel{padding:16px}.pet-copy{max-width:55%;padding-top:10px}.pet-copy h2{font-size:25px}.pet-art{min-width:100px;width:37%;right:12px}.history-panel{padding:16px}.history-layout{grid-template-columns:1fr}.history-aside{display:grid;grid-template-columns:repeat(3,1fr)}.scene-selects{grid-template-columns:repeat(2,minmax(0,1fr))}.device-details{gap:10px}.mood-gallery{grid-template-columns:repeat(4,minmax(0,1fr))}.music-status{white-space:normal}}
  </style>
</head>
<body>
  <div class="wrap">
    <header class="topbar">
      <a class="brand" href="/" aria-label="MèoDesk home">
        <span class="brandmark"><svg viewBox="0 0 24 24" fill="none" stroke="#f0b09d" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M3 15V5l5 4 4-3 4 3 5-4v10a9 9 0 0 1-18 0Z"/><path d="M8 15h.01M16 15h.01M10 18q2 2 4 0"/></svg></span>
        <span><span class="brandname">mèo<span>desk.</span></span><sub>personal desk companion</sub></span>
      </a>
      <div class="topright"><div class="headnote">SOFTWARE / 10
LOCAL-FIRST DEVICE</div><div class="connection"><span class="led" id="liveLed"></span><span id="liveText">WAITING FOR PC</span></div></div>
    </header>

    <div class="intro"><h1>Your desk, <em>in sync.</em></h1><p>Your companion, scenes and device settings in one local workspace.</p></div>
    <nav class="studio-nav" role="tablist" aria-label="Studio workspace">
      <button id="tab-overview" role="tab" aria-selected="true" aria-controls="view-overview" data-workspace="overview">Overview</button>
      <button id="tab-studio" role="tab" aria-selected="false" aria-controls="view-studio" data-workspace="studio" tabindex="-1">Personalize</button>
      <button id="tab-focus" role="tab" aria-selected="false" aria-controls="view-focus" data-workspace="focus" tabindex="-1">Focus & music</button>
      <button id="tab-device" role="tab" aria-selected="false" aria-controls="view-device" data-workspace="device" tabindex="-1">Device & updates</button>
      <span class="sync-mode" id="syncMode" role="status">HTTP · connecting</span>
    </nav>

    <main class="main-grid">
      <section class="pet-card" aria-labelledby="petTitle">
        <div class="pet-top"><span class="eyebrow">01 — MEET YOUR CO-PILOT</span><button class="round-icon pet-control" data-action="pet" title="Pet your cat (pairing key needed)" aria-label="Pet your little cat">♡</button></div>
        <div class="pet-copy"><h2 id="petTitle">Stay curious.<span>Stay cozy.</span></h2><p id="moodCopy">Waiting for your Windows PC to say hello. Your little friend is here already.</p></div>
        <div class="pet-art"><img id="petImage" src="/api/cat.bmp?state=rescue" alt="MèoDesk cat mascot"></div>
        <div class="pet-foot"><span class="mood"><span class="dot"></span><span id="moodText">STANDBY MODE</span></span><span class="pet-index">MEOW / 010 · <span id="petEnergy">—</span> ⚡</span></div>
      </section>

      <section class="metric-card" aria-label="PC performance data">
        <div class="metric-top"><span class="eyebrow">02 — LIVE TELEMETRY</span><span class="tag" id="dataTag">NO DATA YET</span></div>
        <div class="cpu-head"><div class="cpu-ring" id="cpuRing"><div class="cpu-inner"><strong id="cpuValue">—</strong><span>CPU LOAD</span></div></div><div class="cpu-side"><div class="metric-label">Windows Host</div><h3 id="pcName">Awaiting link</h3><p id="cpuDescription">Start the Python agent to stream real CPU, GPU and memory data.</p></div></div>
        <div class="bars">
          <div class="barline"><span class="metric-label">Graphics processor</span><strong id="gpuValue">N/A</strong></div><div class="track"><div class="fill gpu-fill" id="gpuBar"></div></div>
          <div class="barline"><span class="metric-label">Memory pressure</span><strong id="ramValue">—</strong></div><div class="track"><div class="fill ram-fill" id="ramBar"></div></div>
          <div class="tiny-row"><span id="gpuDetail">GPU sensor not detected</span><span id="ramDetail">— / — GB</span></div>
        </div>
      </section>
    </main>

    <section class="history-panel" aria-labelledby="historyTitle">
      <div class="history-top">
        <div><div class="eyebrow">03 — THE PERFORMANCE STORY</div><h2 id="historyTitle">A little history of <i>right now.</i></h2><p class="history-sub">Real-time performance, sampled on the ESP32. No cloud, no database.</p></div>
        <div class="history-ranges" role="group" aria-label="History time range"><button class="range-btn" data-range="60">1 MIN</button><button class="range-btn" data-range="180">3 MIN</button><button class="range-btn active" data-range="300">5 MIN</button></div>
      </div>
      <div class="history-layout">
        <div class="graph-wrap"><canvas id="historyCanvas" role="img" aria-label="CPU GPU and RAM percent history"></canvas><div class="chart-tip" id="chartTip" aria-hidden="true"></div></div>
        <aside class="history-aside" aria-label="Peak percentages in selected time range"><div class="peak cpu"><span>CPU / PEAK</span><strong id="cpuPeak">—</strong></div><div class="peak gpu"><span>GPU / PEAK</span><strong id="gpuPeak">—</strong></div><div class="peak ram"><span>RAM / PEAK</span><strong id="ramPeak">—</strong></div></aside>
      </div>
      <div class="history-foot"><div class="legend"><b>CPU</b><b>GPU</b><b>RAM</b></div><span id="historyMeta" aria-live="polite">WAITING FOR HISTORICAL SAMPLES</span></div>
    </section>

    <div class="section-top"><h2>Make it yours.</h2><small>CONTROL / FROM THIS BROWSER</small></div>
    <div class="lower-grid">
      <section class="control-card" aria-labelledby="controlTitle">
        <h3 class="panel-title" id="controlTitle">Screen scenes</h3>
        <div class="nav" id="pages" aria-label="Choose display page">
          <button class="navlink" data-action="page" data-value="12">00 / HOME ★</button>
          <button class="navlink active" data-action="page" data-value="0">01 / PC</button>
          <button class="navlink" data-action="page" data-value="1">02 / Clock</button>
          <button class="navlink" data-action="page" data-value="2">03 / Focus</button>
          <button class="navlink" data-action="page" data-value="3">04 / Wi-Fi</button>
          <button class="navlink" data-action="page" data-value="4">05 / ESP</button>
          <button class="navlink" data-action="page" data-value="5">06 / Music</button>
          <button class="navlink" data-action="page" data-value="6">07 / Print</button>
          <button class="navlink" data-action="page" data-value="7">08 / Audio</button>
          <button class="navlink" data-action="page" data-value="8">09 / Moto</button>
          <button class="navlink" data-action="page" data-value="9">10 / Sleep</button>
          <button class="navlink" data-action="page" data-value="10">11 / Health</button>
          <button class="navlink" data-action="page" data-value="11">12 / Trends</button>
          <button class="navlink" data-action="page" data-value="13">13 / Network</button>
        </div>
        <div class="actions">
          <button class="act primary" data-action="next" title="Next screen">→ &nbsp; Next scene</button>
          <button class="act" data-action="motion" id="motionBtn">◉ &nbsp; Motion ON</button>
          <button class="act" data-action="auto" id="autoBtn">↻ &nbsp; Auto OFF</button>
          <button class="act" data-action="focus" id="focusBtn">Ⅱ &nbsp; Focus</button>
          <button class="act" data-action="focus_reset">↺ &nbsp; Reset Focus</button>
          <button class="act" data-action="focus_skip">⇥ &nbsp; Skip Phase</button>
        </div>
      </section>

      <section class="link-card" aria-labelledby="linkTitle">
        <div><div class="eyebrow">03 — DEVICE LINK</div><div class="device-row"><div class="small-square">◈</div><div><h3 id="linkTitle">ESP32-S3 online</h3><small id="deviceSub">Local Wi-Fi bridge</small></div></div>
          <dl class="device-details"><div><dt>Device IP</dt><dd id="deviceIp">—</dd></div><div><dt>Wi-Fi network</dt><dd id="wifiName">—</dd></div><div><dt>Internal heap</dt><dd id="heap">—</dd></div><div><dt>Cat FPS / target</dt><dd id="catFps">—</dd></div><div><dt>PSRAM free</dt><dd id="psramFree">—</dd></div><div><dt>DMA strips</dt><dd id="dmaState">—</dd></div><div><dt>Max cat render</dt><dd id="renderMax">—</dd></div><div><dt>Skipped frames / 5 s</dt><dd id="frameSkipped">—</dd></div></dl>
          <div class="fps-history" aria-label="Measured cat frame rate over the last five minutes"><div class="fps-history-head"><span>FRAME RATE / LAST 5 MIN</span><strong id="fpsHistorySummary">WAITING FOR SAMPLES</strong></div><canvas id="fpsHistoryCanvas" aria-label="Measured FPS in coral, target FPS in lilac" role="img"></canvas><div class="fps-history-legend"><span>MEASURED FPS</span><span>TARGET FPS</span></div></div>
        </div>
        <div><div class="eyebrow" style="margin-bottom:8px">PAIRING KEY / LOCAL CONTROL</div><div class="pair"><input id="pairKey" type="password" autocomplete="off" spellcheck="false" placeholder="Enter firmware API key" aria-label="Pairing key"><button id="pairSave">SAVE KEY</button></div></div>
      </section>
    </div>

    <div class="section-top"><h2>Wellbeing for machines.</h2><small>03 / SMART HEALTH SYSTEM</small></div>
    <div class="health-grid">
      <section class="health-story" aria-labelledby="healthTitle">
        <span class="eyebrow">SENSOR JOURNAL / LIVE WHEN AVAILABLE</span>
        <h3 id="healthTitle">Every little signal<br>has a story.</h3>
        <div class="health-metrics">
          <div><small>CPU TEMP</small><strong id="cpuTemp">N/A</strong></div>
          <div><small>GPU TEMP</small><strong id="gpuTemp">N/A</strong></div>
          <div><small>PC BATTERY</small><strong id="batteryValue">N/A</strong></div>
        </div>
        <div class="events-heading">LATEST DESK EVENTS / PRIORITIZED</div>
        <ol id="healthEvents" class="events"><li>Waiting for the first health signal...</li></ol>
        <div class="tiny-row"><span id="pcUptime">PC UPTIME / —</span><span>REAL DATA OR N/A</span></div>
      </section>

      <section class="health-settings" aria-labelledby="settingsTitle">
        <div class="eyebrow">TUNE THE INSTINCTS</div>
        <h3 id="settingsTitle" class="settings-title">Your cat, your rules.</h3>
        <p class="settings-note">Adjust alerts on the ESP32 itself. Settings survive a reboot. No cloud account.</p>
        <div class="setting"><label for="cpuLimit">CPU load warning</label><output id="cpuLimitOut">88%</output><input id="cpuLimit" type="range" min="50" max="99" value="88"></div>
        <div class="setting"><label for="gpuLimit">GPU load warning</label><output id="gpuLimitOut">92%</output><input id="gpuLimit" type="range" min="50" max="99" value="92"></div>
        <div class="setting"><label for="tempLimit">Temperature warning</label><output id="tempLimitOut">80°C</output><input id="tempLimit" type="range" min="55" max="110" value="80"></div>
        <div class="setting"><label for="batteryLimit">Low battery warning</label><output id="batteryLimitOut">20%</output><input id="batteryLimit" type="range" min="5" max="50" value="20"></div>
        <label class="quiet-row"><input id="quietMode" type="checkbox"> Quiet desk mode — show pause cat, stop bobbing (critical warnings remain)</label>
        <button id="settingsSave" class="settings-save">SAVE TO ESP32 MEMORY</button>
      </section>
    </div>

    <div class="section-top"><h2>Find your rhythm.</h2><small>04 / FOCUS + AMBIENT MODE</small></div>
    <div class="living-grid">
      <section class="focus-studio" aria-labelledby="focusTitle">
        <span class="eyebrow">DEEP WORK / YOUR TINY TIMEKEEPER</span>
        <h2 id="focusTitle" class="focus-clock" aria-live="off">25:00</h2>
        <div class="focus-sub"><span id="focusPhase">READY TO FOCUS</span> · <span id="focusSessions">0 SESSIONS</span></div>
        <div class="focus-track"><div id="focusBar"></div></div>
        <div class="actions"><button class="act primary" id="focusHero" data-focus="toggle">▶ Start / pause</button><button class="act" data-focus="reset">↺ Reset</button><button class="act" data-focus="skip">⇥ Skip</button></div>
      </section>
      <section class="living-info" aria-labelledby="ambientTitle">
        <span class="eyebrow">SYSTEM / REAL-TIME OR N/A</span>
        <h3 id="ambientTitle">An entire desk, in sync.</h3>
        <div class="ambient-metrics"><div><small>NETWORK DOWNLOAD</small><strong id="netDown">N/A</strong></div><div><small>NETWORK UPLOAD</small><strong id="netUp">N/A</strong></div><div><small>GPU VRAM USED</small><strong id="vramUsed">N/A</strong></div><div><small>GPU VRAM TOTAL</small><strong id="vramTotal">N/A</strong></div></div>
        <span class="eyebrow">TFT APPEARANCE / SAVED ON DEVICE</span>
        <div class="theme-buttons" role="group" aria-label="TFT color theme"><button data-action="theme" data-value="0">☀ COZY DAY</button><button data-action="theme" data-value="1">☾ MIDNIGHT</button><button data-action="theme" data-value="2">◴ AUTO</button></div>
        <p class="wifi-note" style="margin-top:11px">AUTO follows the device local time when NTP is available. Manual modes always work.</p>
        <span class="eyebrow">CAT ANIMATION / FRAME PACING · SAVED ON DEVICE</span>
        <div class="theme-buttons" role="group" aria-label="Cat animation target FPS"><button data-action="fps" data-value="0">STABLE 20</button><button data-action="fps" data-value="1">BALANCED 25</button><button data-action="fps" data-value="2">SMOOTH 30</button><button data-action="fps" data-value="3">◈ AUTO 20–25</button></div>
        <p class="wifi-note" style="margin-top:11px">AUTO adapts between 20 and 25 FPS after sustained load changes. It never switches to 30 FPS automatically. See actual FPS and missed frames in Device Link.</p>
        <span class="eyebrow">LIVING MOTION / ANIMATION PERSONALITY · SAVED ON DEVICE</span>
        <div class="theme-buttons" role="group" aria-label="Cat motion personality"><button data-action="motion_style" data-value="0">☾ CALM</button><button data-action="motion_style" data-value="1">✦ NATURAL</button><button data-action="motion_style" data-value="2">♫ PLAYFUL</button></div>
        <div class="motion-signal" aria-live="off"><span class="motion-state" id="motionState">COZY / WAITING</span><span id="motionCel">CEL 0 / 3</span><span id="motionGuard">FPS GUARD / NORMAL</span><span id="motionSignal">REAL PC / N-A</span></div>
        <p class="wifi-note" style="margin-top:9px">Natural blinks, dance timing, quiet Focus and alert cues follow live Windows data. FPS Guard drops extra effects when rendering is overloaded. Frame 2 looks like a real blink only with authored closed-eye art.</p>
      </section>
    </div>

    <section class="scene-studio" aria-labelledby="sceneTitle">
      <div><div class="eyebrow">05 — YOUR FOUR FAVORITE SCENES</div><h3 id="sceneTitle">Design your little loop.</h3><p>Pick any four different screens. AUTO rotates only these favorites; all fourteen legacy pages remain available in the full scene menu.</p><span class="scene-tip">SAVED TO DEVICE MEMORY · PAIRING KEY REQUIRED</span></div>
      <div class="scene-selects"><label>01 / FAVORITE<select id="sceneSlot0" aria-label="First favorite scene"></select></label><label>02 / FAVORITE<select id="sceneSlot1" aria-label="Second favorite scene"></select></label><label>03 / FAVORITE<select id="sceneSlot2" aria-label="Third favorite scene"></select></label><label>04 / FAVORITE<select id="sceneSlot3" aria-label="Fourth favorite scene"></select></label></div>
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
        <div class="eyebrow">06 — WINDOWS NOW PLAYING / LIVE</div>
        <img class="album-image" id="albumCoverWeb" alt="Album cover from Windows Media" hidden>
        <h2 id="musicTitle">Nothing playing</h2>
        <div class="artist" id="musicArtist">Waiting for a Windows media session.</div>
        <div class="track"><div id="musicBar"></div></div>
        <div class="music-time" id="musicTime">— / — · NOT AVAILABLE</div>
        <div class="music-actions">
          <button data-media="media_previous" title="Previous track">⏮</button>
          <button class="main" data-media="media_play_pause" title="Play / pause">⏯</button>
          <button data-media="media_next" title="Next track">⏭</button>
          <span class="music-status" id="musicCommand">WINDOWS AGENT REQUIRED</span>
        </div>
      </section>

      <section class="wifi-studio" aria-labelledby="wifiTitle">
        <div class="eyebrow">07 — NETWORK / DEVICE SETTINGS</div>
        <h2 id="wifiTitle">Find your way home.</h2>
        <p class="wifi-note">Thiết lập Wi-Fi 2.4 GHz trực tiếp trên ESP32. Hotspot dự phòng vẫn bật để sửa nếu nhập sai mạng.</p>
        <label for="newSsid">WI-FI SSID</label>
        <input id="newSsid" type="text" maxlength="32" autocomplete="off" placeholder="Tên Wi-Fi 2.4 GHz">
        <label for="newPassword">WI-FI PASSWORD</label>
        <input id="newPassword" type="password" autocomplete="new-password" maxlength="63" placeholder="Để trống nếu mạng mở">
        <button id="wifiSave">SAVE WI-FI TO DEVICE</button>
        <button id="wifiScan" type="button" style="background:transparent;color:#5a5365;border:1px solid #cdbdd3">SCAN NEARBY NETWORKS (ESP32)</button>
        <p class="wifi-note" id="wifiScanResult">Fallback AP vẫn hoạt động tại 192.168.4.1.</p>
      </section>
    </div>

    <section class="ota-panel" aria-label="Firmware updates over local Wi-Fi">
      <div><span class="eyebrow">08 — OTA GUARDIAN</span><h3>Give your cat<br>a fresh little brain.</h3><p>Upload to the inactive flash slot. After restart, MèoDesk will check the running slot and device health before claiming success.</p><div class="ota-checks"><div><small>RUNNING FIRMWARE</small><strong id="otaVersion">CHECKING…</strong></div><div><small>INACTIVE SLOT</small><strong id="otaSlot">CHECKING…</strong></div><div><small>CAPABILITY</small><strong id="otaReady">CHECKING…</strong></div><div><small>BOOTLOADER ROLLBACK</small><strong id="otaRollback">CHECKING…</strong></div><div><small>STARTUP HEALTH</small><strong id="otaBootHealth">CHECKING…</strong></div><div><small>UPTIME</small><strong id="otaUptime">CHECKING…</strong></div></div></div>
      <div class="ota-upload"><label for="otaFile">COMPILED FIRMWARE · .BIN ONLY</label><input id="otaFile" type="file" accept=".bin,application/octet-stream"><button id="otaStart" disabled>↑ UPLOAD & VERIFY FIRMWARE</button><div class="ota-progress"><div id="otaProgress"></div></div><p class="ota-result" id="otaResult" role="status">Device status pending.</p><p class="ota-warning">Local developer OTA only · Pairing key required · Keep USB power connected. Do not expose this HTTP endpoint to the Internet.</p></div>
    </section>

    <section class="ota-panel catpack-panel" aria-label="Animated cat sprite packs over local Wi-Fi">
      <div><span class="eyebrow">09 — MOTION PACK STUDIO</span><h3>Little movements.<br>Big personality.</h3><p>16 cat moods · up to 3 hand-drawn animation frames each. Keep your existing 648 KiB collections, or install a 1,944 KiB Motion Pack. Both stay in PSRAM and use the original internal-RAM DMA strips; interrupted uploads leave the previous bank intact.</p><div class="ota-checks"><div><small>ACTIVE COLLECTION</small><strong id="catPackSource">CHECKING…</strong></div><div><small>BACKUP GENERATION</small><strong id="catPackGen">CHECKING…</strong></div><div><small>PSRAM / FLASH READY</small><strong id="catPackReady">CHECKING…</strong></div><div><small>ANIMATION</small><strong id="catPackMotion">CHECKING…</strong></div></div></div>
      <div class="ota-upload"><label for="catPackFile">STATIC 648 KiB / MOTION 1,944 KiB · .MEOCAT</label><input id="catPackFile" type="file" accept=".meocat,application/octet-stream"><button id="catPackStart" disabled>✦ INSTALL CAT MOTION PACK</button><div class="ota-progress"><div id="catPackProgress"></div></div><p class="ota-result" id="catPackResult" role="status">Checking your cat collection…</p><p class="ota-warning">Use make_catpack.py --motion-demo for a ready-to-test animated collection, or --motion with your own 48 PNG frames. Reboots once; requires private LAN + pairing key.</p></div>
    </section>

    <footer class="status-strip"><span class="signature">built for little joys.</span><span>STATUS <span id="footerStatus">WAITING FOR SIGNAL</span> • NO CLOUD REQUIRED</span><span>MEODESK LAB / 2026 • <span id="firmwareBuild">DEV</span></span></footer>
  </div>
  <div id="toast" role="status" aria-live="polite"></div>
  <script>
    (function(){
      'use strict';
      const $=(id)=>document.getElementById(id);
      const workspaceGroups={overview:['.main-grid','.history-panel','.health-story'],studio:['.control-card','.living-info','.scene-studio','.mood-gallery','.catpack-panel'],focus:['.focus-studio','.music-studio'],device:['.link-card','.health-settings','.wifi-studio','.ota-panel:not(.catpack-panel)']};
      for(const [name,selectors] of Object.entries(workspaceGroups)){
        const panel=document.createElement('div');panel.id='view-'+name;panel.className='workspace-panel';panel.setAttribute('role','tabpanel');panel.setAttribute('aria-labelledby','tab-'+name);panel.hidden=name!=='overview';
        document.querySelector('.wrap').insertBefore(panel,document.querySelector('.status-strip'));
        selectors.forEach(selector=>document.querySelectorAll(selector).forEach(node=>panel.append(node)));
      }
      document.querySelectorAll('.section-top,.mood-intro,.lower-grid,.health-grid,.living-grid,.feature-grid,[aria-label="Quick scene shortcuts"]').forEach(node=>node.remove());
      document.querySelector('.health-story').append(document.querySelector('.ambient-metrics'));
      const motionHelp=document.createElement('details');motionHelp.className='motion-help';
      const helpSummary=document.createElement('summary');helpSummary.textContent='About themes, motion and FPS';motionHelp.append(helpSummary);
      document.querySelectorAll('.living-info .wifi-note').forEach(note=>motionHelp.append(note));document.querySelector('.living-info').append(motionHelp);
      document.querySelectorAll('.eyebrow').forEach(label=>{if(label.closest('.pet-top')||(label.nextElementSibling&&/^H[1-6]$/.test(label.nextElementSibling.tagName)))label.remove();});
      const labPanel=document.createElement('section');labPanel.className='lab-panel';labPanel.setAttribute('aria-labelledby','labTitle');
      labPanel.innerHTML='<div class="lab-head"><h3 id="labTitle">Performance Lab</h3><button id="labToggle" class="act" data-action="perf_trace" data-value="1" disabled>Enable tracing</button></div><p id="labStatus">Waiting for a compatible firmware.</p><dl class="lab-metrics"><div><dt>Strip CPU / cat frame</dt><dd id="labCpu">N/A</dd></div><div><dt>DMA submit / strip</dt><dd id="labSubmit">N/A</dd></div><div><dt>DMA wait / strip</dt><dd id="labWait">N/A</dd></div><div><dt>Cat wall time / frame</dt><dd id="labWall">N/A</dd></div><div><dt>Card wall / call</dt><dd id="labCard">N/A</dd></div><div><dt>Bridge max / loop</dt><dd id="labBridge">N/A</dd></div><div><dt>Tracked pixel payload</dt><dd id="labBytes">N/A</dd></div><div><dt>Window duration</dt><dd id="labWindow">N/A</dd></div></dl><p>DMA wait measures CPU stalls, not total SPI bus activity. Payload counts cat strips, buffered cards and album tiles only; direct text, commands and protocol overhead are excluded. Tracing is optional and resets when toggled.</p>';
      $('view-device').prepend(labPanel);
      const configPanel=document.createElement('section');configPanel.className='lab-panel';
      configPanel.innerHTML='<h3>Configuration backup</h3><p>Export scenes, theme, Focus and alert preferences. Wi-Fi passwords and pairing keys are excluded.</p><div class="actions"><button class="act" id="configExport">Export configuration</button><input id="configFile" type="file" accept=".json,application/json" aria-label="Configuration JSON"><button class="act" id="configRestore">Restore selected file</button></div><p id="configResult" role="status">Restore validates the whole file before saving. A changed Focus duration resets its current session.</p><details><summary>Firmware update history</summary><button class="act" id="otaHistoryRefresh">Read device history</button><ol id="otaHistoryList"><li>No history loaded.</li></ol></details>';
      $('view-device').append(configPanel);
      const focusConfig=document.createElement('form');focusConfig.className='focus-config';
      focusConfig.innerHTML='<label>Work · min<input id="focusWorkMin" type="number" min="1" max="180" value="25" required></label><label>Break · min<input id="focusBreakMin" type="number" min="1" max="60" value="5" required></label><label>Session goal<input id="focusGoal" type="number" min="1" max="24" value="4" required></label><button class="act" type="submit">Save Focus plan</button>';
      document.querySelector('.focus-studio').append(focusConfig);
      let focusConfigDirty=false;
      focusConfig.addEventListener('input',()=>focusConfigDirty=true);
      focusConfig.addEventListener('submit',async event=>{event.preventDefault();try{await keyedJson('/api/v1/focus/config',{work_min:Number($('focusWorkMin').value),break_min:Number($('focusBreakMin').value),goal:Number($('focusGoal').value)});focusConfigDirty=false;toast('Focus plan saved. Duration changes reset the current session.');streamSeenAt=0;poll();}catch(error){toast(error.message);}});
      async function keyedJson(path,payload){const key=pair.value.trim();if(!key)throw Error('Enter the pairing key in Device & updates.');const response=await fetch(path,{method:payload?'POST':'GET',headers:{'X-MEO-KEY':key,...(payload?{'Content-Type':'application/json'}:{})},body:payload?JSON.stringify(payload):undefined,cache:'no-store'});const result=await response.json();if(!response.ok||result.ok===false)throw Error(result.error||'Device request failed');return result;}
      $('configExport').addEventListener('click',async()=>{try{const data=await keyedJson('/api/v1/config');const link=document.createElement('a'),url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));link.href=url;link.download='meodesk-profile.json';link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);$('configResult').textContent='Configuration exported. No network credentials included.';}catch(error){$('configResult').textContent=error.message;}});
      $('configRestore').addEventListener('click',async()=>{try{const file=$('configFile').files[0];if(!file||file.size>8192)throw Error('Choose a configuration JSON file up to 8 KiB.');const data=JSON.parse(await file.text());if(data.schema!==1||data.board!=='meodesk-esp32s3-n16r8'||!data.profile)throw Error('Incompatible configuration file.');await keyedJson('/api/v1/config',data);$('configResult').textContent='Configuration restored and saved on ESP32.';focusConfigDirty=false;streamSeenAt=0;poll();}catch(error){$('configResult').textContent=error.message;}});
      $('otaHistoryRefresh').addEventListener('click',async()=>{const root=$('otaHistoryList');root.replaceChildren();try{const data=await keyedJson('/api/v1/ota/history');for(const update of (Array.isArray(data.updates)?data.updates:[]).slice().reverse()){const row=document.createElement('li');row.textContent=[update.phase,update.version_before,update.version_after,update.target_slot].filter(Boolean).join(' · ');root.append(row);}if(!root.children.length){const row=document.createElement('li');row.textContent='No updates recorded by this firmware yet.';root.append(row);}}catch(error){const row=document.createElement('li');row.textContent=error.message;root.append(row);}});
      function chooseWorkspace(name){
        if(!workspaceGroups[name])name='overview';
        document.querySelectorAll('[data-workspace]').forEach(button=>{const active=button.dataset.workspace===name;button.setAttribute('aria-selected',String(active));button.tabIndex=active?0:-1;});
        Object.keys(workspaceGroups).forEach(key=>$('view-'+key).hidden=key!==name);
        try{sessionStorage.setItem('meodesk-workspace',name);}catch(_){}
        requestAnimationFrame(()=>{drawHistory();drawFpsHistory();});
      }
      document.querySelectorAll('[data-workspace]').forEach(button=>{
        button.addEventListener('click',()=>chooseWorkspace(button.dataset.workspace));
        button.addEventListener('keydown',event=>{const tabs=Array.from(document.querySelectorAll('[data-workspace]')),index=tabs.indexOf(button);let next;if(event.key==='ArrowRight')next=(index+1)%tabs.length;if(event.key==='ArrowLeft')next=(index+tabs.length-1)%tabs.length;if(event.key==='Home')next=0;if(event.key==='End')next=tabs.length-1;if(next!==undefined){event.preventDefault();chooseWorkspace(tabs[next].dataset.workspace);tabs[next].focus();}});
      });
      let stateSocket=null,streamSeenAt=0,stateBoot='',stateRevision=-1,pollInFlight=false;
      function receiveState(data){
        if(data.device!=='MEO DESK'&&data.device!=='MEO DESK / MOCK')return;
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
        socket.onmessage=event=>{try{const data=JSON.parse(event.data);if(data.type!=='state'||!['MEO DESK','MEO DESK / MOCK'].includes(data.device))return;streamSeenAt=performance.now();$('syncMode').textContent='WebSocket · live';$('syncMode').dataset.live='true';receiveState(data);}catch(_){} };
        socket.onerror=()=>socket.close();
        socket.onclose=()=>{if(stateSocket!==socket)return;stateSocket=null;streamSeenAt=0;$('syncMode').textContent='HTTP · reconnecting';$('syncMode').dataset.live='false';setTimeout(()=>connectState(port),2000);};
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
            else if(otaDevice.phase==='idle')$('otaResult').textContent='Ready to verify a local N16R8 firmware image.';
          }
          updateOtaButton();
        }catch(e){otaDevice=null;$('otaReady').textContent='OFFLINE';updateOtaButton();}
      }
      async function verifyOtaBoot(targetSlot){
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
          if(data.running_slot===targetSlot){
            sawNewSlot=true;
            if(data.boot_health==='failed')throw Error('New firmware failed device startup health check');
            if(data.boot_health==='passed'&&data.ready){otaDevice=data;return data;}
            $('otaResult').textContent='New slot active. Waiting for startup self-check (~25 s)…';
          }else if(sawNewSlot){throw Error('Device returned to previous firmware slot');}
          else{$('otaResult').textContent='Waiting for ESP32 to boot into '+targetSlot+'…';}
        }
        throw Error('Boot not confirmed within 85 s. Check USB Serial before retrying.');
      }
      async function uploadOta(){
        const file=$('otaFile').files[0],key=pair.value.trim();
        if(!file||!otaDevice||!otaDevice.ready)return;
        if(!key){toast('Enter the device pairing key first');pair.focus();return;}
        if(!file.name.toLowerCase().endsWith('.bin')||file.size<4096||file.size>otaDevice.inactive_slot_bytes){toast('Choose a .bin within the inactive slot size');return;}
        if(otaDevice.board!=='meodesk-esp32s3-n16r8'){toast('Board type mismatch: OTA blocked');return;}
        if(!confirm('Update MèoDesk with '+file.name+' ('+(file.size/1048576).toFixed(2)+' MiB)? Keep power connected.'))return;
        const targetSlot=otaDevice.inactive_slot;
        if(targetSlot!=='app0'&&targetSlot!=='app1'){toast('Invalid inactive OTA slot');return;}
        otaUploading=true;updateOtaButton();$('otaProgress').style.width='0%';
        let accepted=false;
        try{
          $('otaResult').textContent='Calculating SHA-256…';
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
          $('otaResult').textContent='SHA-256 verified. Waiting for device restart…';
          await verifyOtaBoot(targetSlot);
          $('otaResult').textContent='✓ UPDATE CONFIRMED: new firmware running, startup health PASS.';
          toast('MèoDesk successfully booted the new firmware');
        }catch(e){$('otaResult').textContent=(accepted?'BOOT NOT CONFIRMED: ':'UPDATE FAILED: ')+e.message;toast(e.message);}
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
        if(!key){toast('Enter your device pairing key');return;}
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
          toast('New cat collection installed!');
        }catch(e){$('catPackResult').textContent=(accepted?'NOT CONFIRMED: ':'UPLOAD FAILED: ')+e.message;toast(e.message);}
        finally{catPackResultPinnedUntil=Date.now()+30000;catPackUploading=false;pollCatPackStatus();updateCatPackButton();}
      }
      let lastEmotion='',toastTimer=null,settingsDirty=false;
      const pair=$('pairKey');
      try{pair.value=sessionStorage.getItem('meodesk-key')||'';}catch(e){}
      function toast(msg){let node=$('toast');node.textContent=msg;node.classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>node.classList.remove('show'),2900);}
      function percent(n){return Number.isFinite(n)?Math.round(n)+'%':'—';}
      function bar(id,n){$(id).style.width=(Number.isFinite(n)?Math.max(0,Math.min(100,n)):0)+'%';}
      const mood={rescue:['SEARCHING FOR YOU','Your Windows PC has gone quiet. Check the agent and the Wi-Fi connection.'],sleep:['CHILL MODE','All quiet. Low CPU and GPU load; the cat can finally take a little nap.'],healthy:['EVERYTHING IS PAWSOME','Your PC looks healthy. No warning thresholds have been reached.'],stressed:['BUSY BEANS','CPU or GPU load is above your chosen threshold. Take a peek at the workload.'],hot:['FEELING TOASTY','A temperature sensor reached your alert threshold. Check cooling and airflow.'],low_battery:['BATTERY SOS','Battery is running low on an unplugged laptop. A charger would help.'],paused:['DO NOT DISTURB','Focus or quiet mode is active. Critical alerts still take priority.'],power:['FULL OF ENERGY','Your Windows device is connected to external power.'],hotspot:['HOTSPOT HELPER','ESP32 is using its fallback Wi-Fi access point.'],music:['MUSIC TIME','Windows says something is playing. Enjoy your tiny listening buddy.'],demo:['DEMO / SIMULATED','These numbers are synthetic test data. Start the real agent for actual PC measurements.']};
      const previews=['paused','low_battery','rescue','power','sleep','hot','healthy','hotspot'];
      const personaArt={"looking for PC":"rescue",cozy:"healthy",sleepy:"sleep",dancing:"music","deep focus":"paused",busy:"stressed","too warm":"hot","need charge":"low_battery",celebrating:"healthy"};
      const scenes=[[12,'HOME'],[0,'PC MONITOR'],[1,'CLOCK'],[2,'FOCUS'],[3,'WI-FI'],[4,'DEVICE'],[5,'NOW PLAYING'],[6,'PRINT'],[7,'AUDIO'],[8,'MOTO'],[9,'SLEEP'],[10,'HEALTH'],[11,'TRENDS'],[13,'PC NETWORK'],[14,'MY WIDGETS']];
      const defaultScenes=[12,0,5,2];
      scenes.forEach(([id,name])=>{for(let i=0;i<4;i++){const option=document.createElement('option');option.value=String(id);option.textContent=name;$('sceneSlot'+i).append(option);}});
      defaultScenes.forEach((id,i)=>{$('sceneSlot'+i).value=String(id);});
      let coverBlobUrl='',coverShownId='',coverPendingId='';
      const settingIds=['cpuLimit','gpuLimit','tempLimit','batteryLimit'];
      const chartState={history:null,range:300,hover:-1};
      const chartColors={cpu:'#ed8576',gpu:'#a58bdc',ram:'#57b69a'};
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
        $('historyMeta').textContent=(data.mode==='demo'?'SIMULATED / ':data.mode==='offline'?'OFFLINE / ':'LIVE / ')+all.length+' SAMPLES · 1 HZ · LAST '+(chartState.range/60)+' MIN';drawHistory();}
      function moveHistoryHover(event){const rect=$('historyCanvas').getBoundingClientRect(),n=rangeSeries('cpu').length;if(n<2)return;const left=38,width=rect.width-left-15,i=Math.max(0,Math.min(n-1,Math.round((event.clientX-rect.left-left)*((n-1)/Math.max(1,width)))));chartState.hover=i;const values=['cpu','gpu','ram'].map(name=>{const v=rangeSeries(name)[i];return name.toUpperCase()+' '+(typeof v==='number'?v+'%':'N/A');});const tip=$('chartTip');tip.textContent=(n-1-i)+'s ago · '+values.join(' / ');tip.style.left=Math.max(80,Math.min(rect.width-80,event.clientX-rect.left))+'px';tip.style.top='53px';tip.classList.add('visible');drawHistory();}
      async function pollHistory(){try{const res=await fetch('/api/history',{cache:'no-store'});if(!res.ok)throw new Error('History offline');renderHistory(await res.json());}catch(e){$('historyMeta').textContent='HISTORY TEMPORARILY UNAVAILABLE';}}
      function renderSettings(settings){if(!settings||settingsDirty)return;for(const id of settingIds){const key=id.replace(/[A-Z]/g,m=>'_'+m.toLowerCase());if(Number.isFinite(settings[key]))$(id).value=settings[key];} $('quietMode').checked=!!settings.quiet;showSettingValues();}
      function showSettingValues(){for(const id of settingIds){$(id+'Out').textContent=$(id).value+(id==='tempLimit'?'°C':'%');}}
      function renderEvents(entries){const root=$('healthEvents');root.replaceChildren();if(!Array.isArray(entries)||entries.length===0){const li=document.createElement('li');li.textContent='Waiting for the first desk event...';root.append(li);return;}for(const event of entries.slice(0,5)){const li=document.createElement('li'),span=document.createElement('span'),ago=document.createElement('time'),level=document.createElement('b');const pri=Number(event.priority)||0;li.className='priority-'+pri;level.className='event-label';level.textContent=['INFO','NOTE','WARN','ALERT'][Math.min(3,Math.max(0,pri))];span.textContent=event.message||'Status change';ago.textContent=(event.age_s||0)+'s ago';li.append(level,span,ago);root.append(li);}}
      function displayMetric(value,suffix){return Number.isFinite(value)?Number(value).toFixed(1)+suffix:'N/A';}
      let focusAnchor=null;
      function updateFocus(data){focusAnchor={...data,at:performance.now()};paintFocus(data,0);}
      function paintFocus(data,elapsed){const seconds=Math.max(0,(Number(data.focus_remaining_s)||0)-(data.focus_running?Math.floor(elapsed/1000):0)),work=data.focus_phase!=='break',duration=Number(data.focus_duration_s)||(work?1500:300);
        $('focusTitle').textContent=String(Math.floor(seconds/60)).padStart(2,'0')+':'+String(seconds%60).padStart(2,'0');
        $('focusPhase').textContent=(work?'FOCUS':'BREAK')+' / '+(data.focus_running?'IN PROGRESS':'PAUSED / READY');
        $('focusSessions').textContent=(Number(data.focus_sessions)||0)+' SESSIONS';
        $('focusBar').style.width=(Math.round(100*(duration-Math.min(duration,seconds))/duration))+'%';
        $('focusHero').textContent=data.focus_running?'Ⅱ Pause focus':'▶ Start / resume';}
      function update(data){
        $('firmwareBuild').textContent=data.version||'unknown';
        const labSupported=Array.isArray(data.capabilities)&&data.capabilities.includes('performance_lab_v1');
        $('labToggle').disabled=!labSupported;$('labToggle').dataset.value=data.lab_enabled?'0':'1';$('labToggle').textContent=data.lab_enabled?'Disable tracing':'Enable tracing';
        $('labStatus').textContent=!labSupported?'Install a Performance Lab build to measure this device.':!data.lab_enabled?'Tracing is off. Enable for a comparison run; settings are not saved to Flash.':data.lab_valid?'Device-reported measurement window. Compare matched theme / style / pack sessions.':'Collecting the first complete window (at least 5 seconds). DEMO previews have no hardware measurements.';
        const labMetric=(id,key,unit)=>{$(id).textContent=data.lab_valid&&Number.isFinite(data[key])?data[key].toFixed(2)+unit:'N/A';};
        labMetric('labCpu','lab_cpu_frame_ms',' ms');labMetric('labSubmit','lab_dma_submit_ms',' ms');labMetric('labWait','lab_dma_wait_ms',' ms');labMetric('labWall','lab_cat_wall_ms',' ms');labMetric('labCard','lab_card_wall_ms',' ms');labMetric('labBridge','lab_bridge_max_ms',' ms');
        $('labBytes').textContent=data.lab_valid&&Number.isFinite(data.lab_payload_bytes_s)?(data.lab_payload_bytes_s/1024).toFixed(1)+' KiB/s':'N/A';$('labWindow').textContent=data.lab_valid?(data.lab_window_ms/1000).toFixed(2)+' s':'N/A';
        const ready=!!data.pc_connected;
        if(!focusConfigDirty){$('focusWorkMin').value=data.focus_work_min||25;$('focusBreakMin').value=data.focus_break_min||5;$('focusGoal').value=data.focus_goal||4;}
        const persona=data.cat_persona||'';
        const emo=data.preview_active?(data.emotion||'rescue'):(personaArt[persona]||data.emotion||'rescue');
        $('liveLed').classList.toggle('live',ready);
        $('liveText').textContent=ready?(data.demo?'DEMO STREAM':'PC STREAM LIVE'):'WAITING FOR PC';
        $('dataTag').textContent=ready?(data.demo?'SIMULATED':data.preview_active?'MASCOT PREVIEW':'REAL SENSOR DATA'):'NO DATA YET';
        $('pcName').textContent=ready?(data.pc_name||'Windows PC'):'Awaiting link';
        $('cpuValue').textContent=ready?percent(data.cpu_percent):'—';
        $('gpuValue').textContent=ready&&data.gpu_percent!=null?percent(data.gpu_percent):'N/A';
        $('ramValue').textContent=ready?percent(data.ram_percent):'—';
        $('cpuDescription').textContent=ready?(data.demo?'Demo mode is active. Data is simulated.':'Measurements update every second from your Windows machine.'):'Start the Python agent to stream real CPU, GPU and memory data.';
        const cpu=ready&&Number.isFinite(data.cpu_percent)?Math.min(100,Math.max(0,data.cpu_percent)):0;
        $('cpuRing').style.background=`conic-gradient(#f08271 0% ${cpu}%, #4a4b58 ${cpu}% 100%)`;
        bar('gpuBar',ready?data.gpu_percent:null);bar('ramBar',ready?data.ram_percent:null);
        $('gpuDetail').textContent=ready?(data.gpu_temp_c!=null?'GPU '+Math.round(data.gpu_temp_c)+'°C':(data.gpu_name||'GPU unavailable')):'GPU sensor not detected';
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
        $('deviceSub').textContent=data.wifi_connected?'Connected to router':'Fallback AP active';
        $('cpuTemp').textContent=ready&&data.cpu_temp_c!=null?Math.round(data.cpu_temp_c)+'°C':'N/A';
        $('gpuTemp').textContent=ready&&data.gpu_temp_c!=null?Math.round(data.gpu_temp_c)+'°C':'N/A';
        $('batteryValue').textContent=ready&&data.battery_percent!=null?Math.round(data.battery_percent)+'%':'N/A';
        $('pcUptime').textContent=ready&&data.pc_uptime_s!=null?'PC UPTIME / '+Math.floor(data.pc_uptime_s/3600)+'H '+Math.floor(data.pc_uptime_s%3600/60)+'M':'PC UPTIME / —';
        renderEvents(data.events);renderSettings(data.settings);
        $('footerStatus').textContent=ready?'STREAMING '+(data.demo?'DEMO':'LIVE'):'STANDBY';
        $('motionBtn').textContent=(data.motion?'◉  Motion ON':'○  Motion OFF');
        $('autoBtn').textContent=(data.auto_pages?'↻  Auto ON':'↻  Auto OFF');
        $('focusBtn').textContent=(data.focus_running?'Ⅱ  Focus':'▶  Resume');
        document.querySelectorAll('[data-media]').forEach(b=>b.disabled=!data.pc_connected||!data.media_available);
        if(data.command_pending)$('musicCommand').textContent='WAITING FOR WINDOWS ACK...';
        document.querySelectorAll('[data-action="page"]').forEach(b=>{const active=Number(b.dataset.value)===data.page;b.classList.toggle('active',active);b.setAttribute('aria-current',active?'true':'false');});
        if(emo!==lastEmotion){lastEmotion=emo;$('petImage').src='/api/cat.bmp?state='+encodeURIComponent(emo);}
        $('moodText').textContent=(data.preview_active?'PREVIEW / ':'')+(persona&&!data.preview_active?persona.toUpperCase():(mood[emo]?mood[emo][0]:'STANDBY'));
        const active=Array.isArray(data.events)?data.events.find(e=>e.code===data.active_event):null;
        $('moodCopy').textContent=active&&active.message?active.message:(mood[emo]?mood[emo][1]:mood.rescue[1]);
        document.querySelectorAll('[data-preview]').forEach(b=>{b.classList.toggle('active',!!data.preview_active&&previews[Number(b.dataset.preview)]===emo);});
      }
      async function poll(){if(pollInFlight||streamFresh())return;pollInFlight=true;const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),3000);try{const res=await fetch('/api/status',{cache:'no-store',signal:controller.signal});if(!res.ok)throw new Error('HTTP '+res.status);const data=await res.json();if(!streamFresh())receiveState(data);if(!streamFresh()){$('syncMode').textContent='HTTP · polling';$('syncMode').dataset.live='false';}}catch(e){if(!streamFresh()){$('liveLed').classList.remove('live');$('liveText').textContent='DEVICE OFFLINE';$('footerStatus').textContent='RECONNECTING';$('syncMode').textContent='Offline · reconnecting';}}finally{clearTimeout(timeout);pollInFlight=false;}}
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
        }catch(e){$('fpsHistorySummary').textContent='HISTORY OFFLINE';drawFpsHistory();}
      }
      async function control(action,value=0){const key=pair.value.trim();if(!key){toast('Enter the pairing key to control your screen');pair.focus();return;}try{const res=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({action,value})});const body=await res.json();if(!res.ok||!body.ok)throw new Error(body.error||'Request failed');toast(action==='preview'?'Cat preview active for eight seconds':action==='page'?'Scene updated':'Screen command sent');poll();}catch(e){toast('Could not control screen: '+e.message);}}
      async function mediaControl(action){
        const key=pair.value.trim();if(!key){toast('Pairing key is required for media control');return;}
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
          $('musicCommand').textContent='TIMEOUT / TRY AGAIN';toast('Windows agent did not acknowledge the command');
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
      function renderMedia(data){
          $('musicTitle').textContent=data.available?(data.title||'Untitled track'):'Nothing playing';
          $('musicArtist').textContent=data.available?(data.artist||'Unknown artist'):'No active Windows media session';
          const position=data.position_ms||0,duration=data.duration_ms||0;
          $('musicBar').style.width=duration?Math.min(100,Math.round(position*100/duration))+'%':'0%';
          const fmt=n=>Math.floor(n/60000)+':'+String(Math.floor(n/1000)%60).padStart(2,'0');
          $('musicTime').textContent=data.available?(data.playing?'▶ PLAYING · ':'Ⅱ PAUSED · ')+(duration?fmt(position)+' / '+fmt(duration):'TIME N/A'):'NOT AVAILABLE / N-A';
          renderArtwork(data);
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
        if(!key||!ssid){toast('Enter pairing key and Wi-Fi SSID');return;}
        if(!confirm('Change ESP32 Wi-Fi to "'+ssid+'"? The fallback AP stays available.'))return;
        try{
          const res=await fetch('/api/v1/wifi/config',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify({ssid,password})});
          const data=await res.json();if(!res.ok||!data.ok)throw new Error(data.error||'Unable to save');
          $('newPassword').value='';toast('Saved. ESP32 reconnecting; fallback AP remains active.');
        }catch(error){toast('Wi-Fi: '+error.message);}
      }
      async function saveSettings(){const key=pair.value.trim();if(!key){toast('Enter your pairing key first');pair.focus();return;}const body={cpu_limit:Number($('cpuLimit').value),gpu_limit:Number($('gpuLimit').value),temp_limit:Number($('tempLimit').value),battery_limit:Number($('batteryLimit').value),quiet:$('quietMode').checked};try{const res=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json','X-MEO-KEY':key},body:JSON.stringify(body)});const result=await res.json();if(!res.ok||!result.ok)throw new Error(result.error||'Could not save');settingsDirty=false;toast(result.persisted?'Thresholds saved to ESP32 memory':'Thresholds applied (memory persistence unavailable)');poll();}catch(e){toast('Settings not saved: '+e.message);}}
      document.querySelectorAll('[data-action]').forEach(b=>b.addEventListener('click',()=>control(b.dataset.action,Number(b.dataset.value)||0)));
      for(let i=0;i<4;i++){$('sceneSlot'+i).addEventListener('change',()=>{const chosen=Number($('sceneSlot'+i).value);const slots=[0,1,2,3].map(j=>Number($('sceneSlot'+j).value));if(new Set(slots).size!==4){toast('Pick four different scenes');poll();return;}control('scene_'+i,chosen);});}
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
      $('pairSave').addEventListener('click',()=>{try{sessionStorage.setItem('meodesk-key',pair.value.trim());toast('Pairing key saved for this browser tab');}catch(e){toast('Key active until this tab closes');}if(stateSocket&&stateSocket.readyState===WebSocket.OPEN)stateSocket.send(JSON.stringify({type:'subscribe',key:pair.value.trim()}));pollMedia();});
      pair.addEventListener('keydown',e=>{if(e.key==='Enter')$('pairSave').click();});
      document.querySelectorAll('[data-range]').forEach(b=>b.addEventListener('click',()=>{chartState.range=Number(b.dataset.range);chartState.hover=-1;document.querySelectorAll('[data-range]').forEach(el=>el.classList.toggle('active',el===b));if(chartState.history)renderHistory(chartState.history);$('chartTip').classList.remove('visible');}));
      $('historyCanvas').addEventListener('pointermove',moveHistoryHover);
      $('historyCanvas').addEventListener('pointerleave',()=>{chartState.hover=-1;$('chartTip').classList.remove('visible');drawHistory();});
      window.addEventListener('resize',drawHistory);
      window.addEventListener('resize',drawFpsHistory);
      try{chooseWorkspace(sessionStorage.getItem('meodesk-workspace')||'overview');}catch(_){chooseWorkspace('overview');}
      setInterval(()=>{if(focusAnchor){const age=performance.now()-focusAnchor.at;if(age<3000)paintFocus(focusAnchor,age);}},200);
      poll();pollHistory();pollMedia();pollFpsHistory();pollOtaStatus();pollCatPackStatus();setInterval(poll,1100);setInterval(pollHistory,3000);setInterval(()=>{if(!streamFresh())pollMedia();},2200);setInterval(pollFpsHistory,5000);setInterval(pollOtaStatus,15000);setInterval(pollCatPackStatus,16000);
    })();
  </script>
</body>
</html>
)MEODESK";
