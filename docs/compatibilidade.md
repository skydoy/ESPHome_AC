# Compatibilidade

*[English](compatibility.md)*

**O meu AC é compatível?** Procura a tua marca abaixo. Não está na lista, ou já experimentaste?
**[Reportar o meu AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)**
(2 minutos, sem programar). Os relatórios "não funciona" são igualmente úteis.

Legenda: ✅ Funciona · 🧪 Em teste · 🔶 Reportado com outro firmware · ❔ Por testar · ❌ Não funciona

## Com este firmware

| Marca | Unidade interior | Módulo Wi-Fi | Placa testada | Alimentação | Estado | Notas |
|---|---|---|---|---|---|---|
| Hisense | unidade do autor | AEH-W41H1 | M5StickC Plus2 + RS485 Hat | 5 V | ✅ Funciona (2026-10-29) | Modo, temperatura, ventilação e oscilação vertical confirmados; potência, tensão, corrente e humidade parciais |
| Hisense | qualquer | AEH-W41H1 / AEH-W4x | ATOM Lite + Tail485 | 5 V / 12 V | ❔ Por testar | Mesmo código de protocolo, ainda não testado numa máquina real |

## Candidatos (reportados com outro firmware, por testar aqui)

Hisense, Tornado, ACOND, Ballu e Newtek com módulo Wi-Fi **AEH-W4x / AEH-W41H1** na ficha de
4 pinos. Lista completa com modelos e fontes: [tabela em inglês](compatibility.md#reported-working-with-other-firmware-untested-here).

## Verificar a tua máquina num minuto
1. Usas a app **ConnectLife** (ou uma app Hisense antiga / equivalente).
2. O módulo Wi-Fi atrás da tampa frontal tem uma referência que começa por **AEH-W4**.
3. A ficha tem **4 pinos**; mede 5 V ou 12 V entre a alimentação e GND (ver [ligações](wiring.md)).

Se as três forem verdade, é um bom candidato.
