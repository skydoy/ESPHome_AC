/* Localise the header theme pill the same way the web installer does. */
(function () {
  function apply() {
    var lang = (document.documentElement.lang || "en").toLowerCase();
    var pt = lang.indexOf("pt") === 0;
    var dark = document.body.getAttribute("data-md-color-scheme") === "slate";
    var label = pt
      ? (dark ? "Mudar para modo claro" : "Mudar para modo escuro")
      : (dark ? "Switch to light mode" : "Switch to dark mode");
    document.querySelectorAll(".lang-theme").forEach(function (el) {
      if (el.hidden) return;
      el.title = label;
      el.setAttribute("aria-label", label);
    });
  }
  function arm() {
    apply();
    if (!document.body) return;
    new MutationObserver(apply).observe(document.body, {
      attributes: true,
      attributeFilter: ["data-md-color-scheme"]
    });
  }
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", arm);
  } else {
    arm();
  }
})();
