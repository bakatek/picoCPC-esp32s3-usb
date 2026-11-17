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
<header><h1 data-i="hdr_title">PicoCPC · Pont USB</h1>
<span id="cUsb" class="chip">USB …</span><span id="cNet" class="chip">…</span><span id="cPend" class="chip" data-i="c_pend" hidden>mise à jour en attente</span><select id="lang" title="Language / Langue / Idioma" style="margin-left:auto;padding:4px 6px;border-radius:7px;border:1px solid var(--line);background:var(--card);color:var(--tx)"><option value="fr">Français</option><option value="en">English</option><option value="es">Español</option></select></header>
<main>

<section>
<h2 data-i="h_usb">Clé USB virtuelle</h2>
<div class="row"><span id="spaceTxt" class="mut"></span></div>
<div class="bar"><i id="spaceBar" style="width:0"></i></div>
<div id="drop"><span data-i="drop">Glissez des fichiers ici ou cliquez pour les choisir</span><input id="pick" type="file" multiple hidden></div>
<div id="upl" class="prog"></div>
<table><thead><tr><th data-i="th_name">Nom</th><th class="n" data-i="th_size">Taille</th><th></th></tr></thead><tbody id="files"></tbody></table>
<div class="row" style="margin-top:10px">
<button id="bRemount" class="sec" data-i="b_remount">Remonter la clé maintenant</button>
<button id="bFormat" class="del" data-i="b_format">Formater…</button>
<span class="mut" data-i="t_remount">La PicoCPC voit la clé retirée puis remise (clavier/souris coupés ~2 s).</span></div>
<div id="fmsg" class="msg"></div>
<h3 style="margin:14px 0 6px;font-size:1rem" data-i="h_disk">Disquettes virtuelles CPC (|DLOAD, |DEJECT, |DGP)</h3>
<div class="row">
<button class="sec" id="dCat">|DCAT</button>
<button class="sec" id="dEjA" data-i="b_ejA">Éjecter A:</button><button class="sec" id="dEjB" data-i="b_ejB">Éjecter B:</button>
<button class="sec" id="dGpA" data-i="b_gpA">Groupe A: (|DGP)</button><button class="sec" id="dGpB" data-i="b_gpB">Groupe B: (|DGP,"B")</button>
<button class="sec" id="dReset" data-i="b_dreset">Reset groupes (|DRESET)</button></div>
<div class="row" style="margin-top:8px">
<label><span data-i="l_base">n° du 1er disque dans |DCAT</span> <input type="number" id="dBase" value="0" min="0" max="9" style="width:4em"></label></div>
<div class="mut" style="margin-top:6px" data-ih="disk_help">Les boutons « Monter A / B » à côté de chaque .dsk tapent <code>|DLOAD,n</code> au clavier du CPC (il doit être à l'invite BASIC, ESP en mode <b>clavier</b>). Pour un groupe : « Groupe » puis « Monter » sur chaque disque (8 max). Le n° affiché vient de l'ordre du répertoire : vérifiez-le une fois avec |DCAT et corrigez « n° du 1er disque » si besoin.</div>

</section>

<section>
<h2 data-i="h_kbd">Clavier / souris vers la PicoCPC</h2>
<div id="zone" tabindex="0" data-ih="zone">Cliquez ici pour capturer le clavier.<br>Case « souris » cochée : le clic verrouille aussi le pointeur (Échap pour le libérer).</div>
<div class="row" style="margin-top:10px">
<label><input type="checkbox" id="mouseOn"> <span data-i="l_mouse">souris</span></label>
<button class="sec" id="kEsc">ESC</button>
<button class="sec" id="kReset" data-i="b_cpcreset">Reset CPC (Ctrl+Maj+Échap)</button>
<button class="sec" id="kRel" data-i="b_relall">Tout relâcher</button>
<span id="wsState" class="chip">WS …</span><span id="rtt" class="chip" data-i="rtt_wait">latence …</span></div>
<div class="row" style="margin-top:10px"><span data-i="l_mode">Mode USB vers la PicoCPC :</span>
<button class="sec" id="hKbd" data-i="b_kbd">Clavier</button><button class="sec" id="hMouse" data-i="b_mouse">Souris</button>
<span id="hidState" class="mut"></span></div>
<div class="mut" data-i="hid_note">Le matériel n'offre qu'une interface HID « boot » à la fois : changer de mode redémarre l'ESP32 (la PicoCPC met ~10 s à le reconnaître).</div>
<div style="margin-top:12px"><b data-i="h_type">Taper / coller du texte sur le CPC</b></div>
<textarea id="tText" rows="5" style="width:100%;box-sizing:border-box;font-family:monospace" data-ip="ph_text" placeholder=""></textarea>
<div class="row" style="margin-top:8px">
<button id="tGo" data-i="b_type">Taper</button><button class="del" id="tStop" data-i="b_stop">Arrêter</button>
<label><span data-i="l_layout">Disposition PicoCPC</span> <select id="kLang"><option value="FR" selected>FR</option><option value="FR_PIPE">FR_PIPE</option><option value="EN">EN</option></select></label>
<label><span data-i="l_speed">Vitesse</span> <select id="tSpeed"><option value="70,70,400" data-i="sp_slow">lente</option><option value="40,40,200" selected data-i="sp_norm">normale</option><option value="25,25,120" data-i="sp_fast">rapide</option></select></label>
<button class="sec" id="tTest" data-i="b_test">Test : taper |DCAT</button>
<span id="tProg" class="mut"></span></div>
<div class="mut" data-ih="layout_help">« Disposition » = valeur <code>KeyboardLang</code> de PicoCPC.cfg ([USB]) : <b>FR</b> = clavier français (| par AltGr+6), <b>FR_PIPE</b> = variante où la touche ù donne |, <b>EN</b> = clavier US. Si le A devient Q, choisissez FR. Ctrl+V dans la zone de capture colle aussi. Caractères sans touche équivalente (accents…) : ignorés et signalés.</div>
<div class="mut" style="margin-top:6px" data-i="pos_note">Les touches sont envoyées par position physique : la PicoCPC applique sa propre disposition (KEYBOARDLANG).</div>
</section>

<section>
<h2 data-i="h_ble">Bluetooth (clavier / souris)</h2>
<div class="row"><button id="bScan" data-i="b_scan">Rechercher</button>
<span class="mut"><span data-i="pin_lbl">Code d'appairage clavier :</span> <b id="pin">…</b></span></div>
<div id="bmsg" class="msg"></div>
<table><tbody id="ble"></tbody></table>
</section>

<section>
<h2 data-i="h_wifi">Wi-Fi</h2>
<div id="wInfo" class="mut"></div>
<div class="row" style="margin-top:10px">
<input id="wSsid" list="wList" data-ip="ph_ssid" placeholder="Nom du réseau (SSID)" style="flex:1;min-width:180px;padding:6px;border-radius:7px;border:1px solid var(--line);background:var(--bg);color:var(--tx)">
<datalist id="wList"></datalist>
<input id="wPass" type="password" data-ip="ph_pass" placeholder="Mot de passe" style="flex:1;min-width:140px;padding:6px;border-radius:7px;border:1px solid var(--line);background:var(--bg);color:var(--tx)">
<button id="wScan" class="sec" data-i="b_wscan">Chercher les réseaux</button>
<button id="wSave" data-i="b_wsave">Enregistrer et redémarrer</button></div>
<div class="mut" style="margin-top:6px" data-ih="wifi_help">SSID vide = retour au point d'accès de l'ESP32. Après redémarrage, rouvrez <b>http://picocpc.local/</b> (ou l'IP affichée sur le moniteur série).</div>
<div id="wMsg" class="msg"></div>
</section>

<section>
<h2 data-i="h_fw">Mise à jour du firmware</h2>
<div class="mut"><span data-i="fw_ver">Version installée :</span> <span id="fwVer">…</span></div>
<div class="row" style="margin-top:10px">
<input type="file" id="fwFile" accept=".bin">
<button id="fwGo" class="sec" data-i="b_fwgo">Mettre à jour</button></div>
<div id="fwBar" style="display:none;margin-top:10px;height:8px;background:#8883;border-radius:4px;overflow:hidden"><div id="fwFill" style="height:100%;width:0;background:#4a8"></div></div>
<div id="fwMsg" class="msg"></div>
<div class="mut" style="margin-top:6px" data-ih="fw_help">Choisissez le fichier <b>firmware.bin</b> (dossier <code>release/</code> ou <code>.pio/build/picocpc_bridge/</code>), pas le fichier « complet ». Les fichiers de la clé et le Wi-Fi sont conservés. L'ESP32 redémarre puis la PicoCPC le reconnaît après ~10 s.</div>
</section>

<section>
<h2 data-i="h_log">Journal USB</h2>
<div class="row"><button id="lClear" class="sec" data-i="b_lclear">Effacer</button><span class="mut" data-i="log_note">Ce que la PicoCPC demande à la clé (80 dernières lignes).</span></div>
<pre id="log" style="max-height:260px;overflow:auto;font-size:12px;margin:8px 0 0"></pre>
</section>
</main>
<script>
const $=s=>document.querySelector(s);

/* ---------- langues (fr / en / es) ---------- */
const L={"fr":{"hdr_title":"PicoCPC · Pont USB","c_usb_ok":"USB : PicoCPC connectée","c_usb_ko":"USB : non connecté","c_pend":"mise à jour en attente","u_o":"o","u_k":"Ko","u_m":"Mo","h_usb":"Clé USB virtuelle","space":"{0} libres sur {1} ({2} élément(s))","drop":"Glissez des fichiers ici ou cliquez pour les choisir","th_name":"Nom","th_size":"Taille","b_remount":"Remonter la clé maintenant","b_format":"Formater…","t_remount":"La PicoCPC voit la clé retirée puis remise (clavier/souris coupés ~2 s).","no_files":"Aucun fichier","dsk_guess":"n° |DCAT supposé : {0}","alias":"alias CPC : {0}","b_rename":"Renommer","b_delete":"Supprimer","p_rename":"Nouveau nom :","c_delete":"Supprimer {0} ?","b_mountA":"Monter A","upl_done":"Envoi terminé.","net_err":"erreur réseau","c_format":"Effacer TOUS les fichiers de la clé ?","m_remount":"Remontage en cours…","h_disk":"Disquettes virtuelles CPC (|DLOAD, |DEJECT, |DGP)","b_ejA":"Éjecter A:","b_ejB":"Éjecter B:","b_gpA":"Groupe A: (|DGP)","b_gpB":"Groupe B: (|DGP,\"B\")","b_dreset":"Reset groupes (|DRESET)","l_base":"n° du 1er disque dans |DCAT","disk_help":"Les boutons « Monter A / B » à côté de chaque .dsk tapent <code>|DLOAD,n</code> au clavier du CPC (il doit être à l'invite BASIC, ESP en mode <b>clavier</b>). Pour un groupe : « Groupe » puis « Monter » sur chaque disque (8 max). Le n° affiché suit l'ordre alphabétique du répertoire : vérifiez-le une fois avec |DCAT et corrigez « n° du 1er disque » si besoin.","h_kbd":"Clavier / souris vers la PicoCPC","zone":"Cliquez ici pour capturer le clavier.<br>Case « souris » cochée : le clic verrouille aussi le pointeur (Échap pour le libérer).","l_mouse":"souris","b_cpcreset":"Reset CPC (Ctrl+Maj+Échap)","b_relall":"Tout relâcher","ws_wait":"WS …","ws_ok":"WS ok","ws_off":"WS coupé","rtt_wait":"latence …","rtt":"latence {0} ms (max {1})","l_mode":"Mode USB vers la PicoCPC :","b_kbd":"Clavier","b_mouse":"Souris","mode_now":"Mode actuel : {0}","mode_k":"clavier","mode_m":"souris","m_rebk":"Redémarrage en mode clavier…","m_rebm":"Redémarrage en mode souris…","hid_note":"Le matériel n'offre qu'une interface HID « boot » à la fois : changer de mode redémarre l'ESP32 (la PicoCPC met ~10 s à le reconnaître).","h_type":"Taper / coller du texte sur le CPC","ph_text":"Collez ici un listing BASIC ou des commandes ; ils seront tapés touche par touche sur le CPC (qui doit être à l'invite).","b_type":"Taper","b_stop":"Arrêter","l_layout":"Disposition PicoCPC","l_speed":"Vitesse","sp_slow":"lente","sp_norm":"normale","sp_fast":"rapide","b_test":"Test : taper |DCAT","layout_help":"« Disposition » = valeur <code>KeyboardLang</code> de PicoCPC.cfg ([USB]) : <b>FR</b> = clavier français (| par AltGr+6), <b>FR_PIPE</b> = variante où la touche ù donne |, <b>EN</b> = clavier US. Si le A devient Q, choisissez FR. Ctrl+V dans la zone de capture colle aussi. Caractères sans touche équivalente (accents…) : ignorés et signalés.","pos_note":"Les touches sont envoyées par position physique : la PicoCPC applique sa propre disposition (KEYBOARDLANG).","typing":"Saisie en cours… {0} %","done":"Terminé.","ws_nc":"WebSocket non connecté","skipped":"Ignorés : {0}","stopped":"Arrêté.","h_ble":"Bluetooth (clavier / souris)","b_scan":"Rechercher","pin_lbl":"Code d'appairage clavier :","ble_none":"Aucun appareil. Mettez le clavier/la souris en mode appairage puis « Rechercher ».","ble_nonamee":"(sans nom)","ble_nothid":"non HID","ble_conn":"connecté","ble_bonded":"appairé","b_connect":"Connecter","b_forget":"Oublier","h_wifi":"Wi-Fi","wifi_sta":"Connecté au réseau « {0} »","wifi_ap":"Point d’accès « {0} » (aucun réseau configuré ou connexion échouée)","wifi_ip":"IP {0}","ph_ssid":"Nom du réseau (SSID)","ph_pass":"Mot de passe","b_wscan":"Chercher les réseaux","b_wsave":"Enregistrer et redémarrer","wifi_help":"SSID vide = retour au point d'accès de l'ESP32. Après redémarrage, rouvrez <b>http://picocpc.local/</b> (ou l'IP affichée sur le moniteur série).","wifi_scanning":"Recherche… (quelques secondes)","wifi_found":"{0} réseau(x) trouvé(s) : choisissez-en un dans la liste du champ.","c_wifi":"Enregistrer et redémarrer l’ESP32 ?","wifi_saved":"Enregistré. Redémarrage en cours : cette page va se couper.","h_fw":"Mise à jour du firmware","fw_ver":"Version installée :","b_fwgo":"Mettre à jour","fw_help":"Choisissez le fichier <b>firmware.bin</b> (dossier <code>release/</code> ou <code>.pio/build/picocpc_bridge/</code>), pas le fichier « complet ». Les fichiers de la clé et le Wi-Fi sont conservés. L'ESP32 redémarre puis la PicoCPC le reconnaît après ~10 s.","fw_choose":"Choisissez d'abord un fichier firmware.bin","fw_confirm":"Installer {0} ({1} Ko) ? L'ESP32 va redémarrer.","fw_sending":"Envoi…","fw_sent":"Envoi… {0} %","fw_ok":"Firmware installé. Redémarrage…","fw_fail":"Échec : {0}","fw_lost":"Connexion perdue (si l'ESP32 redémarre, rechargez la page dans quelques secondes).","h_log":"Journal USB","b_lclear":"Effacer","log_note":"Ce que la PicoCPC demande à la clé (80 dernières lignes)."},"en":{"hdr_title":"PicoCPC · USB Bridge","c_usb_ok":"USB: PicoCPC connected","c_usb_ko":"USB: not connected","c_pend":"changes pending","u_o":"B","u_k":"KB","u_m":"MB","h_usb":"Virtual USB stick","space":"{0} free of {1} ({2} item(s))","drop":"Drop files here or click to choose them","th_name":"Name","th_size":"Size","b_remount":"Remount the stick now","b_format":"Format…","t_remount":"The PicoCPC sees the stick removed then reinserted (keyboard/mouse cut for ~2 s).","no_files":"No files","dsk_guess":"assumed |DCAT no.: {0}","alias":"CPC alias: {0}","b_rename":"Rename","b_delete":"Delete","p_rename":"New name:","c_delete":"Delete {0}?","b_mountA":"Mount A","upl_done":"Upload complete.","net_err":"network error","c_format":"Erase ALL files on the stick?","m_remount":"Remounting…","h_disk":"CPC virtual floppies (|DLOAD, |DEJECT, |DGP)","b_ejA":"Eject A:","b_ejB":"Eject B:","b_gpA":"Group A: (|DGP)","b_gpB":"Group B: (|DGP,\"B\")","b_dreset":"Reset groups (|DRESET)","l_base":"No. of the 1st disk in |DCAT","disk_help":"The « Mount A / B » buttons next to each .dsk type <code>|DLOAD,n</code> on the CPC keyboard (it must be at the BASIC prompt, ESP in <b>keyboard</b> mode). For a group: « Group » then « Mount » on each disk (8 max). The number shown follows the alphabetical order of the directory: check it once with |DCAT and adjust « No. of the 1st disk » if needed.","h_kbd":"Keyboard / mouse to the PicoCPC","zone":"Click here to capture the keyboard.<br>With « mouse » checked, the click also locks the pointer (Esc to release it).","l_mouse":"mouse","b_cpcreset":"Reset CPC (Ctrl+Shift+Esc)","b_relall":"Release all","ws_wait":"WS …","ws_ok":"WS ok","ws_off":"WS disconnected","rtt_wait":"latency …","rtt":"latency {0} ms (max {1})","l_mode":"USB mode toward the PicoCPC:","b_kbd":"Keyboard","b_mouse":"Mouse","mode_now":"Current mode: {0}","mode_k":"keyboard","mode_m":"mouse","m_rebk":"Restarting in keyboard mode…","m_rebm":"Restarting in mouse mode…","hid_note":"The hardware offers only one « boot » HID interface at a time: changing mode restarts the ESP32 (the PicoCPC takes ~10 s to recognise it).","h_type":"Type / paste text on the CPC","ph_text":"Paste a BASIC listing or commands here; they will be typed key by key on the CPC (which must be at the prompt).","b_type":"Type","b_stop":"Stop","l_layout":"PicoCPC layout","l_speed":"Speed","sp_slow":"slow","sp_norm":"normal","sp_fast":"fast","b_test":"Test: type |DCAT","layout_help":"« Layout » = the <code>KeyboardLang</code> value in PicoCPC.cfg ([USB]): <b>FR</b> = French keyboard (| via AltGr+6), <b>FR_PIPE</b> = variant where the ù key gives |, <b>EN</b> = US keyboard. If A turns into Q, choose FR. Ctrl+V in the capture zone also pastes. Characters with no matching key (accents…): skipped and reported.","pos_note":"Keys are sent by physical position: the PicoCPC applies its own layout (KEYBOARDLANG).","typing":"Typing… {0} %","done":"Done.","ws_nc":"WebSocket not connected","skipped":"Skipped: {0}","stopped":"Stopped.","h_ble":"Bluetooth (keyboard / mouse)","b_scan":"Scan","pin_lbl":"Keyboard pairing code:","ble_none":"No device. Put the keyboard/mouse in pairing mode, then press « Scan ».","ble_nonamee":"(no name)","ble_nothid":"not HID","ble_conn":"connected","ble_bonded":"paired","b_connect":"Connect","b_forget":"Forget","h_wifi":"Wi-Fi","wifi_sta":"Connected to network « {0} »","wifi_ap":"Access point « {0} » (no network configured or connection failed)","wifi_ip":"IP {0}","ph_ssid":"Network name (SSID)","ph_pass":"Password","b_wscan":"Find networks","b_wsave":"Save and restart","wifi_help":"Empty SSID = back to the ESP32 access point. After the restart, reopen <b>http://picocpc.local/</b> (or the IP shown on the serial monitor).","wifi_scanning":"Scanning… (a few seconds)","wifi_found":"{0} network(s) found: pick one from the field's list.","c_wifi":"Save and restart the ESP32?","wifi_saved":"Saved. Restarting: this page will disconnect.","h_fw":"Firmware update","fw_ver":"Installed version:","b_fwgo":"Update","fw_help":"Choose the <b>firmware.bin</b> file (<code>release/</code> folder or <code>.pio/build/picocpc_bridge/</code>), not the « full » file. The stick files and Wi-Fi settings are kept. The ESP32 restarts, then the PicoCPC recognises it after ~10 s.","fw_choose":"Choose a firmware.bin file first","fw_confirm":"Install {0} ({1} KB)? The ESP32 will restart.","fw_sending":"Uploading…","fw_sent":"Uploading… {0} %","fw_ok":"Firmware installed. Restarting…","fw_fail":"Failed: {0}","fw_lost":"Connection lost (if the ESP32 is restarting, reload the page in a few seconds).","h_log":"USB log","b_lclear":"Clear","log_note":"What the PicoCPC asks of the stick (last 80 lines)."},"es":{"hdr_title":"PicoCPC · Puente USB","c_usb_ok":"USB: PicoCPC conectada","c_usb_ko":"USB: no conectado","c_pend":"cambios pendientes","u_o":"B","u_k":"KB","u_m":"MB","h_usb":"Llave USB virtual","space":"{0} libres de {1} ({2} elemento(s))","drop":"Suelte archivos aquí o haga clic para elegirlos","th_name":"Nombre","th_size":"Tamaño","b_remount":"Remontar la llave ahora","b_format":"Formatear…","t_remount":"La PicoCPC ve la llave retirada y vuelta a insertar (teclado/ratón cortados ~2 s).","no_files":"Ningún archivo","dsk_guess":"n.º |DCAT supuesto: {0}","alias":"alias CPC: {0}","b_rename":"Renombrar","b_delete":"Eliminar","p_rename":"Nuevo nombre:","c_delete":"¿Eliminar {0}?","b_mountA":"Montar A","upl_done":"Envío terminado.","net_err":"error de red","c_format":"¿Borrar TODOS los archivos de la llave?","m_remount":"Remontando…","h_disk":"Disquetes virtuales CPC (|DLOAD, |DEJECT, |DGP)","b_ejA":"Expulsar A:","b_ejB":"Expulsar B:","b_gpA":"Grupo A: (|DGP)","b_gpB":"Grupo B: (|DGP,\"B\")","b_dreset":"Reiniciar grupos (|DRESET)","l_base":"N.º del 1.er disco en |DCAT","disk_help":"Los botones « Montar A / B » junto a cada .dsk teclean <code>|DLOAD,n</code> en el teclado del CPC (debe estar en el prompt de BASIC, ESP en modo <b>teclado</b>). Para un grupo: « Grupo » y luego « Montar » en cada disco (8 máx.). El número mostrado sigue el orden alfabético del directorio: compruébelo una vez con |DCAT y ajuste « N.º del 1.er disco » si hace falta.","h_kbd":"Teclado / ratón hacia la PicoCPC","zone":"Haga clic aquí para capturar el teclado.<br>Con « ratón » marcado, el clic también bloquea el puntero (Esc para liberarlo).","l_mouse":"ratón","b_cpcreset":"Reset CPC (Ctrl+Mayús+Esc)","b_relall":"Soltar todo","ws_wait":"WS …","ws_ok":"WS ok","ws_off":"WS cortado","rtt_wait":"latencia …","rtt":"latencia {0} ms (máx {1})","l_mode":"Modo USB hacia la PicoCPC:","b_kbd":"Teclado","b_mouse":"Ratón","mode_now":"Modo actual: {0}","mode_k":"teclado","mode_m":"ratón","m_rebk":"Reiniciando en modo teclado…","m_rebm":"Reiniciando en modo ratón…","hid_note":"El hardware solo ofrece una interfaz HID « boot » a la vez: cambiar de modo reinicia el ESP32 (la PicoCPC tarda ~10 s en reconocerlo).","h_type":"Escribir / pegar texto en el CPC","ph_text":"Pegue aquí un listado BASIC o comandos; se teclearán tecla a tecla en el CPC (que debe estar en el prompt).","b_type":"Teclear","b_stop":"Detener","l_layout":"Distribución PicoCPC","l_speed":"Velocidad","sp_slow":"lenta","sp_norm":"normal","sp_fast":"rápida","b_test":"Prueba: teclear |DCAT","layout_help":"« Distribución » = valor <code>KeyboardLang</code> de PicoCPC.cfg ([USB]): <b>FR</b> = teclado francés (| con AltGr+6), <b>FR_PIPE</b> = variante donde la tecla ù da |, <b>EN</b> = teclado US. Si la A se convierte en Q, elija FR. Ctrl+V en la zona de captura también pega. Caracteres sin tecla equivalente (acentos…): omitidos y señalados.","pos_note":"Las teclas se envían por posición física: la PicoCPC aplica su propia distribución (KEYBOARDLANG).","typing":"Escribiendo… {0} %","done":"Terminado.","ws_nc":"WebSocket no conectado","skipped":"Omitidos: {0}","stopped":"Detenido.","h_ble":"Bluetooth (teclado / ratón)","b_scan":"Buscar","pin_lbl":"Código de emparejamiento del teclado:","ble_none":"Ningún dispositivo. Ponga el teclado/ratón en modo de emparejamiento y pulse « Buscar ».","ble_nonamee":"(sin nombre)","ble_nothid":"no HID","ble_conn":"conectado","ble_bonded":"emparejado","b_connect":"Conectar","b_forget":"Olvidar","h_wifi":"Wi-Fi","wifi_sta":"Conectado a la red « {0} »","wifi_ap":"Punto de acceso « {0} » (ninguna red configurada o conexión fallida)","wifi_ip":"IP {0}","ph_ssid":"Nombre de la red (SSID)","ph_pass":"Contraseña","b_wscan":"Buscar redes","b_wsave":"Guardar y reiniciar","wifi_help":"SSID vacío = vuelta al punto de acceso del ESP32. Tras el reinicio, vuelva a abrir <b>http://picocpc.local/</b> (o la IP que muestra el monitor serie).","wifi_scanning":"Buscando… (unos segundos)","wifi_found":"{0} red(es) encontrada(s): elija una de la lista del campo.","c_wifi":"¿Guardar y reiniciar el ESP32?","wifi_saved":"Guardado. Reiniciando: esta página se cortará.","h_fw":"Actualización del firmware","fw_ver":"Versión instalada:","b_fwgo":"Actualizar","fw_help":"Elija el archivo <b>firmware.bin</b> (carpeta <code>release/</code> o <code>.pio/build/picocpc_bridge/</code>), no el archivo « completo ». Se conservan los archivos de la llave y el Wi-Fi. El ESP32 se reinicia y la PicoCPC lo reconoce tras ~10 s.","fw_choose":"Elija primero un archivo firmware.bin","fw_confirm":"¿Instalar {0} ({1} KB)? El ESP32 se reiniciará.","fw_sending":"Enviando…","fw_sent":"Enviando… {0} %","fw_ok":"Firmware instalado. Reiniciando…","fw_fail":"Error: {0}","fw_lost":"Conexión perdida (si el ESP32 se está reiniciando, recargue la página en unos segundos).","h_log":"Registro USB","b_lclear":"Borrar","log_note":"Lo que la PicoCPC pide a la llave (últimas 80 líneas)."}};
const SRV=[[/^(.*) - utilisez "Formater"$/,"$1 - use \"Format\"","$1 - use \"Formatear\""],[/^lecture du secteur 0 impossible$/,"cannot read sector 0","no se puede leer el sector 0"],[/^pas de signature FAT \(disque non formate \?\)$/,"no FAT signature (unformatted disk?)","sin firma FAT (¿disco sin formatear?)"],[/^volume non FAT16 \(reformatez-le\)$/,"volume is not FAT16 (reformat it)","el volumen no es FAT16 (reformatéelo)"],[/^geometrie FAT incoherente$/,"inconsistent FAT geometry","geometría FAT incoherente"],[/^partition trop petite pour FAT16$/,"partition too small for FAT16","partición demasiado pequeña para FAT16"],[/^ecriture du formatage impossible$/,"cannot write the format","no se puede escribir el formato"],[/^lecture du repertoire impossible$/,"cannot read the directory","no se puede leer el directorio"],[/^impossible de generer un nom court$/,"cannot generate a short name","no se puede generar un nombre corto"],[/^fichier introuvable$/,"file not found","archivo no encontrado"],[/^suppression de dossier non geree$/,"deleting folders is not supported","no se admite eliminar carpetas"],[/^renommage de dossier non gere$/,"renaming folders is not supported","no se admite renombrar carpetas"],[/^nom invalide$/,"invalid name","nombre no válido"],[/^nom de fichier invalide$/,"invalid file name","nombre de archivo no válido"],[/^ce nom existe deja$/,"this name already exists","este nombre ya existe"],[/^un dossier porte deja ce nom$/,"a folder already has this name","ya hay una carpeta con este nombre"],[/^repertoire racine plein$/,"root directory full","directorio raíz lleno"],[/^ecriture repertoire$/,"directory write failed","fallo al escribir el directorio"],[/^disque plein$/,"disk full","disco lleno"],[/^ecriture flash$/,"flash write failed","fallo de escritura en flash"],[/^pas d'ecriture en cours$/,"no write in progress","ninguna escritura en curso"],[/^confirmation manquante$/,"missing confirmation","falta la confirmación"],[/^SSID 32 caracteres max ; mot de passe de 8 a 63 caracteres$/,"SSID 32 characters max; password 8 to 63 characters","SSID de 32 caracteres máx.; contraseña de 8 a 63 caracteres"],[/^ce n'est pas un firmware ESP32 \(utilisez firmware\.bin, pas le fichier complet\)$/,"not an ESP32 firmware (use firmware.bin, not the full file)","no es un firmware ESP32 (use firmware.bin, no el archivo completo)"],[/^firmware prevu pour une autre puce que l'ESP32-S3$/,"firmware built for a chip other than the ESP32-S3","firmware para un chip distinto del ESP32-S3"],[/^ecriture flash : (.*)$/,"flash write: $1","escritura en flash: $1"],[/^verification : (.*)$/,"verification: $1","verificación: $1"],[/^Update\.begin : (.*)$/,"Update.begin: $1","Update.begin: $1"],[/^envoi annule$/,"upload cancelled","envío cancelado"],[/^echec de la mise a jour$/,"update failed","fallo de la actualización"],[/^erreur réseau$/,"network error","error de red"],[/^Prêt$/,"Ready","Listo"],[/^Recherche en cours…$/,"Scanning…","Buscando…"],[/^Recherche terminée \((\d+) appareil\(s\)\)$/,"Scan finished ($1 device(s))","Búsqueda terminada ($1 dispositivo(s))"],[/^Appareil inconnu : relancez une recherche$/,"Unknown device: scan again","Dispositivo desconocido: vuelva a buscar"],[/^Appareil inconnu$/,"Unknown device","Dispositivo desconocido"],[/^Appairage supprimé$/,"Pairing removed","Emparejamiento eliminado"],[/^Connexion à (.*)…$/,"Connecting to $1…","Conectando con $1…"],[/^Échec de connexion à (.*)$/,"Connection to $1 failed","Fallo de conexión con $1"],[/^Appairage de (.*) \(code (\d+) si demandé\)…$/,"Pairing with $1 (code $2 if asked)…","Emparejando con $1 (código $2 si se pide)…"],[/^(.*) n'est pas un périphérique HID$/,"$1 is not an HID device","$1 no es un dispositivo HID"],[/^Aucun rapport HID utilisable sur (.*)$/,"No usable HID report on $1","Ningún informe HID utilizable en $1"],[/^Connecté : (.*)$/,"Connected: $1","Conectado: $1"],[/^Déconnecté : (.*)$/,"Disconnected: $1","Desconectado: $1"]];
let LANG='fr';
try{LANG=localStorage.lang||''}catch(e){}
if(!L[LANG]){const n=(navigator.language||'fr').slice(0,2);LANG=L[n]?n:'en'}
const _t=(k,...a)=>{let s=(L[LANG][k]!==undefined?L[LANG][k]:(L.fr[k]!==undefined?L.fr[k]:k));a.forEach((v,i)=>{s=s.split('{'+i+'}').join(v)});return s};
function srv(m){m=String(m);if(LANG==='fr')return m;
 for(const [re,en,es] of SRV){const g=m.match(re);if(g)return (LANG==='en'?en:es).replace(/\$(\d)/g,(_,i)=>srv(g[i]))}
 return m}
let onLang=[];
function applyLang(){document.documentElement.lang=LANG;
 document.querySelectorAll('[data-i]').forEach(e=>e.textContent=_t(e.dataset.i));
 document.querySelectorAll('[data-ih]').forEach(e=>e.innerHTML=_t(e.dataset.ih));
 document.querySelectorAll('[data-ip]').forEach(e=>e.placeholder=_t(e.dataset.ip));
 $('#lang').value=LANG;onLang.forEach(f=>f())}
$('#lang').onchange=()=>{LANG=$('#lang').value;try{localStorage.lang=LANG}catch(e){}applyLang()};
const fmt=n=>n<1024?n+' '+_t('u_o'):n<1048576?(n/1024).toFixed(1)+' '+_t('u_k'):(n/1048576).toFixed(2)+' '+_t('u_m');
async function api(p,o){const r=await fetch(p,o);const t=await r.text();let j={};try{j=JSON.parse(t)}catch(e){}
 if(!r.ok||j.ok===false)throw new Error(srv(j.error||t||('HTTP '+r.status)));return j}
const post=(p,q)=>api(p+(q?'?'+new URLSearchParams(q):''),{method:'POST'});

/* ---------- fichiers ---------- */
async function refresh(){
 try{
  const [st,fl]=await Promise.all([api('/api/status'),api('/api/files')]);
  const used=st.total-st.free;
  $('#spaceTxt').textContent=_t('space',fmt(st.free),fmt(st.total),fl.files.length);
  $('#spaceBar').style.width=(st.total?100*used/st.total:0)+'%';
  const u=$('#cUsb');u.textContent=_t(st.usb?'c_usb_ok':'c_usb_ko');u.className='chip '+(st.usb?'ok':'bad');
  $('#cNet').textContent=st.ip;$('#cPend').hidden=!st.pending;
  const tb=$('#files');tb.innerHTML='';
  if(!fl.files.length){tb.innerHTML='<tr><td colspan=3 class="mut">'+_t('no_files')+'</td></tr>'}
  const key=x=>x.name.toUpperCase();
  fl.files.sort((x,y)=>(y.dir?1:0)-(x.dir?1:0)||(key(x)<key(y)?-1:key(x)>key(y)?1:0));   // ordre alphabétique comme |DCAT
  dskList=fl.files.filter(x=>!x.dir&&/\.dsk$/i.test(x.name)).map(x=>x.name);
  for(const f of fl.files){
   const tr=document.createElement('tr');
   const a=document.createElement('td');
   if(f.dir){a.textContent='['+f.name+']';a.className='mut'}else{const l=document.createElement('a');l.href='/download?name='+encodeURIComponent(f.name);l.textContent=f.name;l.style.color='var(--ac)';a.appendChild(l)}
   if(/\.dsk$/i.test(f.name)){const s2=document.createElement('div');s2.className='mut';s2.textContent=_t('dsk_guess',dskIdx(f.name));a.appendChild(s2)}
   if(f.short&&f.short!==f.name){const s=document.createElement('div');s.className='mut';s.textContent=_t('alias',f.short);a.appendChild(s)}
   const n=document.createElement('td');n.className='n';n.textContent=f.dir?'':fmt(f.size);
   const c=document.createElement('td');c.className='act';
   if(!f.dir){
    const r=document.createElement('button');r.className='sec';r.textContent=_t('b_rename');
    r.onclick=async()=>{const nn=prompt(_t('p_rename'),f.name);if(!nn||nn===f.name)return;try{await post('/api/rename',{from:f.name,to:nn});refresh()}catch(e){msg(e.message)}};
    const d=document.createElement('button');d.className='del';d.textContent=_t('b_delete');d.style.marginLeft='6px';
    d.onclick=async()=>{if(!confirm(_t('c_delete',f.name)))return;try{await post('/api/delete',{name:f.name});refresh()}catch(e){msg(e.message)}};
    if(/\.dsk$/i.test(f.name)){
     const id=dskIdx(f.name);
     const ma=document.createElement('button'),mb=document.createElement('button');
     ma.className=mb.className='sec';ma.textContent=_t('b_mountA');mb.textContent='B';ma.style.marginRight='6px';mb.style.marginRight='6px';
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
 x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}x.status==200?res():rej(new Error(file.name+' : '+(j.error?srv(j.error):x.status)))};
 x.onerror=()=>rej(new Error(file.name+' : '+_t('net_err')));x.send(fd)})}
async function upAll(list){
 const fl=[...list];let i=0;
 try{for(const f of fl){i++;await up(f,'('+i+'/'+fl.length+')')}$('#upl').textContent=_t('upl_done');msg('')}
 catch(e){$('#upl').textContent='';msg(e.message)}
 refresh()}
const drop=$('#drop');
drop.onclick=()=>$('#pick').click();$('#pick').onchange=e=>{upAll(e.target.files);e.target.value=''};
drop.ondragover=e=>{e.preventDefault();drop.classList.add('over')};drop.ondragleave=()=>drop.classList.remove('over');
drop.ondrop=e=>{e.preventDefault();drop.classList.remove('over');upAll(e.dataTransfer.files)};
$('#fwGo').onclick=()=>{
  const f=$('#fwFile').files[0],m=$('#fwMsg');
  if(!f){m.textContent=_t('fw_choose');return}
  if(!confirm(_t('fw_confirm',f.name,Math.round(f.size/1024))))return;
  const fd=new FormData();fd.append('fw',f,f.name);
  const x=new XMLHttpRequest();x.open('POST','/api/fwupdate');
  $('#fwBar').style.display='block';$('#fwGo').disabled=true;m.textContent=_t('fw_sending');
  x.upload.onprogress=e=>{if(e.lengthComputable){$('#fwFill').style.width=(100*e.loaded/e.total)+'%';m.textContent=_t('fw_sent',Math.round(100*e.loaded/e.total))}};
  x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}
    if(x.status==200&&j.ok){m.textContent=_t('fw_ok');setTimeout(()=>location.reload(),9000)}
    else{m.textContent=_t('fw_fail',j.error?srv(j.error):x.status);$('#fwGo').disabled=false}};
  x.onerror=()=>{m.textContent=_t('fw_lost');$('#fwGo').disabled=false};
  x.send(fd);
};
api('/api/status').then(s=>{$('#fwVer').textContent=s.fw||'?'}).catch(()=>{});
$('#hKbd').onclick=()=>post('/api/hidmode',{m:1}).then(()=>msg(_t('m_rebk')));
$('#hMouse').onclick=()=>post('/api/hidmode',{m:2}).then(()=>msg(_t('m_rebm')));
let hidMode=1;const updHid=()=>{$('#hidState').textContent=_t('mode_now',_t(hidMode==2?'mode_m':'mode_k'))};
api('/api/status').then(s=>{hidMode=s.hid;updHid()}).catch(()=>{});onLang.push(updHid);
$('#bRemount').onclick=async()=>{try{await post('/api/remount');msg(_t('m_remount'));setTimeout(refresh,2500)}catch(e){msg(e.message)}};
$('#bFormat').onclick=async()=>{if(!confirm(_t('c_format')))return;try{await post('/api/format',{confirm:'1'});refresh()}catch(e){msg(e.message)}};

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
 $('#tProg').textContent=tTotal&&left?_t('typing',Math.round(100*(tTotal-left)/tTotal)):(tTotal?_t('done'):'');
 if(!left)tTotal=0}
