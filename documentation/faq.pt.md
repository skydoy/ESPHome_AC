# FAQ

## Que placas são suportadas?

**M5StickC Plus2 + RS485 Hat** e **ATOM Lite + ATOM Tail485**, numa unidade interior Hisense (ou no mesmo barramento) cujo módulo Wi-Fi usa o conector RS-485 de 4 pinos (**AEH-W4x / AEH-W41H1**). A versão Stick é a que foi testada numa máquina real. A versão ATOM ainda não foi testada em hardware. [Compatibilidade](compatibility.md) · [Primeiros passos](getting-started.md).

## Preciso de YAML?

Não. Usa o [instalador web](https://skydoy.github.io/ESPHome_AC/). O YAML é opcional se adotares a placa no ESPHome Device Builder ou se compilares a tua. [Configuração](configuration.md).

## Porque é que tem de ser Wi-Fi 2,4 GHz?

O instalador e o guia rápido pedem uma rede de **2,4 GHz**. O repositório não documenta um caminho de 5 GHz para estas placas.

## O que é a rede `ESPHome_AC Setup`?

Uma rede Wi-Fi de recurso que a placa abre quando não tem rede guardada, ou não consegue chegar-lhe. Liga-te a ela com o telemóvel para configurares o Wi-Fi. É aberta, e só está ativa enquanto o Wi-Fi está em baixo. Também podes usar Bluetooth Improv a partir da app do Home Assistant. [Instalar](install.md) · [Segurança](security.md).

## Posso usar isto comercialmente?

Não. O projeto é [PolyForm Noncommercial 1.0.0](licence.md): uso pessoal e não comercial. O uso comercial não é permitido.

## Onde estão as capturas da página web da placa?

[Página web da placa](web-page.md). Ainda não há fotos de uma placa dentro de um ar condicionado no repositório. [Questões em aberto](open-questions.md).

## Algo correu mal — onde olho?

[Problemas](troubleshooting.md) · [Issues no GitHub](https://github.com/skydoy/ESPHome_AC/issues/new/choose)
