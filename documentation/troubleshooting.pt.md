# Problemas

Soluções rápidas para os problemas mais comuns. Continua sem funcionar?
[Abre um pedido](https://github.com/skydoy/ESPHome_AC/issues/new/choose) e cola o registo
([Ler os registos](#ler-os-registos)).

!!! warning "Tensão da rede"
    Desliga o ar condicionado **no disjuntor** antes de abrires a unidade interior ou tocares em qualquer fio.

- [O ecrã não mostra o AC, ou a ligação não arranca](#o-ecra-nao-mostra-o-ac-ou-a-ligacao-nao-arranca)
- [O Stick funciona por USB mas não pelo ar condicionado](#o-stick-funciona-por-usb-mas-nao-pelo-ar-condicionado)
- [O Home Assistant não encontra a placa](#o-home-assistant-nao-encontra-a-placa)
- [Uma atualização falhou, ou o Device Builder instala uma versão antiga](#uma-atualizacao-falhou-ou-o-device-builder-instala-uma-versao-antiga)
- [A página da placa não abre](#a-pagina-da-placa-nao-abre)
- [Mudar de Wi-Fi, repor de fábrica ou reinstalar](#mudar-de-wi-fi-repor-de-fabrica-ou-reinstalar)
- [Ler os registos](#ler-os-registos)

## O ecrã não mostra o AC, ou a ligação não arranca

Sinais: o Stick mostra **No link** / `No AC: swap A/B? G0 10k pull-up?`, o LED do ATOM fica **vermelho**, ou o **AC Link** está desligado no Home Assistant.

1. **Troca o A com o B.** É a causa mais comum. Trocar é seguro: só levam sinal.
2. **Verifica o GND.** O fio GND da ficha tem de ir ao terminal GND (terminal 4 do Hat, GND do Tail485).
3. **Verifica a alimentação.** A placa tem de ficar ligada com o USB desligado. Ficha de 5 V: o X vai ao **pino 5V** da placa, nunca ao terminal 12V. Ficha de 12 V: o X vai ao terminal 12V. Se o **Reset Reason** mostrar `brownout`, a alimentação de 5 V está fraca ou mal ligada.
4. **Não sabes qual é cada fio?** Vê [Ligações → Medir a ficha](wiring.md#1-medir-a-ficha).

A ligação costuma arrancar menos de um minuto depois de ligares.

## O Stick funciona por USB mas não pelo ar condicionado

A linha TX do Stick é o **G0**, um pino de arranque. Alimentado pelo ar condicionado, por vezes fica em modo de programação: ecrã apagado, sem Wi-Fi. Solução: solda uma **resistência de 10 kΩ entre o G0 e os 3,3 V** no conector do Stick. [Detalhes](wiring.md#3-pino-de-arranque-g0-le-isto-antes-de-fechares-o-ar-condicionado). O ATOM não tem este problema.

## O Home Assistant não encontra a placa

1. Confirma que a placa está no Wi-Fi: o Stick mostra o endereço IP no canto inferior direito (com a ligação ao AC ativa), o LED do ATOM não está **azul**.
2. Adiciona-a à mão: **Definições → Dispositivos e serviços → Adicionar integração → ESPHome**, escreve o endereço IP da placa, porta `6053`.
3. O Home Assistant e a placa têm de estar na mesma rede. Redes de convidados e o isolamento de clientes impedem a descoberta.
4. Aparece como indisponível depois de pores uma chave de API, ou depois de instalares o firmware do instalador web por cima da tua própria versão? Vê [encriptação da API](configuration.md#encriptacao-da-api-e-palavra-passe-ota-opcional).

## Uma atualização falhou, ou o Device Builder instala uma versão antiga

- **Firmware do instalador web:** as atualizações instalam-se sozinhas de madrugada (interruptor **Auto Update**), ou carrega em **Install** na entidade **Firmware Update**. Uma atualização demora 1 a 2 minutos e a placa parece desligada até reiniciar. Se falhar, o firmware antigo continua a funcionar; tenta mais tarde.
- **Adotada no ESPHome Device Builder:** o teu YAML tem de importar a `@stable`:

```yaml
packages:
  skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
```

ATOM: `devices/hisense-atom-lite-tail485.yaml@stable`. O Device Builder guarda o GitHub em cache até um dia, por isso uma versão nova pode demorar um dia a aparecer. Para a teres já, abre o menu **⋮** da placa → **Clean Build Files** e depois **Install**.

- **Erros como "invalid option" depois de atualizar:** **Clean Build Files** e depois **Install**.
- **Nunca carregues em *Install* numa entidade "Firmware Update" de uma placa adotada** (só as versões adotadas antes da v0.2.9 a têm). Substituiria a tua configuração. Carrega uma vez em **Install** no Device Builder e a entidade desaparece.

## A página da placa não abre

Escreve **`http://`** seguido do endereço IP, por exemplo `http://192.168.1.50/`. A placa não tem `https://`, e alguns navegadores acrescentam-no sozinhos. A página precisa de acesso à internet no navegador (vai buscar os scripts ao site do ESPHome). [Página web da placa](web-page.md).

## Mudar de Wi-Fi, repor de fábrica ou reinstalar

- **Rede Wi-Fi nova:** se a placa não chega à rede antiga, abre a rede `ESPHome_AC Setup` ao fim de cerca de um minuto. Liga-te a ela com o telemóvel e escolhe a rede nova. Ou liga-a por USB, abre o [instalador web](https://skydoy.github.io/ESPHome_AC/), carrega em **Connect** e escolhe **Change Wi-Fi**.
- **Repor de fábrica:** ATOM: mantém o botão carregado 10 s (o LED fica branco e depois reinicia em modo de configuração). Stick, ou qualquer das placas: liga por USB, **Connect** no instalador web e escolhe **Erase User Data** (aparece quando a placa já tem a versão mais recente).
- **Reinstalar:** **Connect** no instalador web. Numa placa que já tem este firmware, **Update** mantém o Wi-Fi. Uma primeira instalação apaga a placa. ATOM: tira primeiro o Tail485, que usa a porta USB-C.

## Ler os registos

- **No navegador:** abre `http://<ip-da-placa>/`. A página mostra o registo em direto.
- **Por USB:** **Connect** no [instalador web](https://skydoy.github.io/ESPHome_AC/) → **Logs & Console**.
- **ESPHome Device Builder:** o botão **Logs** da placa.

Linhas úteis: `hisense_ac` (ligação e tramas), `auto_update` (atualizações de madrugada), `update_check` (versão nova encontrada). Para capturares as tramas do ar condicionado para um relatório de erro, liga o **AC Frame Logging** ([detalhes](configuration.md#registo-de-tramas-em-bruto)).

O guia no GitHub: [docs/problemas.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/problemas.md).
