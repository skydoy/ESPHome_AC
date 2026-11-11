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

## Reportar a partir do dispositivo (AC Identity)

Depois de a placa ligar ao AC, corre uma **identificação só de leitura** (tipo de unidade,
versão do firmware, capacidades e uma trama de estado completa). Abre a página do dispositivo
em `http://<ip-do-dispositivo>/`:

1. Vê o **AC Identity** (resumo compacto; também aparece no log INFO).
2. Abre ou copia **Report my AC**. Com o **Auto report** ligado, o primeiro relatório com
   sucesso guarda um link curto `/r/<8 hex>` no Worker, que redirecciona para o formulário do
   GitHub já preenchido. O URL completo do GitHub fica só no log INFO (`Report my AC (full):`)
   para a página web da placa não partir o layout.
3. Preenche o resto (marca/modelo, placa, resultado) e carrega em **Submit**.

Se o **Auto report** estava desligado antes desse primeiro POST com sucesso, o sensor de texto
continua a mostrar o link curto `/r/<8 hex>`, mas o Worker não tem nada guardado e responde 404.
Usa `Report my AC (full):` no log INFO. O **Identify AC** repete as consultas só de leitura e
volta a publicar o mesmo link curto. Não cria o mapeamento enquanto o relatório está desligado.

### Relatório automático de compatibilidade (opcional)

Com **Auto report** ligado (predefinição), depois da identificação e com Wi-Fi activo o
dispositivo faz um POST uma vez para um Cloudflare Worker: `identity`, `firmware`, `board` e
um **sha256 do MAC Wi-Fi** como `device_id` (nunca o MAC em claro). Pode ir também
`status_hex`. Não se guardam IPs. Desliga **Auto report** no Home Assistant / na página do
dispositivo para desactivar. A mesma identidade só volta a ser enviada se mudar. O controlo
do AC continua local — isto é um sinal anónimo de compatibilidade, não cloud de controlo.

## Verificar a tua máquina num minuto
1. Usas a app **ConnectLife** (ou uma app Hisense antiga / equivalente).
2. O módulo Wi-Fi atrás da tampa frontal tem uma referência que começa por **AEH-W4**.
3. A ficha tem **4 pinos**; mede 5 V ou 12 V entre a alimentação e GND (ver [ligações](wiring.md)).

Se as três forem verdade, é um bom candidato.
