<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/logo-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/logo-light.png">
    <img src="docs/logo-light.png" alt="ESPHome_AC" width="256">
  </picture>
</p>

<p align="center"><b>Gostas? Ajuda-me a fazer algo ainda melhor.</b><br>
  <a href="https://ko-fi.com/skydoy"><img src="https://img.shields.io/badge/Ko--fi-Support%20me-FF5E5B?logo=ko-fi&logoColor=white" alt="Apoiar no Ko-fi"></a>
  <a href="https://buymeacoffee.com/skydoy"><img src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Support%20me-FFDD00?logo=buymeacoffee&logoColor=black" alt="Buy Me a Coffee"></a>
</p>

<p align="center">
  <a href="https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml"><img src="https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml"><img src="https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml/badge.svg" alt="Validate"></a>
  <a href="https://github.com/skydoy/ESPHome_AC/releases/latest"><img src="https://img.shields.io/github/v/release/skydoy/ESPHome_AC?sort=semver" alt="Versão"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-blue" alt="Licença: PolyForm Noncommercial"></a>
</p>

<p align="center"><b>O teu ar condicionado no Home Assistant. Liga por USB, carrega em Instalar, feito. Sem YAML.</b><br>
🇬🇧 <a href="README.md">Read in English</a></p>

<p align="center">
  <a href="https://skydoy.github.io/ESPHome_AC/"><img src="https://img.shields.io/badge/Abrir%20o%20instalador%20web-1784F7?style=for-the-badge&logo=esphome&logoColor=white" alt="Abrir o instalador web" height="40"></a>
</p>

<p align="center">
  <img src="docs/social-preview.png" alt="ESPHome_AC — ar condicionado Hisense no Home Assistant" width="640">
</p>

## Compatibilidade

**M5StickC Plus2 + RS485 Hat** ou **ATOM Lite + ATOM Tail485**, em unidades Hisense com o módulo Wi-Fi **AEH-W4x / AEH-W41H1** (RS-485 de 4 pinos).  
Lista completa: [compatibilidade](docs/compatibilidade.md) · [Reportar o meu AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)

## Instalar em 3 passos

1. **Instalar o firmware** — Abre o **[instalador web](https://skydoy.github.io/ESPHome_AC/)**, escolhe a placa, liga por USB → **Instalar**.
2. **Wi-Fi** — Escreve a tua rede de 2,4 GHz na mesma janela (ou liga-te depois a `ESPHome_AC Setup`).
3. **Ligar e adicionar** — Disjuntor desligado, liga a ficha de 4 fios ([ligações](docs/wiring.md)), disjuntor ligado. Home Assistant → **Configurar**.

Com imagens: [Guia rápido](docs/guia-rapido.md)

## O que obténs

- Clima: Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 velocidades, swing, Eco / Boost / Sleep
- Sensores: temps exterior e serpentinas, Hz do compressor, humidade, tensão, potência, energia (quando a máquina os envia)
- AC Link / Fault, luz do painel, Eco / Super / Fan Mute / Sleep
- Atualizações de firmware com um clique (ou Auto Update de madrugada)
- Página web da placa na tua LAN (`http://<ip-da-placa>/`) — [capturas](docs/pagina-web.md)
- Controlo só local — sem nuvem para o controlo; relatório anónimo de compatibilidade opcional

## Documentação e mais

[Visão geral (placas, OTA, créditos)](docs/visao-geral.md) ·
[Guia rápido](docs/guia-rapido.md) ·
[Ligações](docs/wiring.md) ·
[Home Assistant](docs/home-assistant.md) ·
[Configuração](docs/configuration.md) ·
[Problemas](docs/problemas.md) ·
[Compatibilidade](docs/compatibilidade.md) ·
[Página web da placa](docs/pagina-web.md) ·
[Segurança e OTA](docs/seguranca.md) ·
[Notas Hisense](brands/hisense/README.md) ·
[Alterações](CHANGELOG.md) ·
[Discussões](https://github.com/skydoy/ESPHome_AC/discussions)

[PolyForm Noncommercial 1.0.0](LICENSE) — gratuito para uso pessoal e não comercial; uso comercial não permitido.  
[Política de segurança](SECURITY.md) · [Avisos de terceiros](THIRD_PARTY_NOTICES.md)
