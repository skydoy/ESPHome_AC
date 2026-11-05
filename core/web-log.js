// Cap web_server v3 debug log height. Stock UI grows unbounded; .logs lives in
// esp-log's shadowRoot, so css_include cannot reach it — inject a <style> there.
(function () {
  var CSS =
    ".logs{max-height:min(45vh,420px);overflow-y:auto;overscroll-behavior:contain}";
  function fix() {
    var app = document.querySelector("esp-app");
    var log = app && app.shadowRoot && app.shadowRoot.querySelector("esp-log");
    var root = log && log.shadowRoot;
    if (!root) return false;
    if (!root.getElementById("web-log-css")) {
      var st = document.createElement("style");
      st.id = "web-log-css";
      st.textContent = CSS;
      root.appendChild(st);
    }
    return true;
  }
  var n = 0,
    t = setInterval(function () {
      if (fix() || ++n > 120) clearInterval(t);
    }, 250);
})();
