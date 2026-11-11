# Atualizações

A placa procura firmware novo ao arrancar e de 6 em 6 horas. Só aparecem releases testadas do ramo `stable`. Numa atualização de fábrica o Wi-Fi e as definições mantêm-se.

## Automáticas (imagem do instalador web)

O Home Assistant mostra **Firmware Update**. Carrega em **Install**, ou deixa o **Auto Update** ligado (configuração, predefinição ligada, mantém-se entre reinícios).

Com o **Auto Update** ligado, a placa verifica uma vez por noite às **03:MM** hora local. O minuto é aleatório, para as placas não baterem todas no GitHub ao mesmo tempo. Nunca corre nos primeiros 10 minutos depois do arranque e regista cada decisão com a etiqueta `auto_update`. Desliga-o para atualizares à mão.

Uma atualização demora cerca de 1–2 minutos. A transferência bloqueia o ciclo principal da placa, por isso o Home Assistant não mostra progresso e a placa parece desligada até reiniciar. O ecrã do Stick mostra uma barra de progresso. O registo em direto mostra `Progress: NN%` até a atualização começar.

Manifestos:

| Quem | Manifesto |
|---|---|
| Stick, gravado antes da v0.3.0 (`hisense-ac` antigo) | `https://skydoy.github.io/ESPHome_AC/firmware/manifest.json` |
| ATOM, gravado antes da v0.3.0 | `https://skydoy.github.io/ESPHome_AC/firmware/atom-lite-tail485/manifest.json` |
| Stick, instalação nova (v0.3.0+, `esphome-ac`) | `https://skydoy.github.io/ESPHome_AC/install/m5stickc-plus2/manifest.json` |
| ATOM, instalação nova (v0.3.0+) | `https://skydoy.github.io/ESPHome_AC/install/atom-lite-tail485/manifest.json` |

Os manifestos antigos continuam a servir builds `hisense-ac`. As instalações novas usam nomes `esphome-ac` e o nome amigável **Air Conditioner**. As configs adotadas no Device Builder definem elas próprias `name` e `friendly_name`. As configs de base continuam com a predefinição `hisense-ac` / `Hisense AC`.

## Device Builder (adotada)

A placa só *avisa* que há versão nova (**Update Available**, **Latest Version**, **Check for Update**, **Update Status**). Não instala nada. Carrega em **Install** → *Wirelessly* no Device Builder. O teu YAML importa `@stable`, por isso não o editas.

```yaml
packages:
  skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
  # ATOM Lite: devices/hisense-atom-lite-tail485.yaml
```

O CI move o ramo `stable` para cada release, por isso **Install** compila sempre a release mais recente. O componente `hisense_ac` dentro do pacote está fixo na tag dessa release.

O Device Builder guarda as fontes git em cache até 1 dia. Abre o menu **⋮** da placa → **Clean Build Files** e depois **Install** para ires buscar a release já.

Queres uma versão fixa? Substitui `@stable` por uma tag, por exemplo `@v0.2.12`. Evita `@main` (só desenvolvimento). Pode misturar YAML novo com um componente antigo em cache.

Adotaste antes da v0.2.9? Essas builds ainda incluem a entidade de atualização. Não carregues em *Install* nela. Carrega uma vez em **Install** no Device Builder e a entidade desaparece.

A build adotada não tem a entidade **Firmware Update** de propósito. A imagem genérica de fábrica substituiria a tua config e largaria a chave de API.

!!! warning "Não graves a imagem de fábrica por cima de uma build adotada"
    Isso apaga a tua chave de API e os extras. Vê [Segurança](security.md).

## Avisa-me

As builds adotadas podem importar o blueprint em [Home Assistant → Aviso de atualização](home-assistant.md#aviso-de-atualizacao).

Referência completa, incluindo o que o sensor **Firmware Version** mostra: [Configuração → Atualizações](configuration.md#atualizacoes).
