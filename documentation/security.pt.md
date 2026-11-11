# Segurança e atualizações pelo ar

## Firmware pré-compilado (instalador web)

A imagem do [instalador web](https://skydoy.github.io/ESPHome_AC/) é a mesma para toda a gente, por isso **não pode conter as tuas chaves nem palavras-passe**. Vem assim:

| Item | Firmware pré-compilado | Porquê |
|---|---|---|
| Chave de encriptação da API do Home Assistant | nenhuma | Qualquer Home Assistant na tua rede a adota com um clique. |
| Palavra-passe OTA (ESPHome) | nenhuma | A mesma imagem pública para todos. |
| Rede de recurso (`ESPHome_AC Setup`) | aberta, só enquanto o Wi-Fi não está ligado | Para configurares o Wi-Fi no telemóvel. |
| Página web da placa (`http://<ip>/`) | **sem palavra-passe** (só na LAN) | De propósito, para controlo local e registos em direto. Quem estiver na tua rede consegue ver e controlar o AC. |
| Atualizações de firmware | **Firmware Update** + **Auto Update** | Descarregadas por HTTPS do GitHub Pages deste projeto. O manifesto traz um MD5 que a placa confirma antes de reiniciar. |

Numa rede doméstica de confiança, esta é a configuração habitual dos firmwares ESPHome prontos a usar. Se outras pessoas ou dispositivos partilharem essa rede, protege-a como se explica a seguir, ou mete o AC numa VLAN de confiança.

## OTA de fábrica vs Device Builder

- **Imagem do instalador / fábrica:** o Home Assistant mostra **Firmware Update**. Carrega em **Install**, ou deixa o **Auto Update** ligado para instalar de madrugada. O Wi-Fi mantém-se.
- **Adotada no ESPHome Device Builder:** a placa só *avisa* que há versão nova (**Update Status**, **Check for Update**). Carrega em **Install** no Device Builder. A config já segue a `@stable`. **Não** instales a imagem genérica de fábrica por cima de uma build adotada. Isso apaga a tua chave de API e os extras.

Detalhes: [Atualizações](updates.md) e [Configuração → Atualizações](configuration.md#atualizacoes).

## Acrescentar uma chave de API (recomendado)

Adota a placa no ESPHome Device Builder e acrescenta ao YAML (e aos Secrets):

```yaml
api:
  encryption:
    key: !secret api_key   # openssl rand -base64 32

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ap:
    password: !secret ap_password   # pelo menos 8 caracteres
```

Instala com **Install → Wirelessly**. O Home Assistant pede a mesma `api_key`.
Passo a passo: [Encriptação da API e palavra-passe OTA](configuration.md#encriptacao-da-api-e-palavra-passe-ota-opcional).

Para desligares ou protegeres a página web da placa, vê [Servidor web](configuration.md#servidor-web).

## Reportar uma vulnerabilidade

Vê o [SECURITY.md](https://github.com/skydoy/ESPHome_AC/blob/main/SECURITY.md). **Não** abras um issue público para problemas de segurança.

A tabela de versões suportadas nesse ficheiro ainda diz que a mais recente é `0.2.x`. A secção de release mais nova no changelog é `0.3.9` (2026-11-09). Estas páginas não alteram essa política. Vê [Questões em aberto](open-questions.md#tabela-de-versoes-da-politica-de-seguranca).

O guia no GitHub: [docs/seguranca.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/seguranca.md).
