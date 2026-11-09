# Visão geral

Detalhes que estavam no [README em português](../README.pt.md). Começo curto: [README.pt.md](../README.pt.md) · [Guia rápido](guia-rapido.md).

## O que é

Substitui o módulo Wi-Fi do teu ar condicionado **Hisense** (AEH-W4x / **AEH-W41H1**) por uma pequena
placa ESP32 da M5Stack e controla-o localmente a partir do Home Assistant. Sem nuvem para o controlo;
relatório anónimo de compatibilidade opcional. Sem app do fabricante.
Mais marcas de ar condicionado serão adicionadas à medida que o projeto cresce.

Gratuito para uso pessoal e não comercial. Forks são bem-vindos. O uso comercial não é permitido.

**O meu AC é compatível?** Vê a [lista de compatibilidade](compatibilidade.md) ·
[Reportar o meu AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)

## O que aparece no Home Assistant

| Entidade | Para que serve |
|---|---|
| **Air Conditioner** | Modos Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 velocidades (incl. silenciosa), swing, predefinições Eco, Boost, Sleep e proteção anti-gelo a 8 °C. |
| **Sensores** | Temperatura exterior e das serpentinas, frequência do compressor, humidade, tensão, potência, corrente, energia de hoje (quando a máquina os envia). |
| **AC Link**, **AC Fault** | Se a ligação à máquina está bem, e os códigos de avaria. |
| **AC Display**, **Eco**, **Super**, **Fan Mute**, **Sleep Mode** | Luz do painel e os modos extra da própria máquina. |
| **Firmware Update**, **Auto Update** | Novas versões com um clique, ou sozinhas de madrugada. |

A placa tem também uma página em `http://<ip-da-placa>/` com os estados e os registos em direto (sem palavra-passe,
só na rede local; [como pôr uma ou desligar](configuration.md#web-server)).
O firmware vem sem chave de API; para pôr uma (recomendado), vê
[API encryption](configuration.md#api-encryption-and-ota-password-optional).
Adotaste-a no ESPHome Device Builder? Basta carregar em **Install**: segue a `@stable`
([detalhes](configuration.md#esphome-device-builder)).

Capturas da página web da placa: [Página web da placa](pagina-web.md).

## Placas

| | **ATOM Lite + ATOM Tail485** | **M5StickC Plus2 + RS485 Hat** |
|---|---|---|
| Preço aprox. | ≈ 15 US$ (7,50 + 6,95) | ≈ 28 US$ (19,95 + ≈ 8) |
| Estado | LED RGB + botão | Ecrã + dois botões |
| Ligações | [Ligações do ATOM](wiring.md#atom-lite--tail485) | [Ligações do Stick](wiring.md#2-connect) |
| Ligações externas | [ATOM Lite](https://docs.m5stack.com/en/core/ATOM%20Lite) · [Tail485](https://docs.m5stack.com/en/atom/tail485) | [M5StickC Plus2](https://docs.m5stack.com/en/core/M5StickC%20PLUS2) · [RS485 Hat](https://docs.m5stack.com/en/hat/hat_rs485) |

Preços de tabela da loja M5Stack, sem portes nem impostos. O Plus2 foi descontinuado na loja M5Stack.

> ⚠️ **Pino de arranque G0 (só no Stick):** se o Stick arranca por USB mas não pelo ar condicionado, solda uma
> resistência de 10 kΩ entre o G0 e os 3,3 V. [Detalhes](wiring.md#3-boot-pin-g0-read-this-before-closing-the-ac)

> **Estado:** por agora só a Hisense é suportada (máquinas cujo módulo Wi-Fi liga ao conector RS-485 de 4 pinos,
> vê a [tabela de compatibilidade](compatibilidade.md)). A versão ATOM é verificada automaticamente mas ainda
> não foi testada numa máquina real. Experimentaste?
> [Conta-nos como correu](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml).
> Outras marcas (Mitsubishi, Daikin, Midea, …) esperam por quem as possa testar; vê o
> [CONTRIBUTING.md](../CONTRIBUTING.md).

[![Esquema de ligações](wiring.svg)](wiring.md)

## Atualizações automáticas (OTA)

A placa procura firmware novo sozinha (ao arrancar e de 6 em 6 horas). Só aparecem releases
testadas do ramo `stable`.
Chave de API, Device Builder vs OTA de fábrica, e a página web aberta na LAN:
[Segurança e OTA](seguranca.md).

- **Imagem do instalador web:** o Home Assistant mostra **Firmware Update**. Carrega em
  **Install** (ou deixa o **Auto Update** ligado para instalar de madrugada). O Wi-Fi e as
  definições mantêm-se.
- **Adotada no ESPHome Device Builder:** a placa só *avisa* que há versão nova
  (**Update Status**, **Check for Update**). Carrega em **Install** no Device Builder; a
  config já segue a `stable`, sem editar YAML.

## Página web da placa

Abre `http://<ip-da-placa>/` na tua rede local para veres os estados em direto, o esquema
do AC e os registos (sem palavra-passe de propósito; [notas de segurança](seguranca.md)).
Língua: botão PT/EN no canto superior. Os textos de diagnóstico seguem a mesma língua.

Capturas completas: [Página web da placa](pagina-web.md).

## Compilação própria / Device Builder

- Stick: [devices/hisense-m5stickc-plus2.yaml](../devices/hisense-m5stickc-plus2.yaml)
- ATOM: [devices/hisense-atom-lite-tail485.yaml](../devices/hisense-atom-lite-tail485.yaml)
- Exemplo: [examples/my-ac-livingroom.yaml](../examples/my-ac-livingroom.yaml)

Os documentos técnicos estão em inglês, exceto o guia rápido, os problemas e esta página.

## Créditos

- Implementação original; nenhum código copiado. Conhecimento do protocolo vindo de
  (vê [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)):
  [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1),
  [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome),
  [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense),
  [pio2398/W4G2](https://github.com/pio2398/W4G2),
  [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac),
  [straga/scrivo_project](https://github.com/straga/scrivo_project),
  [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl) e
  [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).
- Firmware, instalador e logótipo: [skydoy](https://github.com/skydoy). As imagens das placas são ilustrações.

[PolyForm Noncommercial 1.0.0](../LICENSE): gratuito para uso pessoal e não comercial, forks bem-vindos,
uso comercial não permitido. Usa por tua conta e risco: a unidade interior tem tensão da rede lá dentro.
Hisense é uma marca registada do respetivo dono; este é um projeto independente da comunidade, sem ligação à marca.

## Mais documentação

[Problemas](problemas.md) · [Ligações](wiring.md) · [Home Assistant](home-assistant.md) ·
[Configuração](configuration.md) · [Compatibilidade](compatibilidade.md) ·
[Notas Hisense](../brands/hisense/README.md) · [Hardware](../brands/hisense/hardware.md) ·
[Protocolo](../brands/hisense/protocol.md) · [Adaptadores](../adapters/README.md) ·
[Alterações](../CHANGELOG.md) · [Segurança e OTA](seguranca.md) ·
[Reportar um problema](https://github.com/skydoy/ESPHome_AC/issues/new/choose) ·
[Discussões](https://github.com/skydoy/ESPHome_AC/discussions) ·
[Política de segurança](../SECURITY.md) · [Avisos de terceiros](../THIRD_PARTY_NOTICES.md)
