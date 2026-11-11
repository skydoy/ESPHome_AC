# Compatibilidade

**O meu AC é compatível?** Procura a tua marca abaixo. Não está na lista, ou já experimentaste?
**[Reportar o meu AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)**
(2 minutos, sem programar). Os relatórios sobem os modelos nesta página, incluindo os "não funciona".

As unidades interiores Hisense cujo módulo Wi-Fi está num **conector RS-485 de 4 pinos** (5 V ou 12 V, GND, A, B) falam o mesmo barramento "XM" a 9600 8N1, por isso *deviam* funcionar. "Devia" não é "funciona". Esta página separa as duas coisas.

Legenda:

- ✅ **Confirmado:** funciona com este firmware.
- 🧪 **Em teste:** a ser testado com este firmware.
- 🔶 **Reportado noutro sítio:** reportado a funcionar com *outro* firmware no mesmo barramento; por testar aqui.
- ❌ **Não funciona:** confirmado que não funciona com este firmware. Ainda não há nenhum na lista.
- ❔ **Por testar:** espera-se que funcione (mesmo barramento); ainda ninguém experimentou com este firmware.

## Com este firmware

| Marca | Unidade interior | Módulo Wi-Fi | Placa testada | Alimentação | Estado | Notas |
|---|---|---|---|---|---|---|
| Hisense | unidade do autor | AEH-W41H1 | M5StickC Plus2 + RS485 Hat | 5 V | ✅ Funciona (2026-10-29) | Modo, temperatura, ventilação e oscilação vertical confirmados; leituras de potência, tensão, corrente e humidade parciais. [Detalhes](#aeh-w41h1-unidade-do-autor). Reportado por [@skydoy](https://github.com/skydoy). |
| Hisense | qualquer | AEH-W41H1 / AEH-W4x | ATOM Lite + Tail485 | 5 V / 12 V | ❔ Por testar | O mesmo código de protocolo, ainda não experimentado numa máquina real. |

O autor testou o M5StickC Plus2 + RS485 Hat. A versão ATOM Lite + Tail485 usa o mesmo código de protocolo e é verificada automaticamente, mas não foi testada em hardware. Os relatórios são bem-vindos.

### AEH-W41H1 (unidade do autor)

Testado em 2026-10-28 e 2026-10-29 (comandos) num M5StickC Plus2 + RS485 Hat, alimentado pelo fio de 5 V (X).

Confirmado (estado reportado e na máquina):

- Handshake da ligação: AC Link ligado, códigos de avaria OK, tramas de estado de 160 bytes.
- Modos Off / Cool / Heat / Fan only / Dry / Auto oferecidos; Cool a 24 °C e a 22 °C.
- Ventilação Auto → Low → High → Auto; oscilação vertical ligada/desligada; desligar.
- Frequência do compressor (28–30 Hz em funcionamento, 0 desligado), temperatura exterior, temperatura da serpentina exterior e temperatura da sala, todas plausíveis.

Calibração ainda por fazer (vê [registo de tramas em bruto](configuration.md#registo-de-tramas-em-bruto)):

- **Potência:** descodifica-se como `4.15 × b55²`, como na referência, por isso em carga baixa anda em degraus grosseiros (b55 = 4 → 66,4 W, 5 → 103,75 W). Ainda não foi comparada com um medidor.
- **Temperatura da serpentina interior** (byte 21): ficou na temperatura da sala (27–28 °C) durante cerca de 2 minutos de Cool, por isso neste aparelho o byte pode querer dizer outra coisa.
- **Tensão de alimentação, corrente, humidade:** esta unidade ainda não os enviou. Ficam escondidos por predefinição e leem "unknown".

## Reportado a funcionar com outro firmware (por testar aqui)

Estas unidades foram reportadas a funcionar com outro firmware de código aberto no mesmo barramento. São candidatas, não garantias.

| Marca | Unidade interior | Módulo Wi-Fi | Fonte |
|---|---|---|---|
| Hisense | CITY DC Inverter AS-13UW4RYRCM04G / AS-13UW4RYRCM04W | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | SILVER CRYSTAL SUPER DC Inverter AS-13UW4RVETG01 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | Free Match multi-split 4AMW81U4RJC | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Newtek | NT-77HSDC12 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Ballu | iGreen Pro DC BSAGI-07HN8, BSAGI-12HN8, BSAGI-18HN8_V4 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Ballu | Platinum DC BSEI-09HN8_V3 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | AST-12UW4RVETG00A | AEH-W4E1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Tornado | TOP-INV-120A (WIFI), também TOP-INV-140A / 180A | AEH-W4F1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Tornado | MULTI-12A (WIFI), AST-09UW4RVETV00D | AEH-W4G2 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| ACOND | ASTI-09UW4RVEDC00 | AEH-W4B1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Hisense | modelo não indicado | AEH-W4G2 (regravado no sítio) | [pio2398/W4G2](https://github.com/pio2398/W4G2) |
| Hisense | unidades do autor, modelo não indicado | AEH-W41H1 | [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1) |
| não indicado (mercado da China) | KFR-26GW | | [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac) (v1/v2) |
| não indicado (mercado da China) | KFR-72GW | | [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac) (v3; escritas RS-485 pouco fiáveis, usa IR para comandos) |

## Módulos Wi-Fi neste barramento

AEH-W4A1, AEH-W4B1, AEH-W4E1, AEH-W4F1, AEH-W4G1, AEH-W4G2 e AEH-W41H1 usam o mesmo conector de 4 pinos. **Mede o pino de alimentação antes de ligares.** Umas máquinas dão 5 V, outras 12 V ([ligações](wiring.md)). Máquinas com outra marca (Tornado, ACOND, Ballu, Newtek e outras) usam muitas vezes a mesma eletrónica.

## Reportar a partir do dispositivo (AC Identity)

Depois de a placa ligar ao AC, corre uma **identificação só de leitura** (tipo de unidade, versão do firmware, capacidades e uma trama de estado completa). Abre `http://<ip-da-placa>/`:

1. Vê o **AC Identity** (resumo compacto; também aparece no log INFO).
2. Abre ou copia **Report my AC**. Com o **Auto report** ligado, o primeiro relatório com sucesso guarda um link curto `/r/<8 hex>` no Worker. Esse link redireciona para o formulário do GitHub já preenchido. O URL completo do GitHub fica só no log INFO, na linha `Report my AC (full):`, para a página web da placa continuar legível.
3. Preenche o resto (marca/modelo, placa, resultado) e carrega em **Submit**.

Se o **Auto report** estiver desligado antes desse primeiro POST com sucesso, o sensor de texto continua a mostrar o link curto `/r/<8 hex>`, mas o Worker não tem o mapeamento guardado e responde 404. Usa a linha `Report my AC (full):` no log INFO. Carregar em **Identify AC** repete as consultas só de leitura e volta a publicar o mesmo link curto. Não escreve o mapeamento enquanto o relatório está desligado.

Com o **Auto report** ligado, **Identify AC** atualiza o link depois de um relatório novo ficar guardado. O comportamento do Worker está em [worker/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/worker/README.md). Esta página não repete os passos de publicação.

### Relatório automático de compatibilidade (opcional)

Com **Auto report** ligado (predefinição), depois da identificação e com Wi-Fi ativo a placa faz um POST uma vez: `identity`, `firmware`, `board` e um **sha256 do MAC Wi-Fi** como `device_id` (nunca o MAC em claro). Pode ir também `status_hex`. Não se guardam IPs. Desliga **Auto report** no Home Assistant ou na página da placa para o desativares. A mesma identidade só volta a ser enviada se mudar. O controlo do AC continua local. Isto é um sinal anónimo de compatibilidade, não uma nuvem de controlo.

## Verificar a tua máquina num minuto

1. Emparelha com a app **ConnectLife**, uma app Hisense antiga, ou um equivalente com outra marca.
2. O módulo Wi-Fi é um pequeno dongle atrás da tampa frontal. A referência começa por **AEH-W4**.
3. O conector tem **4 pinos**. Mede 5 V ou 12 V entre o pino de alimentação e o GND.

Se as três forem verdade, é um bom candidato. Notas do módulo: [brands/hisense/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/README.md).

O que ainda não se sabe: [Questões em aberto](open-questions.md). A lista curta em português no GitHub não inclui a tabela de candidatos: [docs/compatibilidade.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/compatibilidade.md). A tabela completa em inglês: [docs/compatibility.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/compatibility.md).