function typeText(txt,quiet){
 if(!ws||ws.readyState!==1){$('#tProg').textContent=_t('ws_nc');return}
 const r=textToSteps(txt,$('#kLang').value);
 tCfg=$('#tSpeed').value;
 tq=tq.concat(r.st);tTotal+=r.st.length;tPump();
 if(r.skip.size&&!quiet)$('#tProg').textContent=_t('skipped',[...r.skip].join(' '))}
const PAUSE=n=>[[0xFF,n]];
function cmds(lines,pauseAfterFirst){
 const lang=$('#kLang').value;let steps=[];
 lines.forEach((l,i)=>{steps=steps.concat(textToSteps(l,lang).st,[[0,0x28]]);if(i<lines.length-1)steps=steps.concat(PAUSE(pauseAfterFirst||40))});
 if(!ws||ws.readyState!==1){$('#tProg').textContent=_t('ws_nc');return}
 tCfg=$('#tSpeed').value;tq=tq.concat(steps);tTotal+=steps.length;tPump()}
const base=()=>parseInt($("#dBase").value||"0",10);
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
$('#tStop').onclick=()=>{tq=[];espRem=0;tTotal=0;send('x');$('#tProg').textContent=_t('stopped')};
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
 ws.onopen=()=>{wsState.textContent=_t('ws_ok');wsState.className='chip ok';espRem=0};
 ws.onmessage=e=>{const d=String(e.data);
  if(d[0]==='t'){espRem=parseInt(d.slice(2),10)||0;tPump()}
  else if(d[0]==='P'){const r=performance.now()-parseFloat(d.slice(2));rtts.push(r);if(rtts.length>10)rtts.shift();
   const a=rtts.reduce((x,y)=>x+y,0)/rtts.length;$('#rtt').textContent=_t('rtt',Math.round(a),Math.round(Math.max(...rtts)))}};
 ws.onclose=()=>{wsState.textContent=_t('ws_off');wsState.className='chip bad';setTimeout(wsc,1500)};}
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
 try{const b=await api('/api/ble');$('#pin').textContent=b.pin;$('#bmsg').textContent=srv(b.status);$('#bScan').disabled=b.scanning;
  const tb=$('#ble');tb.innerHTML='';
  if(!b.devices.length)tb.innerHTML='<tr><td class="mut">'+_t('ble_none')+'</td></tr>';
  for(const d of b.devices){
   const tr=document.createElement('tr'),a=document.createElement('td'),c=document.createElement('td');c.className='act';
   a.textContent=(d.name||_t('ble_noname'))+'  ';const s=document.createElement('span');s.className='mut';
   s.textContent=d.addr+(d.rssi!==undefined?' · '+d.rssi+' dBm':'')+(d.kind?' · '+d.kind:'')+(d.hid?'':' · '+_t('ble_nothid'));a.appendChild(s);
   if(d.connected){const t=document.createElement('span');t.className='chip ok';t.textContent=_t('ble_conn');t.style.marginLeft='6px';a.appendChild(t)}
   else if(d.bonded){const t=document.createElement('span');t.className='chip';t.textContent=_t('ble_bonded');t.style.marginLeft='6px';a.appendChild(t)}
   if(!d.connected){const x=document.createElement('button');x.textContent=_t('b_connect');x.onclick=async()=>{await post('/api/ble/connect',{addr:d.addr});setTimeout(bleRefresh,500)};c.appendChild(x)}
   if(d.bonded||d.connected){const f=document.createElement('button');f.className='del';f.textContent=_t('b_forget');f.style.marginLeft='6px';f.onclick=async()=>{await post('/api/ble/forget',{addr:d.addr});setTimeout(bleRefresh,500)};c.appendChild(f)}
   tr.append(a,c);tb.appendChild(tr)}
 }catch(e){}}
