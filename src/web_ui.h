#pragma once
#include <pgmspace.h>

// Page web unique (servie depuis la flash du programme).
static const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PicoCPC USB Bridge</title>
<style>
:root{--bg:#14161a;--card:#1d2026;--line:#2c313a;--tx:#e6e8ec;--mut:#8b93a1;--ac:#4cc2ff;--ok:#4cd388;--bad:#ff6b6b;--warn:#ffc857}
@media(prefers-color-scheme:light){:root{--bg:#f3f4f7;--card:#fff;--line:#dcdfe5;--tx:#1b1e24;--mut:#667085;--ac:#0a7bd6;--ok:#12874f;--bad:#c53030;--warn:#b7791f}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--tx);font:15px/1.45 system-ui,sans-serif}
header{padding:14px 16px;display:flex;flex-wrap:wrap;gap:10px;align-items:center;border-bottom:1px solid var(--line)}
h1{font-size:18px;margin:0 8px 0 0}h2{font-size:15px;margin:0 0 10px}
.chip{font-size:12px;padding:3px 9px;border-radius:99px;border:1px solid var(--line);color:var(--mut)}
.chip.ok{color:var(--ok);border-color:var(--ok)}.chip.bad{color:var(--bad);border-color:var(--bad)}
main{max-width:980px;margin:0 auto;padding:16px;display:grid;gap:16px}
section{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px}
button{background:var(--ac);color:#fff;border:0;border-radius:7px;padding:6px 12px;font:inherit;cursor:pointer}
button.sec{background:transparent;color:var(--tx);border:1px solid var(--line)}
button.del{background:transparent;color:var(--bad);border:1px solid var(--line)}
button:disabled{opacity:.5;cursor:default}
.bar{height:8px;background:var(--line);border-radius:5px;overflow:hidden;margin:6px 0}.bar>i{display:block;height:100%;background:var(--ac)}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
#drop{border:2px dashed var(--line);border-radius:10px;padding:18px;text-align:center;color:var(--mut);margin:10px 0;cursor:pointer}
#drop.over{border-color:var(--ac);color:var(--ac)}
table{width:100%;border-collapse:collapse}td,th{padding:6px 8px;border-bottom:1px solid var(--line);text-align:left;vertical-align:middle}
th{color:var(--mut);font-weight:500;font-size:12px}td.n{text-align:right;white-space:nowrap}td.act{text-align:right;white-space:nowrap}
.mut{color:var(--mut);font-size:13px}.msg{min-height:20px;font-size:13px}
#zone{border:2px solid var(--line);border-radius:10px;height:190px;display:flex;align-items:center;justify-content:center;text-align:center;color:var(--mut);outline:none;user-select:none;padding:10px}
#zone:focus{border-color:var(--ac);color:var(--tx)}#zone.lock{border-color:var(--ok);color:var(--ok)}
.prog{font-size:12px;color:var(--mut)}
</style></head><body>
<header><h1>PicoCPC · Pont USB</h1>
<span id="cUsb" class="chip">USB …</span><span id="cNet" class="chip">…</span><span id="cPend" class="chip" hidden>mise à jour en attente</span></header>
<main>

<section>
<h2>Clé USB virtuelle</h2>
<div class="row"><span id="spaceTxt" class="mut"></span></div>
<div class="bar"><i id="spaceBar" style="width:0"></i></div>
<div id="drop">Glissez des fichiers ici ou cliquez pour les choisir<input id="pick" type="file" multiple hidden></div>
<div id="upl" class="prog"></div>
<table><thead><tr><th>Nom</th><th class="n">Taille</th><th></th></tr></thead><tbody id="files"></tbody></table>
<div class="row" style="margin-top:10px">
<button id="bRemount" class="sec">Remonter la clé maintenant</button>
<button id="bFormat" class="del">Formater…</button>
<span class="mut">La PicoCPC voit la clé retirée puis remise (clavier/souris coupés ~2 s).</span></div>
<div id="fmsg" class="msg"></div>
</section>

<section>
<h2>Clavier / souris vers la PicoCPC</h2>
<div id="zone" tabindex="0">Cliquez ici pour capturer le clavier.<br>Case « souris » cochée : le clic verrouille aussi le pointeur (Échap pour le libérer).</div>
<div class="row" style="margin-top:10px">
<label><input type="checkbox" id="mouseOn"> souris</label>
<button class="sec" id="kEsc">ESC</button>
<button class="sec" id="kReset">Reset CPC (Ctrl+Maj+Échap)</button>
<button class="sec" id="kRel">Tout relâcher</button>
<span id="wsState" class="chip">WS …</span></div>
<div class="mut" style="margin-top:6px">Les touches sont envoyées par position physique : la PicoCPC applique sa propre disposition (KEYBOARDLANG).</div>
</section>

<section>
<h2>Bluetooth (clavier / souris)</h2>
<div class="row"><button id="bScan">Rechercher</button>
<span class="mut">Code d'appairage clavier : <b id="pin">…</b></span></div>
<div id="bmsg" class="msg"></div>
<table><tbody id="ble"></tbody></table>
</section>

<section>
<h2>Wi-Fi</h2>
<div id="wInfo" class="mut"></div>
<div class="row" style="margin-top:10px">
<input id="wSsid" list="wList" placeholder="Nom du réseau (SSID)" style="flex:1;min-width:180px;padding:6px;border-radius:7px;border:1px solid var(--line);background:var(--bg);color:var(--tx)">
<datalist id="wList"></datalist>
<input id="wPass" type="password" placeholder="Mot de passe" style="flex:1;min-width:140px;padding:6px;border-radius:7px;border:1px solid var(--line);background:var(--bg);color:var(--tx)">
<button id="wScan" class="sec">Chercher les réseaux</button>
<button id="wSave">Enregistrer et redémarrer</button></div>
<div class="mut" style="margin-top:6px">SSID vide = retour au point d'accès de l'ESP32. Après redémarrage, rouvrez <b>http://picocpc.local/</b> (ou l'IP affichée sur le moniteur série).</div>
<div id="wMsg" class="msg"></div>
</section>
</main>
<script>
const $=s=>document.querySelector(s);
const fmt=n=>n<1024?n+' o':n<1048576?(n/1024).toFixed(1)+' Ko':(n/1048576).toFixed(2)+' Mo';
async function api(p,o){const r=await fetch(p,o);const t=await r.text();let j={};try{j=JSON.parse(t)}catch(e){}
 if(!r.ok||j.ok===false)throw new Error(j.error||t||('HTTP '+r.status));return j}
const post=(p,q)=>api(p+(q?'?'+new URLSearchParams(q):''),{method:'POST'});

/* ---------- fichiers ---------- */
async function refresh(){
 try{
  const [st,fl]=await Promise.all([api('/api/status'),api('/api/files')]);
  const used=st.total-st.free;
  $('#spaceTxt').textContent=fmt(st.free)+' libres sur '+fmt(st.total)+' ('+fl.files.length+' élément(s))';
  $('#spaceBar').style.width=(st.total?100*used/st.total:0)+'%';
  const u=$('#cUsb');u.textContent=st.usb?'USB : PicoCPC connectée':'USB : non connecté';u.className='chip '+(st.usb?'ok':'bad');
  $('#cNet').textContent=st.ip;$('#cPend').hidden=!st.pending;
  const tb=$('#files');tb.innerHTML='';
  if(!fl.files.length){tb.innerHTML='<tr><td colspan=3 class="mut">Aucun fichier</td></tr>'}
  for(const f of fl.files){
   const tr=document.createElement('tr');
   const a=document.createElement('td');
   if(f.dir){a.textContent='['+f.name+']';a.className='mut'}else{const l=document.createElement('a');l.href='/download?name='+encodeURIComponent(f.name);l.textContent=f.name;l.style.color='var(--ac)';a.appendChild(l)}
   if(f.short&&f.short!==f.name){const s=document.createElement('div');s.className='mut';s.textContent='alias CPC : '+f.short;a.appendChild(s)}
   const n=document.createElement('td');n.className='n';n.textContent=f.dir?'':fmt(f.size);
   const c=document.createElement('td');c.className='act';
   if(!f.dir){
    const r=document.createElement('button');r.className='sec';r.textContent='Renommer';
    r.onclick=async()=>{const nn=prompt('Nouveau nom :',f.name);if(!nn||nn===f.name)return;try{await post('/api/rename',{from:f.name,to:nn});refresh()}catch(e){msg(e.message)}};
    const d=document.createElement('button');d.className='del';d.textContent='Supprimer';d.style.marginLeft='6px';
    d.onclick=async()=>{if(!confirm('Supprimer '+f.name+' ?'))return;try{await post('/api/delete',{name:f.name});refresh()}catch(e){msg(e.message)}};
    c.append(r,d)}
   tr.append(a,n,c);tb.appendChild(tr)}
 }catch(e){msg(e.message)}
}
function msg(t){$('#fmsg').textContent=t}
function up(file,label){return new Promise((res,rej)=>{
 const x=new XMLHttpRequest(),fd=new FormData();fd.append('file',file,file.name);
 x.open('POST','/api/upload');
 x.upload.onprogress=e=>{if(e.lengthComputable)$('#upl').textContent=label+' '+file.name+' : '+Math.round(100*e.loaded/e.total)+' %'};
 x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}x.status==200?res():rej(new Error(file.name+' : '+(j.error||x.status)))};
 x.onerror=()=>rej(new Error(file.name+' : erreur réseau'));x.send(fd)})}
async function upAll(list){
 const fl=[...list];let i=0;
 try{for(const f of fl){i++;await up(f,'('+i+'/'+fl.length+')')}$('#upl').textContent='Envoi terminé.';msg('')}
 catch(e){$('#upl').textContent='';msg(e.message)}
 refresh()}
const drop=$('#drop');
drop.onclick=()=>$('#pick').click();$('#pick').onchange=e=>{upAll(e.target.files);e.target.value=''};
drop.ondragover=e=>{e.preventDefault();drop.classList.add('over')};drop.ondragleave=()=>drop.classList.remove('over');
drop.ondrop=e=>{e.preventDefault();drop.classList.remove('over');upAll(e.dataTransfer.files)};
$('#bRemount').onclick=async()=>{try{await post('/api/remount');msg('Remontage en cours…');setTimeout(refresh,2500)}catch(e){msg(e.message)}};
$('#bFormat').onclick=async()=>{if(!confirm('Effacer TOUS les fichiers de la clé ?'))return;try{await post('/api/format',{confirm:'1'});refresh()}catch(e){msg(e.message)}};

/* ---------- clavier / souris ---------- */
const MODS={ControlLeft:1,ShiftLeft:2,AltLeft:4,MetaLeft:8,ControlRight:16,ShiftRight:32,AltRight:64,MetaRight:128};
const KM={Enter:0x28,Escape:0x29,Backspace:0x2A,Tab:0x2B,Space:0x2C,Minus:0x2D,Equal:0x2E,BracketLeft:0x2F,BracketRight:0x30,Backslash:0x31,IntlHash:0x32,Semicolon:0x33,Quote:0x34,Backquote:0x35,Comma:0x36,Period:0x37,Slash:0x38,CapsLock:0x39,PrintScreen:0x46,ScrollLock:0x47,Pause:0x48,Insert:0x49,Home:0x4A,PageUp:0x4B,Delete:0x4C,End:0x4D,PageDown:0x4E,ArrowRight:0x4F,ArrowLeft:0x50,ArrowDown:0x51,ArrowUp:0x52,NumLock:0x53,NumpadDivide:0x54,NumpadMultiply:0x55,NumpadSubtract:0x56,NumpadAdd:0x57,NumpadEnter:0x58,NumpadDecimal:0x63,IntlBackslash:0x64,ContextMenu:0x65};
for(let i=0;i<26;i++)KM['Key'+String.fromCharCode(65+i)]=4+i;
for(let i=1;i<=9;i++){KM['Digit'+i]=0x1D+i;KM['Numpad'+i]=0x58+i}
KM.Digit0=0x27;KM.Numpad0=0x62;
for(let i=1;i<=12;i++)KM['F'+i]=0x39+i;
let ws=null,mods=0,keys=[],mx=0,my=0,mw=0,btn=0;
const wsState=$('#wsState');
function wsc(){ws=new WebSocket('ws://'+location.hostname+':81/');
 ws.onopen=()=>{wsState.textContent='WS ok';wsState.className='chip ok'};
 ws.onclose=()=>{wsState.textContent='WS coupé';wsState.className='chip bad';setTimeout(wsc,1500)};}
const send=t=>{if(ws&&ws.readyState===1)ws.send(t)};
function sendKeys(){const k=keys.slice(0,6);while(k.length<6)k.push(0);send('k '+mods+' '+k.join(' '))}
function relAll(){mods=0;keys=[];btn=0;mx=my=mw=0;send('r')}
const zone=$('#zone');
function kd(e,down){
 e.preventDefault();e.stopPropagation();if(e.repeat)return;
 if(MODS[e.code]){down?mods|=MODS[e.code]:mods&=~MODS[e.code]}
 else{const u=KM[e.code];if(!u)return;const i=keys.indexOf(u);if(down&&i<0)keys.push(u);if(!down&&i>=0)keys.splice(i,1)}
 sendKeys()}
zone.addEventListener('keydown',e=>kd(e,true));zone.addEventListener('keyup',e=>kd(e,false));
zone.addEventListener('blur',relAll);zone.addEventListener('contextmenu',e=>e.preventDefault());
zone.addEventListener('click',()=>{zone.focus();if($('#mouseOn').checked&&document.pointerLockElement!==zone)zone.requestPointerLock()});
document.addEventListener('pointerlockchange',()=>{const l=document.pointerLockElement===zone;zone.classList.toggle('lock',l);if(!l){btn=0;send('m 0 0 0 0')}});
const BM={0:1,2:2,1:4};
document.addEventListener('mousemove',e=>{if(document.pointerLockElement===zone){mx+=e.movementX;my+=e.movementY}});
document.addEventListener('mousedown',e=>{if(document.pointerLockElement===zone){flush();btn|=BM[e.button]||0;send('m '+btn+' 0 0 0');e.preventDefault()}});
document.addEventListener('mouseup',e=>{if(document.pointerLockElement===zone){flush();btn&=~(BM[e.button]||0);send('m '+btn+' 0 0 0');e.preventDefault()}});
document.addEventListener('wheel',e=>{if(document.pointerLockElement===zone){mw+=e.deltaY>0?-1:1;e.preventDefault()}},{passive:false});
function flush(){if(mx||my||mw){const c=v=>Math.max(-127,Math.min(127,v));
 while(mx||my||mw){const a=c(mx),b=c(my),w=c(mw);send('m '+btn+' '+a+' '+b+' '+w);mx-=a;my-=b;mw-=w}}}
setInterval(flush,16);
const tap=(m,k)=>{mods=m;keys=k?[k]:[];sendKeys();setTimeout(()=>{mods=0;keys=[];sendKeys()},120)};
$('#kEsc').onclick=()=>tap(0,0x29);$('#kReset').onclick=()=>tap(1|2,0x29);$('#kRel').onclick=relAll;

/* ---------- bluetooth ---------- */
async function bleRefresh(){
 try{const b=await api('/api/ble');$('#pin').textContent=b.pin;$('#bmsg').textContent=b.status;$('#bScan').disabled=b.scanning;
  const tb=$('#ble');tb.innerHTML='';
  if(!b.devices.length)tb.innerHTML='<tr><td class="mut">Aucun appareil. Mettez le clavier/la souris en mode appairage puis « Rechercher ».</td></tr>';
  for(const d of b.devices){
   const tr=document.createElement('tr'),a=document.createElement('td'),c=document.createElement('td');c.className='act';
   a.textContent=(d.name||'(sans nom)')+'  ';const s=document.createElement('span');s.className='mut';
   s.textContent=d.addr+(d.rssi!==undefined?' · '+d.rssi+' dBm':'')+(d.kind?' · '+d.kind:'')+(d.hid?'':' · non HID');a.appendChild(s);
   if(d.connected){const t=document.createElement('span');t.className='chip ok';t.textContent='connecté';t.style.marginLeft='6px';a.appendChild(t)}
   else if(d.bonded){const t=document.createElement('span');t.className='chip';t.textContent='appairé';t.style.marginLeft='6px';a.appendChild(t)}
   if(!d.connected){const x=document.createElement('button');x.textContent='Connecter';x.onclick=async()=>{await post('/api/ble/connect',{addr:d.addr});setTimeout(bleRefresh,500)};c.appendChild(x)}
   if(d.bonded||d.connected){const f=document.createElement('button');f.className='del';f.textContent='Oublier';f.style.marginLeft='6px';f.onclick=async()=>{await post('/api/ble/forget',{addr:d.addr});setTimeout(bleRefresh,500)};c.appendChild(f)}
   tr.append(a,c);tb.appendChild(tr)}
 }catch(e){}}
$('#bScan').onclick=async()=>{await post('/api/ble/scan');setTimeout(bleRefresh,300)};

/* ---------- wifi ---------- */
async function wifiLoad(){try{const w=await api('/api/wifi');
 $('#wInfo').textContent=(w.mode==='STA'?'Connecté au réseau « '+w.ssid+' »':'Point d’accès « '+w.ap+' » (aucun réseau configuré ou connexion échouée)')+' · IP '+w.ip;
 $('#wSsid').value=w.ssid||''}catch(e){}}
$('#wScan').onclick=async()=>{$('#wMsg').textContent='Recherche… (quelques secondes)';
 try{const r=await api('/api/wifi/scan');const dl=$('#wList');dl.innerHTML='';
  for(const n of r.nets){const o=document.createElement('option');o.value=n.ssid;o.label=n.ssid+' ('+n.rssi+' dBm)';dl.appendChild(o)}
  $('#wMsg').textContent=r.nets.length+' réseau(x) trouvé(s) : choisissez-en un dans la liste du champ.'}catch(e){$('#wMsg').textContent=e.message}};
$('#wSave').onclick=async()=>{
 if(!confirm('Enregistrer et redémarrer l’ESP32 ?'))return;
 try{await post('/api/wifi/save',{ssid:$('#wSsid').value,pass:$('#wPass').value});
  $('#wMsg').textContent='Enregistré. Redémarrage en cours : cette page va se couper.'}catch(e){$('#wMsg').textContent=e.message}};

wsc();refresh();bleRefresh();wifiLoad();
setInterval(refresh,5000);setInterval(bleRefresh,2000);
</script></body></html>
)HTMLPAGE";
