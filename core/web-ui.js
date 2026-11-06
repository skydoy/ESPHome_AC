// ESPHome_AC web_server v3 add-ons (the only js_include allowed):
//  1) cap the debug log panel height (style injected into esp-log's shadowRoot);
//  2) live AC diagram (outdoor + indoor unit, refrigerant pipes) above the entity list.
// Values come from the page's /events stream. Language: browser default, PT/EN toggle
// saved in localStorage. PT-PT texts use "tu".
(function () {
  function root() {
    var a = document.querySelector("esp-app");
    return a && a.shadowRoot;
  }

  // --- 1) Debug log height ---
  function fixLog() {
    var sr = root(), log = sr && sr.querySelector("esp-log"), lr = log && log.shadowRoot;
    if (!lr) return false;
    if (!lr.getElementById("web-log-css")) {
      var st = document.createElement("style");
      st.id = "web-log-css";
      st.textContent = ".logs{max-height:min(45vh,420px);overflow-y:auto;overscroll-behavior:contain}";
      lr.appendChild(st);
    }
    return true;
  }

  
  // --- 2) AC diagram ---
;
  // Web UI entity names (exact English name -> PT-PT). English is restored on EN.
  var ENT_PT = {
    "Air Conditioner": "Ar condicionado",
    "Outdoor Temperature": "Temperatura exterior",
    "Indoor Coil Temperature": "Temperatura da bateria interior",
    "Outdoor Coil Temperature": "Temperatura da bateria exterior",
    "Compressor Frequency": "Frequência do compressor",
    "Compressor Target Frequency": "Frequência alvo do compressor",
    "Indoor Humidity": "Humidade interior",
    "Supply Voltage": "Tensão de alimentação",
    "Power": "Potência",
    "Current": "Corrente",
    "AC Link": "Ligação ao AC",
    "AC Fault": "Falha do AC",
    "AC Fault Codes": "Códigos de falha do AC",
    "AC Link Status": "Estado da ligação ao AC",
    "AC Firmware Version": "Versão do firmware do AC",
    "AC Bus Checksum Errors": "Erros de checksum do barramento do AC",
    "AC Bus Framing Errors": "Erros de trama do barramento do AC",
    "Humidity Set Point": "Alvo de humidade",
    "AC Panel Unit": "Unidade do painel do AC",
    "AC Capabilities": "Capacidades do AC",
    "AC Device Type": "Tipo de dispositivo do AC",
    "Auto report": "Relatório automático",
    "AC Identity": "Identidade do AC",
    "Auxiliary Heater": "Aquecimento auxiliar",
    "AC Display": "Ecrã do AC",
    "AC Beep": "Som do AC",
    "AC Frame Logging": "Registo de tramas do AC",
    "Fan Mute": "Silenciar ventoinha",
    "Indoor Temperature Sensor Failure": "Falha do sensor de temperatura interior",
    "Indoor Coil Temperature Sensor Failure": "Falha do sensor da bateria interior",
    "Indoor Humidity Sensor Failure": "Falha do sensor de humidade interior",
    "Water Tank Full": "Depósito de água cheio",
    "Indoor Fan Motor Failure": "Falha do motor da ventoinha interior",
    "Cabinet Grille Protection Alarm": "Alarme de proteção da grelha",
    "Indoor Voltage Zero-Crossing Fault": "Falha de passagem por zero da tensão interior",
    "Indoor-Outdoor Communication Failure": "Falha de comunicação interior-exterior",
    "Indoor Board-Display Communication Failure": "Falha de comunicação placa-ecrã da unidade interior",
    "Indoor Board-Keypad Communication Failure": "Falha de comunicação placa-teclado da unidade interior",
    "WiFi-Indoor Communication Failure": "Falha de comunicação WiFi-unidade interior",
    "Electrical Fault": "Falha elétrica",
    "Indoor Control Board EEPROM Error": "Erro de EEPROM da placa de controlo interior",
    "Outdoor EEPROM Error": "Erro de EEPROM da unidade exterior",
    "Outdoor Coil Temperature Sensor Failure": "Falha do sensor da bateria exterior",
    "Exhaust Temperature Sensor Failure": "Falha do sensor de temperatura de descarga",
    "Outdoor Ambient Temperature Sensor Failure": "Falha do sensor de temperatura ambiente exterior",
    "Overheat or Overcool Protection": "Proteção contra sobreaquecimento ou subarrefecimento",
    "High Discharge Temperature Protection": "Proteção contra temperatura de descarga elevada",
    "Firmware Version": "Versão do firmware",
    "ESPHome Version": "Versão do ESPHome",
    "Latest Version": "Última versão",
    "Firmware Update": "Atualização do firmware",
    "Sleep Mode": "Modo de sono",
    "Light On": "Ligar luz",
    "Light Off": "Desligar luz",
    "Eco": "Eco",
    "Super": "Super",
    "Report my AC": "Reportar o meu AC",
    "Identify AC": "Identificar AC",
    "Energy Today": "Energia hoje",
    "Uptime": "Tempo ligado",
    "Wi-Fi Signal": "Sinal Wi-Fi",
    "Reset Reason": "Motivo do reinício",
    "Restart": "Reiniciar",
    "Restart (Safe Mode)": "Reiniciar (modo seguro)",
    "Update Status": "Estado da atualização",
    "Update Available": "Atualização disponível",
    "Check for Update": "Verificar atualizações",
    "Auto Update": "Atualização automática",
    "Install Update": "Instalar atualização",
    "Latest Version": "Última versão",
  };
  var KEY = "esphome_ac_lang", lang;
  try { lang = localStorage.getItem(KEY); } catch (e) {}
  if (lang !== "pt" && lang !== "en") lang = /^pt/i.test(navigator.language || "") ? "pt" : "en";

  // [pt, en]
  var TXT = {
    title: ["Esquema do ar condicionado", "Air conditioner diagram"],
    sub: ["Valores em direto do teu AC", "Live values from your AC"],
    outdoor: ["Unidade exterior", "Outdoor unit"],
    indoor: ["Unidade interior", "Indoor unit"],
    tout: ["Ar exterior", "Outdoor air"],
    tocoil: ["Bateria ext.", "Outdoor coil"],
    freq: ["Compressor", "Compressor"],
    pwr: ["Potência", "Power"],
    troom: ["Divisão", "Room"],
    ttgt: ["Alvo", "Target"],
    ticoil: ["Bateria int.", "Indoor coil"],
    hum: ["Humidade", "Humidity"],
    legHot: ["Linha quente", "Hot line"],
    legCold: ["Linha fria", "Cold line"],
    m_off: ["Desligado", "Off"], m_cool: ["Arrefecer", "Cooling"], m_heat: ["Aquecer", "Heating"],
    m_dry: ["Desumidificar", "Drying"], m_fan_only: ["Ventilação", "Fan only"],
    m_heat_cool: ["Automático", "Auto"], m_auto: ["Automático", "Auto"],
    other: ["A trabalhar para outra unidade", "Running for another unit"],
    lang: ["Mudar língua (PT/EN)", "Switch language (PT/EN)"]
  };
  function t(k) { var p = TXT[k]; return p ? p[lang === "pt" ? 0 : 1] : k; }

  // Normalised entity name (domain prefix stripped) -> [chip key, unit, decimals]
  var MAP = {
    outdoortemperature: ["tout", "°C", 1],
    outdoorcoiltemperature: ["tocoil", "°C", 1],
    compressorfrequency: ["freq", "Hz", 0],
    compressortargetfrequency: ["ftgt", "Hz", 0],
    power: ["pwr", "W", 0],
    indoorcoiltemperature: ["ticoil", "°C", 1],
    indoorhumidity: ["hum", "%", 0],
    // Fallback room temperature when the climate entity does not publish it while OFF.
    roomtemperature: ["troom", "°C", 1],indoortemperature: ["troom", "°C", 1],ambienttemperature: ["troom", "°C", 1]
  };
  var vals = {}, mode = "", action = "", outdoor = { freq: null, pwr: null };

  var OPT_PT = {
    OFF: "Desligado", HEAT_COOL: "Automático", COOL: "Frio", HEAT: "Aquecer",
    DRY: "Desumidificar", FAN_ONLY: "Ventilação", AUTO: "Auto",
    "1": "1", "2": "2", "3": "3", "4": "4"
  };
  var GROUP_PT = {
    "Sensor and Control": "Sensores e controlo", Configuration: "Configuração",
    Diagnostic: "Diagnóstico", Undefined: "Outros"
  };
  var LABEL_PT = { Current: "Atual", Target: "Alvo", Mode: "Modo" };

  // Translate esp-entity-table without touching option values or REST action URLs.
  // entity.name is the source string; the observer keeps new/re-rendered rows translated.
  function tableEl() {
    var sr = root(), t = sr && sr.querySelector("esp-entity-table");
    return t && t.shadowRoot ? t : null;
  }
  // Content translations for common state strings. Keep option values and REST unchanged.
  function uiString(v) {
    if (lang !== "pt") return v;
    var m = /^New version (.+) available, install from Device Builder$/.exec(v);
    if (m) return "Versão nova " + m[1] + " disponível, instala no Device Builder";
    if (/^New version .+ available$/.test(v)) return v.replace("New version ", "Versão nova disponível: ").replace(" available", "");
    m = /^Up to date \((.+)\)$/.exec(v);
    if (m) return "Atualizado (" + m[1] + ")";
    m = /^Update available: (.+)$/.exec(v);
    if (m) return "Atualização disponível: " + m[1];
    var CONTENT_PT = {
      "Checking for updates": "A verificar atualizações",
      "Install requested": "Instalação solicitada",
      "Downloading firmware (do not unplug)": "A transferir o firmware (não desligues)",
      "Installed, rebooting": "Instalado, a reiniciar",
      "Check failed (no internet)": "Falha na verificação (sem internet)"
    };
    if (CONTENT_PT[v]) return CONTENT_PT[v];
    if (/^Update failed \(/.test(v)) return v.replace("Update failed", "Falha na atualização");
    if (/^Invalid manifest$/.test(v)) return "Manifesto inválido";
    return v;
  }

  function translateTable() {
    var tb = tableEl();
    if (!tb) return false;
    var sr = tb.shadowRoot;
    // 1) Entity names: text slot inside each .entity-row (the second div, after the icon).
    if (tb.entities) {
      var rows = sr.querySelectorAll(".entity-row");
      for (var i = 0; i < tb.entities.length && i < rows.length; i++) {
        var e = tb.entities[i], box = rows[i].children && rows[i].children[1];
        if (!box) continue;
        var tn = null;
        for (var c = 0; c < box.childNodes.length; c++)
          if (box.childNodes[c].nodeType === 3 && box.childNodes[c].nodeValue.trim()) tn = box.childNodes[c];
        if (tn) {
          if (tn.__en === undefined) tn.__en = tn.nodeValue.trim();
          var en = tn.__en, pt = ENT_PT[en];
          tn.nodeValue = lang === "pt" && pt ? pt : en;
        }
      }
    }
    // 2) Dynamic text states (e.g. update status) inside entity action/value area.
    sr.querySelectorAll(".entity-row").forEach(function (row) {
      var nameEl = row.children && row.children[1], name = nameEl && nameEl.textContent;
      if (!name) return;
      var wanted = lang === "pt" ? "Estado da atualização" : "Update Status";
      if (name.trim().replace(/^\[[^\]]+\]\s*/, "").trim() !== wanted) return;
      var walker = document.createTreeWalker(row, NodeFilter.SHOW_TEXT), nodes = [], n;
      while ((n = walker.nextNode())) nodes.push(n);
      nodes.forEach(function (node) {
        var txt = node.nodeValue.trim();
        if (!txt || node.parentNode === nameEl) return;
        if (node.__statusEn !== undefined) txt = node.__statusEn;
        else if (/^(New version|Up to date|Checking for updates|Install requested|Downloading firmware|Installed, rebooting|Check failed|Update failed|Invalid manifest)/.test(txt)) node.__statusEn = txt;
        if (node.__statusEn !== undefined) node.nodeValue = uiString(node.__statusEn);
      });
    });
    // 2) Group headers, climate labels, option display text.
    sr.querySelectorAll(".tab-header").forEach(function (h) {
      var v = h.textContent.trim();
      Object.keys(GROUP_PT).forEach(function (en) {
        if (lang === "pt" && v === en) h.textContent = GROUP_PT[en];
        if (lang === "en" && v === GROUP_PT[en]) h.textContent = en;
      });
    });
    // Climate current temperature shares its label with the value ("Current: 26.0 °C").
    // Only replace its prefix, never the state value.
    sr.querySelectorAll("label").forEach(function (l) {
      for (var i = 0; i < l.childNodes.length; i++) {
        var n = l.childNodes[i];
        if (n.nodeType !== 3) continue;
        if (lang === "pt") n.nodeValue = n.nodeValue.replace(/Current:\s*/, "Atual: ");
        else n.nodeValue = n.nodeValue.replace(/Atual:\s*/, "Current: ");
      }
      var key = l.textContent.trim().replace(/\s*:$/, "");
      Object.keys(LABEL_PT).forEach(function (en) {
        if (lang === "pt" && key === en) l.textContent = LABEL_PT[en] + " ";
        if (lang === "en" && key === LABEL_PT[en]) l.textContent = en + ": ";
      });
    });
    sr.querySelectorAll("option").forEach(function (o) {
      if (o.__en === undefined) o.__en = o.textContent.trim();
      o.textContent = lang === "pt" ? OPT_PT[o.__en] || o.__en : o.__en;
    });
    // 3) Buttons: visible label only (PRESS).
    sr.querySelectorAll(".abutton,.abuttonIsState").forEach(function (b) {
      var v = b.textContent.trim();
      if (!/^(PRESS|PREMIR|CARREGAR)$/.test(v)) return;
      b.textContent = lang === "pt" ? "CARREGAR" : "PRESS";
    });
    if (!tb.__langObs) {
      tb.__langObs = new MutationObserver(function (recs) {
        if (recs.every(function (r) {
          return r.type === "childList" &&
            (r.addedNodes.length === 0 ? true : Array.prototype.every.call(r.addedNodes, function (n) {
              return n.nodeType !== 1 || n.tagName !== "OPTION";
            }));
        })) return;
        translateTableSoon();
      });
      tb.__langObs.observe(sr, { childList: true, subtree: true });
    }
    return true;
  }
  var tTimer = null;
  function translateTableSoon() {
    if (tTimer) return;
    tTimer = setTimeout(function () { tTimer = null; translateTable(); }, 30);
  }


  function chip(k, x, y, w, cls) {
    return '<g data-c="' + k + '" transform="translate(' + x + "," + y + ')">' +
      '<rect width="' + w + '" height="36" rx="8" class="c' + (cls ? " " + cls : "") + '"/>' +
      '<text x="' + w / 2 + '" y="14" class="cl" data-t="' + k + '"></text>' +
      '<text x="' + w / 2 + '" y="29" class="cv" data-v="' + k + '">—</text></g>';
  }
  var SVG =
    '<svg viewBox="0 0 640 250" role="img">' +
    // pipes: top = gas line, bottom = liquid line
    '<path id="p1" class="pp" d="M200 95 H440"/><path id="p2" class="pp" d="M200 165 H440"/>' +
    '<path id="f1" class="pf" d="M200 95 H440"/><path id="f2" class="pf" d="M200 165 H440"/>' +
    // outdoor unit
    '<g id="uo" transform="translate(10,10)"><rect width="190" height="220" rx="14" class="u"/>' +
    '<text x="95" y="24" class="ut" data-t="outdoor"></text>' +
    // Axial fan (same look as esphome-altherma): shroud, guard rings, spokes, 3-blade rotor.
    '<g class="fa" transform="translate(62,72) scale(.92)"><circle r="38" class="fs1"/><circle r="34" class="fs2"/>' +
    '<circle r="34" class="frim"/><circle r="26" class="fg"/><circle r="18" class="fg"/><circle r="11" class="fg"/>' +
    '<path d="M0-34V34M-34 0H34M-24-24L24 24M24-24L-24 24" class="fsp"/>' +
    '<g id="fo"><circle r="30" fill-opacity="0"/>' +
    [0, 120, 240].map(function (a) {
      return '<path d="M-3.5-7C-10-28 10-28 3.5-7C2-3 1.2 1 0 5C-1.2 1-2-3-3.5-7Z" class="bl" transform="rotate(' + a + ')"/>';
    }).join('') +
    '<circle r="6" class="fh"/><circle r="2" class="fc"/></g></g>' +
    '<rect x="112" y="52" width="62" height="40" rx="8" class="cp"/>' +
    '<text x="143" y="76" class="sm">comp</text>' +
    '' +
    chip("tout", 12, 112, 80) + chip("tocoil", 98, 112, 80) +
    chip("freq", 12, 154, 80) + chip("pwr", 98, 154, 80) +
    '<text x="95" y="206" class="oh" id="oh"></text></g>' +
    // indoor unit
    '<g transform="translate(440,10)"><rect width="190" height="220" rx="14" class="u ui" id="ui"/>' +
    '<text x="95" y="24" class="ut" data-t="indoor"></text>' +
    '<rect x="18" y="38" width="154" height="40" rx="10" class="sp"/>' +
    '<path d="M30 70 H160" class="lv"/>' +
    '<rect x="30" y="86" width="130" height="18" rx="9" class="md"/>' +
    '<text x="95" y="99" class="mt" id="mt"></text>' +
    chip("troom", 12, 112, 80, "h") + chip("ttgt", 98, 112, 80) +
    chip("ticoil", 12, 154, 80) + chip("hum", 98, 154, 80) +
    '' +
    // legend
    '<g transform="translate(222,240)"><line x1="0" y1="0" x2="20" y2="0" class="lg hot"/>' +
    '<text x="26" y="4" class="lt" data-t="legHot"></text>' +
    '<line x1="112" y1="0" x2="132" y2="0" class="lg cold"/>' +
    '<text x="138" y="4" class="lt" data-t="legCold"></text></g></svg>';

  var CSS =
    "#acd{--hot:#f08a24;--cold:#1784f7;--acc:#1784f7;margin:8px 12px 4px;padding:12px;border-radius:14px;" +
    "background:color-mix(in srgb,Canvas 94%,CanvasText 6%);border:1px solid color-mix(in srgb,CanvasText 12%,transparent);" +
    "color:CanvasText;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif}" +
    "#acd .hd{display:flex;justify-content:space-between;align-items:flex-start;gap:8px}" +
    "#acd h2{margin:0;font-size:16px;font-weight:600;line-height:1.2;background:linear-gradient(90deg,#0fe3fa,#1784f7);-webkit-background-clip:text;background-clip:text;color:transparent}" +
    "#acd p{margin:2px 0 6px;font-size:12px;opacity:.6}" +
    "#acd button{padding:4px 9px;border-radius:8px;border:1px solid color-mix(in srgb,CanvasText 22%,transparent);background:Canvas;color:inherit;font-size:11px;font-weight:700;font-family:inherit;cursor:pointer}" +
    "#acd svg{display:block;width:100%;height:auto;max-height:340px}" +
    "#acd text{fill:currentColor;text-anchor:middle}" +
    "#acd .u{fill:color-mix(in srgb,Canvas 88%,CanvasText 12%);stroke:color-mix(in srgb,CanvasText 20%,transparent);stroke-width:1.5}" +
    "#acd .ui{fill:color-mix(in srgb,Canvas 86%,#1784f7 14%)}" +
    "#acd .ut{font:600 12px sans-serif;opacity:.85}#acd .sm{font:10px sans-serif;opacity:.6}" +
    "#acd .lv{fill:none;stroke:currentColor;stroke-width:2;opacity:.45}" +
    "#acd .fs1{fill:color-mix(in srgb,#6b7280 70%,Canvas 30%)}#acd .fs2{fill:color-mix(in srgb,#111827 55%,#4b5563 45%)}" +
    "#acd .frim{fill:none;stroke:#e5e7eb;stroke-width:2;opacity:.55}#acd .fg,#acd .fsp{fill:none;stroke:#9ca3af;stroke-width:1.1;opacity:.5}" +
    "#acd .bl{fill:#d1d5db;stroke:#6b7280;stroke-width:.6}#acd .fh{fill:#9ca3af;stroke:#4b5563}#acd .fc{fill:#e5e7eb}" +
    "#acd #fo{transform-box:fill-box;transform-origin:50% 50%}" +
    "#acd .cp,#acd .sp{fill:color-mix(in srgb,Canvas 72%,CanvasText 28%);opacity:.8}" +
    "#acd .md{fill:color-mix(in srgb,Canvas 70%,var(--acc) 30%)}#acd .mt{font:700 10px sans-serif}" +
    "#acd .c{fill:color-mix(in srgb,Canvas 74%,CanvasText 26%)}#acd .c.h{fill:color-mix(in srgb,Canvas 70%,var(--acc) 30%)}#acd.idle #ui,#acd.idle #ui .c{opacity:.55}" +
    "#acd .cl{font:600 9px sans-serif;opacity:.7}#acd .cv{font:700 12px ui-monospace,Menlo,monospace}" +
    "#acd .pp{fill:none;stroke-width:9;stroke-linecap:round;stroke:#9aa0a6;opacity:.3;transition:stroke .4s,opacity .4s}" +
    "#acd .pf{fill:none;stroke-width:3;stroke-linecap:round;stroke:Canvas;stroke-dasharray:4 14;opacity:0}" +
    "#acd .hot{stroke:var(--hot)}#acd .cold{stroke:var(--cold)}#acd .pp.hot,#acd .pp.cold{opacity:.9}" +
    "#acd .oh{font:600 9px sans-serif;opacity:.85;text-anchor:middle}#acd.other .oh{fill:var(--hot)}" +"#acd.run .pf{opacity:.55;animation:acf 1.2s linear infinite}#acd.run .pf.rev{animation-direction:reverse}" +
    "#acd.run #fo,#acd.orun #fo{animation:acs 1.15s linear infinite}" +
    "#acd .lg{stroke-width:6;stroke-linecap:round}#acd .lt{font:10px sans-serif;opacity:.65;text-anchor:start}" +
    "@keyframes acf{to{stroke-dashoffset:-18}}@keyframes acs{to{transform:rotate(360deg)}}" +
    "@media (prefers-reduced-motion:reduce){#acd.run .pf,#acd.run #fo,#acd.orun #fo{animation:none}}" +
    "@media (max-width:600px){#acd{margin:6px;padding:8px}}";

  function fmt(v, u, d) {
    return typeof v === "number" && isFinite(v) ? v.toFixed(d) + "\u00a0" + u : "—";
  }
  // Sensors also arrive as strings / "NA" (nan via JSON) on web_server.
  function numberOrNaN(v) {
    if (v == null || v === "NA" || v === "nan" || v === "") return NaN;
    var n = typeof v === "number" ? v : parseFloat(v);
    return isFinite(n) ? n : NaN;
  }
  // web_server sends climate temperatures as strings, or "NA" when NAN.
  function temp(v, prev) {
    if (v == null || v === "NA" || v === "nan" || v === "") return prev;
    var n = typeof v === "number" ? v : parseFloat(v);
    return isFinite(n) ? fmt(n, "°C", 1) : prev;
  }

  function paint() {
    var sr = root(), el = sr && sr.getElementById("acd");
    if (!el) return;
    var q = function (s) { return el.querySelectorAll(s); };
    q("[data-t]").forEach(function (n) { n.textContent = t(n.getAttribute("data-t")); });
    q("[data-v]").forEach(function (n) { n.textContent = vals[n.getAttribute("data-v")] || "—"; });
    if (vals.ftgt && vals.ftgt !== "—") {
      var f = el.querySelector('[data-v="freq"]');
      if (f) f.textContent = (vals.freq || "—").replace("\u00a0Hz", "") + "/" + vals.ftgt;
    }
    el.querySelector("h2").textContent = t("title");
    el.querySelector("p").textContent = t("sub");
    var b = el.querySelector("button");
    b.textContent = lang === "pt" ? "PT · EN" : "EN · PT";
    b.title = t("lang");
    el.querySelector("svg").setAttribute("aria-label", t("title"));
    var m = mode.toLowerCase().replace(/^climate_mode_/, "");
    var act = action.toLowerCase().replace(/^climate_action_/, "");
    el.querySelector("#mt").textContent = m ? t("m_" + m) : "—";
    // Heat: hot gas goes to the indoor unit on the top line. Cool: roles swap.
    if (!m && act) m = act;
    var heat = m === "heat" || act === "heating", cool = m === "cool" || m === "dry" || m === "heat_cool" || m === "auto" || act === "cooling" || act === "drying";
    // Any mode other than OFF (including FAN_ONLY) can run the outdoor fan. Frequency
    // is a fallback for units whose climate action is not exposed or not updated.
    // Frequency fallback only until the climate entity arrives (its last value may be
    // stale, so it must never keep the fan spinning against an OFF climate state).
    var run = !!m && m !== "off" && act !== "off";
    // Multi-split: the outdoor unit is shared, so it can be running for another indoor
    // unit while THIS one is OFF. Outdoor activity (compressor frequency, or power when
    // no valid frequency exists) drives the outdoor fan, never the indoor unit/pipes.
    var active = null;
    if (typeof outdoor.freq === "number" && isFinite(outdoor.freq)) active = outdoor.freq > 0;
    else if (typeof outdoor.pwr === "number" && isFinite(outdoor.pwr)) active = outdoor.pwr > 0;
    var outdoorRun = active === null ? run : active;
    var forOther = outdoorRun && !run;
    var p1 = el.querySelector("#p1"), p2 = el.querySelector("#p2");
    p1.setAttribute("class", "pp" + (run ? (heat ? " hot" : " cold") : ""));
    p2.setAttribute("class", "pp" + (run ? (heat ? " cold" : " hot") : ""));
    // Flow direction: heat = top line outdoor->indoor; cool = reversed.
    el.querySelector("#f1").setAttribute("class", "pf" + (heat ? "" : " rev"));
    el.querySelector("#f2").setAttribute("class", "pf" + (heat ? " rev" : ""));
    el.className = (run ? "run" : "idle") + (outdoorRun ? " orun" : "") + (forOther ? " other" : "");
    el.querySelector("#oh").textContent = forOther ? t("other") : "";
  }

  function mount() {
    var sr = root(), main = sr && sr.querySelector("main");
    if (!main) return false;
    if (!sr.getElementById("acd")) {
      var st = document.createElement("style");
      st.textContent = CSS;
      sr.appendChild(st);
      var el = document.createElement("section");
      el.id = "acd";
      el.innerHTML = '<div class="hd"><div><h2></h2><p></p></div><button type="button"></button></div>' + SVG;
      (main.parentNode || main).insertBefore(el, main);
      el.querySelector("button").onclick = function () {
        lang = lang === "pt" ? "en" : "pt";
        try { localStorage.setItem(KEY, lang); } catch (e) {}
        paint();
        translateTable();
      };
    }
    paint();
    return true;
  }

  function norm(s) { return String(s || "").toLowerCase().replace(/^[a-z_]+[-\/]/, "").replace(/[^a-z0-9]/g, ""); }
  function applyClimate(d) {
    if (d.mode != null) mode = String(d.mode);
    if (d.action != null) action = String(d.action);
    // ESPHome climate JSON: current_temperature / target_temperature. When the unit is OFF
    // the firmware can publish only target, so keep the last known room temperature.
    vals.troom = temp(d.current_temperature, vals.troom);
    vals.ttgt = temp(d.target_temperature, vals.ttgt);
  }

  function onState(e) {
    var d;
    try { d = JSON.parse(e.data); } catch (x) { return; }
    var id = String(d.id || "");
    if (/^climate[-\/]/.test(id)) {
      applyClimate(d);
    } else if (d.state === undefined && d.value === undefined && d.current_temperature !== undefined) {
      // Wrap object form: {climate: {...}}
      applyClimate(d.climate || d);
    } else {
      var m = MAP[norm(d.name_id || id)] || MAP[norm(d.name)];
      if (!m) return;
      vals[m[0]] = fmt(d.value, m[1], m[2]);
      if (outdoor[m[0]] !== undefined) outdoor[m[0]] = numberOrNaN(d.value);
    }
    paint();
  }

  var es = new EventSource("/events");
  es.addEventListener("state", onState);

  var n = 0, logOk = false, dOk = false, timer = setInterval(function () {
    logOk = logOk || fixLog();
    dOk = dOk || mount();
    translateTable();
    if ((logOk && dOk) || ++n > 240) clearInterval(timer);
  }, 250);
})();