$('#bScan').onclick=async()=>{await post('/api/ble/scan');setTimeout(bleRefresh,300)};

/* ---------- wifi ---------- */
async function wifiLoad(){try{const w=await api('/api/wifi');
 $('#wInfo').textContent=(w.mode==='STA'?_t('wifi_sta',w.ssid):_t('wifi_ap',w.ap))+' · '+_t('wifi_ip',w.ip);
 $('#wSsid').value=w.ssid||''}catch(e){}}
$('#wScan').onclick=async()=>{$('#wMsg').textContent=_t('wifi_scanning');
 try{const r=await api('/api/wifi/scan');const dl=$('#wList');dl.innerHTML='';
  for(const n of r.nets){const o=document.createElement('option');o.value=n.ssid;o.label=n.ssid+' ('+n.rssi+' dBm)';dl.appendChild(o)}
  $('#wMsg').textContent=_t('wifi_found',r.nets.length)}catch(e){$('#wMsg').textContent=e.message}};
$('#wSave').onclick=async()=>{
 if(!confirm(_t('c_wifi')))return;
 try{await post('/api/wifi/save',{ssid:$('#wSsid').value,pass:$('#wPass').value});
  $('#wMsg').textContent=_t('wifi_saved')}catch(e){$('#wMsg').textContent=e.message}};

/* ---------- journal ---------- */
async function logLoad(){try{const r=await fetch('/api/log');const t=await r.text();const p=$('#log');const bottom=p.scrollTop+p.clientHeight>=p.scrollHeight-8;p.textContent=t;if(bottom)p.scrollTop=p.scrollHeight}catch(e){}}
$('#lClear').onclick=async()=>{await post('/api/log/clear');logLoad()};
setInterval(logLoad,2000);logLoad();

onLang.push(()=>{refresh();bleRefresh();wifiLoad()});
wsc();applyLang();
setInterval(refresh,5000);setInterval(bleRefresh,2000);
</script></body></html>
)HTMLPAGE";
