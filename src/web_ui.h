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
<h3 style="margin:14px 0 6px;font-size:1rem">Disquettes virtuelles CPC (|DLOAD, |DEJECT, |DGP)</h3>
<div class="row">
<button class="sec" id="dCat">|DCAT</button>
<button class="sec" id="dEjA">Éjecter A:</button><button class="sec" id="dEjB">Éjecter B:</button>
<button class="sec" id="dGpA">Groupe A: (|DGP)</button><button class="sec" id="dGpB">Groupe B: (|DGP,"B")</button>
<button class="sec" id="dReset">Reset groupes (|DRESET)</button></div>
<div class="row" style="margin-top:8px">
<label>n° du 1er disque dans |DCAT <input type="number" id="dBase" value="1" min="0" max="9" style="width:4em"></label></div>
<div class="mut" style="margin-top:6px">Les boutons « Monter A / B » à côté de chaque .dsk tapent <code>|DLOAD,n</code> au clavier du CPC (il doit être à l'invite BASIC, ESP en mode <b>clavier</b>). Pour un groupe : « Groupe » puis « Monter » sur chaque disque (8 max). Le n° affiché vient de l'ordre du répertoire : vérifiez-le une fois avec |DCAT et corrigez « n° du 1er disque » si besoin.</div>

</section>

<section>
<h2>Clavier / souris vers la PicoCPC</h2>
<div id="zone" tabindex="0">Cliquez ici pour capturer le clavier.<br>Case « souris » cochée : le clic verrouille aussi le pointeur (Échap pour le libérer).</div>
<div class="row" style="margin-top:10px">
<label><input type="checkbox" id="mouseOn"> souris</label>
<button class="sec" id="kEsc">ESC</button>
<button class="sec" id="kReset">Reset CPC (Ctrl+Maj+Échap)</button>
<button class="sec" id="kRel">Tout relâcher</button>
<span id="wsState" class="chip">WS …</span><span id="rtt" class="chip">latence …</span></div>
<div class="row" style="margin-top:10px"><span>Mode USB vers la PicoCPC :</span>
<button class="sec" id="hKbd">Clavier</button><button class="sec" id="hMouse">Souris</button>
<span id="hidState" class="mut"></span></div>
<div class="mut">Le matériel n'offre qu'une interface HID « boot » à la fois : changer de mode redémarre l'ESP32 (la PicoCPC met ~10 s à le reconnaître).</div>
<div style="margin-top:12px"><b>Taper / coller du texte sur le CPC</b></div>
<textarea id="tText" rows="5" style="width:100%;box-sizing:border-box;font-family:monospace" placeholder="Collez ici un listing BASIC ou des commandes ; ils seront tapés touche par touche sur le CPC (qui doit être à l'invite)."></textarea>
<div class="row" style="margin-top:8px">
<button id="tGo">Taper</button><button class="del" id="tStop">Arrêter</button>
<label>Disposition PicoCPC <select id="kLang"><option value="FR" selected>FR</option><option value="FR_PIPE">FR_PIPE</option><option value="EN">EN</option></select></label>
<label>Vitesse <select id="tSpeed"><option value="70,70,400">lente</option><option value="40,40,200" selected>normale</option><option value="25,25,120">rapide</option></select></label>
<button class="sec" id="tTest">Test : taper |DCAT</button>
<span id="tProg" class="mut"></span></div>
<div class="mut">« Disposition » = valeur <code>KeyboardLang</code> de PicoCPC.cfg ([USB]) : <b>FR</b> = clavier français (| par AltGr+6), <b>FR_PIPE</b> = variante où la touche ù donne |, <b>EN</b> = clavier US. Si le A devient Q, choisissez FR. Ctrl+V dans la zone de capture colle aussi. Caractères sans touche équivalente (accents…) : ignorés et signalés.</div>
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

<section>
<h2>Mise à jour du firmware</h2>
<div class="mut">Version installée : <span id="fwVer">…</span></div>
<div class="row" style="margin-top:10px">
<input type="file" id="fwFile" accept=".bin">
<button id="fwGo" class="sec">Mettre à jour</button></div>
<div id="fwBar" style="display:none;margin-top:10px;height:8px;background:#8883;border-radius:4px;overflow:hidden"><div id="fwFill" style="height:100%;width:0;background:#4a8"></div></div>
<div id="fwMsg" class="msg"></div>
<div class="mut" style="margin-top:6px">Choisissez le fichier <b>firmware.bin</b> (dossier <code>release/</code> ou <code>.pio/build/picocpc_bridge/</code>), pas le fichier « complet ». Les fichiers de la clé et le Wi-Fi sont conservés. L'ESP32 redémarre puis la PicoCPC le reconnaît après ~10 s.</div>
</section>

<section>
<h2>Journal USB</h2>
<div class="row"><button id="lClear" class="sec">Effacer</button><span class="mut">Ce que la PicoCPC demande à la clé (80 dernières lignes).</span></div>
<pre id="log" style="max-height:260px;overflow:auto;font-size:12px;margin:8px 0 0"></pre>
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
  const key=x=>x.name.toUpperCase();
  fl.files.sort((x,y)=>(y.dir?1:0)-(x.dir?1:0)||(key(x)<key(y)?-1:key(x)>key(y)?1:0));   // ordre alphabétique comme |DCAT
  dskList=fl.files.filter(x=>!x.dir&&/\.dsk$/i.test(x.name)).map(x=>x.name);
  for(const f of fl.files){
   const tr=document.createElement('tr');
   const a=document.createElement('td');
   if(f.dir){a.textContent='['+f.name+']';a.className='mut'}else{const l=document.createElement('a');l.href='/download?name='+encodeURIComponent(f.name);l.textContent=f.name;l.style.color='var(--ac)';a.appendChild(l)}
   if(/\.dsk$/i.test(f.name)){const s2=document.createElement('div');s2.className='mut';s2.textContent='n° |DCAT supposé : '+dskIdx(f.name);a.appendChild(s2)}
   if(f.short&&f.short!==f.name){const s=document.createElement('div');s.className='mut';s.textContent='alias CPC : '+f.short;a.appendChild(s)}
   const n=document.createElement('td');n.className='n';n.textContent=f.dir?'':fmt(f.size);
   const c=document.createElement('td');c.className='act';
   if(!f.dir){
    const r=document.createElement('button');r.className='sec';r.textContent='Renommer';
    r.onclick=async()=>{const nn=prompt('Nouveau nom :',f.name);if(!nn||nn===f.name)return;try{await post('/api/rename',{from:f.name,to:nn});refresh()}catch(e){msg(e.message)}};
    const d=document.createElement('button');d.className='del';d.textContent='Supprimer';d.style.marginLeft='6px';
    d.onclick=async()=>{if(!confirm('Supprimer '+f.name+' ?'))return;try{await post('/api/delete',{name:f.name});refresh()}catch(e){msg(e.message)}};
    if(/\.dsk$/i.test(f.name)){
     const id=dskIdx(f.name);
     const ma=document.createElement('button'),mb=document.createElement('button');
     ma.className=mb.className='sec';ma.textContent='Monter A';mb.textContent='B';ma.style.marginRight='6px';mb.style.marginRight='6px';
     ma.title='|DLOAD,'+id;mb.title='|DLOAD,'+id+',"B"';
     ma.onclick=()=>mount(id,false);mb.onclick=()=>mount(id,true);
     c.append(ma,mb)}
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
$('#fwGo').onclick=()=>{
  const f=$('#fwFile').files[0],m=$('#fwMsg');
  if(!f){m.textContent='Choisissez d\'abord un fichier firmware.bin';return}
  if(!confirm('Installer '+f.name+' ('+Math.round(f.size/1024)+' Ko) ? L\'ESP32 va redémarrer.'))return;
  const fd=new FormData();fd.append('fw',f,f.name);
  const x=new XMLHttpRequest();x.open('POST','/api/fwupdate');
  $('#fwBar').style.display='block';$('#fwGo').disabled=true;m.textContent='Envoi…';
  x.upload.onprogress=e=>{if(e.lengthComputable){$('#fwFill').style.width=(100*e.loaded/e.total)+'%';m.textContent='Envoi… '+Math.round(100*e.loaded/e.total)+' %'}};
  x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}
    if(x.status==200&&j.ok){m.textContent='Firmware installé. Redémarrage…';setTimeout(()=>location.reload(),9000)}
    else{m.textContent='Échec : '+(j.error||x.status);$('#fwGo').disabled=false}};
  x.onerror=()=>{m.textContent='Connexion perdue (si l\'ESP32 redémarre, rechargez la page dans quelques secondes).';$('#fwGo').disabled=false};
  x.send(fd);
};
api('/api/status').then(s=>{$('#fwVer').textContent=s.fw||'?'}).catch(()=>{});
$('#hKbd').onclick=()=>post('/api/hidmode',{m:1}).then(()=>msg('Redémarrage en mode clavier…'));
$('#hMouse').onclick=()=>post('/api/hidmode',{m:2}).then(()=>msg('Redémarrage en mode souris…'));
api('/api/status').then(s=>{$('#hidState').textContent='Mode actuel : '+(s.hid==2?'souris':'clavier')}).catch(()=>{});
$('#bRemount').onclick=async()=>{try{await post('/api/remount');msg('Remontage en cours…');setTimeout(refresh,2500)}catch(e){msg(e.message)}};
$('#bFormat').onclick=async()=>{if(!confirm('Effacer TOUS les fichiers de la clé ?'))return;try{await post('/api/format',{confirm:'1'});refresh()}catch(e){msg(e.message)}};

/* ---------- clavier / souris ---------- */
/* ---------- saisie automatique de texte ---------- */
const SH=2,AG=64;
const US={};
(()=>{const row=(chars,u0)=>{[...chars].forEach((c,i)=>US[c]=[0,u0+i])};
 for(let i=0;i<26;i++){US[String.fromCharCode(97+i)]=[0,4+i];US[String.fromCharCode(65+i)]=[SH,4+i]}
 row('1234567890',0x1E);
 Object.assign(US,{'-':[0,0x2D],'=':[0,0x2E],'[':[0,0x2F],']':[0,0x30],'\\':[0,0x31],';':[0,0x33],"'":[0,0x34],'`':[0,0x35],',':[0,0x36],'.':[0,0x37],'/':[0,0x38],
  '_':[SH,0x2D],'+':[SH,0x2E],'{':[SH,0x2F],'}':[SH,0x30],'|':[SH,0x31],':':[SH,0x33],'"':[SH,0x34],'~':[SH,0x35],'<':[SH,0x36],'>':[SH,0x37],'?':[SH,0x38],
  '!':[SH,0x1E],'@':[SH,0x1F],'#':[SH,0x20],'$':[SH,0x21],'%':[SH,0x22],'^':[SH,0x23],'&':[SH,0x24],'*':[SH,0x25],'(':[SH,0x26],')':[SH,0x27]})})();
const FR={},FRP={};
(()=>{const L={a:0x14,z:0x1A,e:8,r:0x15,t:0x17,y:0x1C,u:0x18,i:0x0C,o:0x12,p:0x13,q:4,s:0x16,d:7,f:9,g:0x0A,h:0x0B,j:0x0D,k:0x0E,l:0x0F,m:0x33,w:0x1D,x:0x1B,c:6,v:0x19,b:5,n:0x11};
 for(const c in L){FR[c]=[0,L[c]];FR[c.toUpperCase()]=[SH,L[c]]}
 '1234567890'.split('').forEach((c,i)=>FR[c]=[SH,0x1E+i]);
 Object.assign(FR,{'&':[0,0x1E],'"':[0,0x20],"'":[0,0x21],'(':[0,0x22],'-':[0,0x23],'_':[0,0x25],')':[0,0x2D],'=':[0,0x2E],'$':[0,0x30],'*':[0,0x32],'<':[0,0x64],
  ',':[0,0x10],';':[0,0x36],':':[0,0x37],'!':[0,0x38],'?':[SH,0x10],'.':[SH,0x36],'/':[SH,0x37],'+':[SH,0x2E],'>':[SH,0x64],'%':[SH,0x34],'^':[0,0x2F],
  '~':[AG,0x1F],'#':[AG,0x20],'{':[AG,0x21],'[':[AG,0x22],'|':[AG,0x23],'`':[AG,0x24],'\\':[AG,0x25],'@':[AG,0x27],']':[AG,0x2D],'}':[AG,0x2E]})})();
Object.assign(FRP,FR,{'|':[0,0x34]});   // FR_PIPE : la touche ù donne |
function textToSteps(t,lang){
 const M=lang==='FR'?FR:lang==='FR_PIPE'?FRP:US,st=[],skip=new Set();
 t=t.replace(/\r\n?/g,'\n');
 for(const ch of t){
  if(ch==='\n'){st.push([0,0x28]);continue}
  if(ch==='\t'){st.push([0,0x2C]);continue}
  if(ch===' '){st.push([0,0x2C]);continue}
  const m=M[ch];if(m)st.push(m);else skip.add(ch)}
 return{st,skip}}
let tq=[],tTotal=0,espRem=0,tCfg='40,40,200';
const hx=n=>n.toString(16).padStart(2,'0');
function tPump(){
 while(tq.length&&espRem<500){const ch=tq.splice(0,100);
  send('t '+tCfg.split(',').join(' ')+' '+ch.map(s=>hx(s[0])+hx(s[1])).join(''));espRem+=ch.length}
 showProg()}
function showProg(){
 const left=tq.length+espRem;
 $('#tProg').textContent=tTotal&&left?('Saisie en cours… '+Math.round(100*(tTotal-left)/tTotal)+' %'):(tTotal?'Terminé.':'');
 if(!left)tTotal=0}
function typeText(txt,quiet){
 if(!ws||ws.readyState!==1){$('#tProg').textContent='WebSocket non connecté';return}
 const r=textToSteps(txt,$('#kLang').value);
 tCfg=$('#tSpeed').value;
 tq=tq.concat(r.st);tTotal+=r.st.length;tPump();
 if(r.skip.size&&!quiet)$('#tProg').textContent='Ignorés : '+[...r.skip].join(' ')}
const PAUSE=n=>[[0xFF,n]];
function cmds(lines,pauseAfterFirst){
 const lang=$('#kLang').value;let steps=[];
 lines.forEach((l,i)=>{steps=steps.concat(textToSteps(l,lang).st,[[0,0x28]]);if(i<lines.length-1)steps=steps.concat(PAUSE(pauseAfterFirst||40))});
 if(!ws||ws.readyState!==1){$('#tProg').textContent='WebSocket non connecté';return}
 tCfg=$('#tSpeed').value;tq=tq.concat(steps);tTotal+=steps.length;tPump()}
const base=()=>parseInt($('#dBase').value||'1',10);
let dskList=[];
const dskIdx=name=>dskList.indexOf(name)+base();
function mount(id,b){
 cmds(['|DLOAD,'+id+(b?',"B"':'')])}
$('#dCat').onclick=()=>cmds(['|DCAT']);
$('#dEjA').onclick=()=>cmds(['|DEJECT']);$('#dEjB').onclick=()=>cmds(['|DEJECT,"B"']);
$('#dGpA').onclick=()=>cmds(['|DGP']);$('#dGpB').onclick=()=>cmds(['|DGP,"B"']);
$('#dReset').onclick=()=>cmds(['|DRESET']);
$('#dBase').onchange=()=>{try{localStorage.dBase=$('#dBase').value}catch(e){}refresh()};
$('#kLang').onchange=()=>{try{localStorage.kLang=$('#kLang').value}catch(e){}};
$('#tSpeed').onchange=()=>{try{localStorage.tSpeed=$('#tSpeed').value}catch(e){}};
try{if(localStorage.dBase)$('#dBase').value=localStorage.dBase;if(localStorage.kLang)$('#kLang').value=localStorage.kLang;if(localStorage.tSpeed)$('#tSpeed').value=localStorage.tSpeed}catch(e){}
$('#tGo').onclick=()=>typeText($('#tText').value);
$('#tStop').onclick=()=>{tq=[];espRem=0;tTotal=0;send('x');$('#tProg').textContent='Arrêté.'};
$('#tTest').onclick=()=>cmds(['|DCAT']);

const MODS={ControlLeft:1,ShiftLeft:2,AltLeft:4,MetaLeft:8,ControlRight:16,ShiftRight:32,AltRight:64,MetaRight:128};
const KM={Enter:0x28,Escape:0x29,Backspace:0x2A,Tab:0x2B,Space:0x2C,Minus:0x2D,Equal:0x2E,BracketLeft:0x2F,BracketRight:0x30,Backslash:0x31,IntlHash:0x32,Semicolon:0x33,Quote:0x34,Backquote:0x35,Comma:0x36,Period:0x37,Slash:0x38,CapsLock:0x39,PrintScreen:0x46,ScrollLock:0x47,Pause:0x48,Insert:0x49,Home:0x4A,PageUp:0x4B,Delete:0x4C,End:0x4D,PageDown:0x4E,ArrowRight:0x4F,ArrowLeft:0x50,ArrowDown:0x51,ArrowUp:0x52,NumLock:0x53,NumpadDivide:0x54,NumpadMultiply:0x55,NumpadSubtract:0x56,NumpadAdd:0x57,NumpadEnter:0x58,NumpadDecimal:0x63,IntlBackslash:0x64,ContextMenu:0x65};
for(let i=0;i<26;i++)KM['Key'+String.fromCharCode(65+i)]=4+i;
for(let i=1;i<=9;i++){KM['Digit'+i]=0x1D+i;KM['Numpad'+i]=0x58+i}
KM.Digit0=0x27;KM.Numpad0=0x62;
for(let i=1;i<=12;i++)KM['F'+i]=0x39+i;
const rtts=[];setInterval(()=>{if(ws&&ws.readyState===1)ws.send('P '+performance.now())},1000);
let ws=null,mods=0,keys=[],mx=0,my=0,mw=0,btn=0;
const wsState=$('#wsState');
function wsc(){ws=new WebSocket('ws://'+location.hostname+':81/');
 ws.onopen=()=>{wsState.textContent='WS ok';wsState.className='chip ok';espRem=0};
 ws.onmessage=e=>{const d=String(e.data);
  if(d[0]==='t'){espRem=parseInt(d.slice(2),10)||0;tPump()}
  else if(d[0]==='P'){const r=performance.now()-parseFloat(d.slice(2));rtts.push(r);if(rtts.length>10)rtts.shift();
   const a=rtts.reduce((x,y)=>x+y,0)/rtts.length;$('#rtt').textContent='latence '+Math.round(a)+' ms (max '+Math.round(Math.max(...rtts))+')'}};
 ws.onclose=()=>{wsState.textContent='WS coupé';wsState.className='chip bad';setTimeout(wsc,1500)};}
const send=t=>{if(ws&&ws.readyState===1)ws.send(t)};
function sendKeys(){const k=keys.slice(0,6);while(k.length<6)k.push(0);send('k '+mods+' '+k.join(' '))}
function relAll(){mods=0;keys=[];btn=0;mx=my=mw=0;send('r')}
const zone=$('#zone');
function kd(e,down){
 if(down&&(e.ctrlKey||e.metaKey)&&e.code==='KeyV'){return}   // laisse le navigateur declencher l'evenement paste
 e.preventDefault();e.stopPropagation();if(e.repeat)return;
 if(MODS[e.code]){down?mods|=MODS[e.code]:mods&=~MODS[e.code]}
 else{const u=KM[e.code];if(!u)return;const i=keys.indexOf(u);if(down&&i<0)keys.push(u);if(!down&&i>=0)keys.splice(i,1)}
 sendKeys()}
zone.addEventListener('keydown',e=>kd(e,true));zone.addEventListener('keyup',e=>kd(e,false));
zone.addEventListener('paste',e=>{e.preventDefault();const t=(e.clipboardData||window.clipboardData).getData('text');if(t){relAll();typeText(t)}});
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

/* ---------- journal ---------- */
async function logLoad(){try{const r=await fetch('/api/log');const t=await r.text();const p=$('#log');const bottom=p.scrollTop+p.clientHeight>=p.scrollHeight-8;p.textContent=t;if(bottom)p.scrollTop=p.scrollHeight}catch(e){}}
$('#lClear').onclick=async()=>{await post('/api/log/clear');logLoad()};
setInterval(logLoad,2000);logLoad();

wsc();refresh();bleRefresh();wifiLoad();
setInterval(refresh,5000);setInterval(bleRefresh,2000);
</script></body></html>
)HTMLPAGE";
