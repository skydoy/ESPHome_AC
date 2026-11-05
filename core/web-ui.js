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
    indoorhumidity: ["hum", "%", 0]
  };
  var vals = {}, mode = "";

  function chip(k, x, y, w, cls) {
    return '<g data-c="' + k + '" transform="translate(' + x + "," + y + ')">' +
      '<rect width="' + w + '" height="36" rx="8" class="c' + (cls ? " " + cls : "") + '"/>' +
      '<text x="' + w / 2 + '" y="14" class="cl" data-t="' + k + '"></text>' +
      '<text x="' + w / 2 + '" y="29" class="cv" data-v="' + k + '">—</text></g>';
  }
  var SVG =
    '<svg viewBox="0 0 640 220" role="img">' +
    // pipes: top = gas line, bottom = liquid line
    '<path id="p1" class="pp" d="M200 95 H440"/><path id="p2" class="pp" d="M200 165 H440"/>' +
    '<path id="f1" class="pf" d="M200 95 H440"/><path id="f2" class="pf" d="M200 165 H440"/>' +
    // outdoor unit
    '<g transform="translate(10,10)"><rect width="190" height="200" rx="14" class="u"/>' +
    '<text x="95" y="24" class="ut" data-t="outdoor"></text>' +
    // Axial fan (same look as esphome-altherma): shroud, guard rings, spokes, 3-blade rotor.
    '<g id="fa" transform="translate(62,72) scale(.92)"><circle r="38" class="fs1"/><circle r="34" class="fs2"/>' +
    '<circle r="34" class="frim"/><circle r="26" class="fg"/><circle r="18" class="fg"/><circle r="11" class="fg"/>' +
    '<path d="M0-34V34M-34 0H34M-24-24L24 24M24-24L-24 24" class="fsp"/>' +
    '<g id="fo"><circle r="30" fill-opacity="0"/>' +
    [0, 120, 240].map(function (a) {
      return '<path d="M-3.5-7C-10-28 10-28 3.5-7C2-3 1.2 1 0 5C-1.2 1-2-3-3.5-7Z" class="bl" transform="rotate(' + a + ')"/>';
    }).join('') +
    '<circle r="6" class="fh"/><circle r="2" class="fc"/></g></g>' +
    '<rect x="112" y="52" width="62" height="40" rx="8" class="cp"/>' +
    '<text x="143" y="76" class="sm">comp</text>' +
    chip("tout", 12, 112, 80) + chip("tocoil", 98, 112, 80) +
    chip("freq", 12, 154, 80) + chip("pwr", 98, 154, 80) + "</g>" +
    // indoor unit
    '<g transform="translate(440,10)"><rect width="190" height="200" rx="14" class="u ui"/>' +
    '<text x="95" y="24" class="ut" data-t="indoor"></text>' +
    '<rect x="18" y="38" width="154" height="40" rx="10" class="sp"/>' +
    '<path d="M30 70 H160" class="lv"/>' +
    '<rect x="30" y="86" width="130" height="18" rx="9" class="md"/>' +
    '<text x="95" y="99" class="mt" id="mt"></text>' +
    chip("troom", 12, 112, 80, "h") + chip("ttgt", 98, 112, 80) +
    chip("ticoil", 12, 154, 80) + chip("hum", 98, 154, 80) + "</g>" +
    // legend
    '<g transform="translate(222,206)"><line x1="0" y1="0" x2="20" y2="0" class="lg hot"/>' +
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
    "#acd .c{fill:color-mix(in srgb,Canvas 74%,CanvasText 26%)}#acd .c.h{fill:color-mix(in srgb,Canvas 70%,var(--acc) 30%)}" +
    "#acd .cl{font:600 9px sans-serif;opacity:.7}#acd .cv{font:700 12px ui-monospace,Menlo,monospace}" +
    "#acd .pp{fill:none;stroke-width:9;stroke-linecap:round;stroke:#9aa0a6;opacity:.3;transition:stroke .4s,opacity .4s}" +
    "#acd .pf{fill:none;stroke-width:3;stroke-linecap:round;stroke:Canvas;stroke-dasharray:4 14;opacity:0}" +
    "#acd .hot{stroke:var(--hot)}#acd .cold{stroke:var(--cold)}#acd .pp.hot,#acd .pp.cold{opacity:.9}" +
    "#acd.run .pf{opacity:.55;animation:acf 1.2s linear infinite}#acd.run .pf.rev{animation-direction:reverse}" +
    "#acd.run #fo{animation:acs 1.15s linear infinite}" +
    "#acd.off .u,#acd.off .c,#acd.off #fa{opacity:.55}" +
    "#acd .lg{stroke-width:6;stroke-linecap:round}#acd .lt{font:10px sans-serif;opacity:.65;text-anchor:start}" +
    "@keyframes acf{to{stroke-dashoffset:-18}}@keyframes acs{to{transform:rotate(360deg)}}" +
    "@media (prefers-reduced-motion:reduce){#acd.run .pf,#acd.run #fo{animation:none}}" +
    "@media (max-width:600px){#acd{margin:6px;padding:8px}}";

  function fmt(v, u, d) {
    return typeof v === "number" && isFinite(v) ? v.toFixed(d) + "\u00a0" + u : "—";
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
    var m = mode.toLowerCase();
    el.querySelector("#mt").textContent = m ? t("m_" + m) : "—";
    // Heat: hot gas goes to the indoor unit on the top line. Cool: roles swap.
    var heat = m === "heat", cool = m === "cool" || m === "dry" || m === "heat_cool" || m === "auto";
    var run = heat || cool;
    var p1 = el.querySelector("#p1"), p2 = el.querySelector("#p2");
    p1.setAttribute("class", "pp" + (run ? (heat ? " hot" : " cold") : ""));
    p2.setAttribute("class", "pp" + (run ? (heat ? " cold" : " hot") : ""));
    // Flow direction: heat = top line outdoor->indoor; cool = reversed.
    el.querySelector("#f1").setAttribute("class", "pf" + (heat ? "" : " rev"));
    el.querySelector("#f2").setAttribute("class", "pf" + (heat ? " rev" : ""));
    el.className = run ? "run" : m === "off" || !m ? "off" : "";
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
      main.parentNode.insertBefore(el, main);
      el.querySelector("button").onclick = function () {
        lang = lang === "pt" ? "en" : "pt";
        try { localStorage.setItem(KEY, lang); } catch (e) {}
        paint();
      };
    }
    paint();
    return true;
  }

  function norm(s) { return String(s || "").toLowerCase().replace(/^[a-z_]+[-\/]/, "").replace(/[^a-z0-9]/g, ""); }
  function onState(e) {
    var d;
    try { d = JSON.parse(e.data); } catch (x) { return; }
    var id = String(d.id || "");
    if (/^climate[-\/]/.test(id)) {
      if (d.mode != null) mode = String(d.mode);
      vals.troom = fmt(d.current_temperature, "°C", 1);
      vals.ttgt = fmt(d.target_temperature, "°C", 1);
    } else {
      var m = MAP[norm(d.name_id || id)] || MAP[norm(d.name)];
      if (!m) return;
      vals[m[0]] = fmt(d.value, m[1], m[2]);
    }
    paint();
  }

  var es = new EventSource("/events");
  es.addEventListener("state", onState);

  var n = 0, logOk = false, dOk = false, timer = setInterval(function () {
    logOk = logOk || fixLog();
    dOk = dOk || mount();
    if ((logOk && dOk) || ++n > 240) clearInterval(timer);
  }, 250);
})();
