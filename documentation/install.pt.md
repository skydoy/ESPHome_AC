# Instalar

Faz isto **antes** de abrires o ar condicionado.

## 1. Abre o instalador

Abre **[https://skydoy.github.io/ESPHome_AC/](https://skydoy.github.io/ESPHome_AC/)** no **Chrome ou Edge** num computador. A página tem de abrir em https. Esta pré-visualização da documentação não muda o instalador.

## 2. Escolhe a placa

- **ATOM Lite + Tail485** — manifesto `install/atom-lite-tail485/manifest.json`
- **M5StickC Plus2 + RS485 Hat** — manifesto `install/m5stickc-plus2/manifest.json` (escolha predefinida no instalador)

Só Hisense AEH-W4x / AEH-W41H1. Outras marcas não estão no instalador.

## 3. Liga o USB e Instalar

Usa um cabo USB-C **de dados**. No ATOM, grava o firmware **antes** de encaixares o Tail485: o tail usa a mesma porta USB-C.

Carrega em **Install**, escolhe a porta série e confirma.

Deves ver uma barra de progresso (cerca de 1 minuto) e *Installation complete*. A mesma janela pede depois o Wi-Fi. Escolhe a tua rede de **2,4 GHz** e escreve a palavra-passe, ou liga-te mais tarde à rede `ESPHome_AC Setup` com o telemóvel. Também podes configurar o Wi-Fi por Bluetooth (Improv) a partir da app do Home Assistant.

Esta rede volta a aparecer sozinha sempre que a placa não consegue chegar ao Wi-Fi guardado. É aberta, e só enquanto o Wi-Fi não está ligado.

!!! tip "A placa não aparece?"
    Experimenta outro cabo. Stick: instala o controlador CH9102 ([M5Stack](https://docs.m5stack.com/en/core/M5StickC%20PLUS2)). ATOM: instala o [controlador FTDI VCP](https://ftdichip.com/drivers/vcp-drivers/).

Depois de o Wi-Fi ligar, deves ver:

- **Stick:** o ecrã deixa de dizer `Wi-Fi: join "ESPHome_AC Setup"`. *No AC* é normal até estar ligado à máquina. Com Wi-Fi e sem ligação ao AC, o endereço IP fica no canto inferior direito cerca de 60 segundos. Depois essa linha passa a `No AC: swap A/B? G0 10k pull-up?` e o endereço deixa de aparecer. Essa pista quer dizer que a placa está no Wi-Fi e ainda não ouviu um AC. É o esperado antes de ligares os fios. Para leres o endereço, olha nesse primeiro minuto, ou reinicia a placa e olha outra vez antes dos 60 segundos. O Home Assistant pode descobrir a placa sem esse endereço. O registo por USB é **Connect → Logs & Console** no instalador.
- **ATOM:** o LED passa de **azul** a **vermelho** até a ligação ao AC arrancar. Vermelho com Wi-Fi e sem fios é o esperado.

## A seguir

[Ligações](wiring.md) → liga a ficha com o disjuntor desligado, depois [Home Assistant](home-assistant.md).
