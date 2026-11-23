# Questões em aberto

São lacunas que o repositório já escreve. Esta página não as preenche.

| Tema | O que o repositório diz | Onde |
|---|---|---|
| ATOM Lite + Tail485 numa máquina real | Verificado automaticamente, não testado em hardware. | [Compatibilidade](compatibility.md) |
| Temperatura da serpentina interior, byte 21, nessa W41H1 | Ficou na temperatura da sala num Cool curto, por isso o byte pode querer dizer outra coisa nessa unidade. | A mesma secção, e [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Tensão e humidade nessa unidade | Ainda não foram enviadas. Ficam escondidas até chegar um valor real. | [Compatibilidade](compatibility.md#aeh-w41h1-unidade-do-autor) |
| AC Firmware Version (resposta `07`) | Ainda não foi capturada nenhuma resposta. O formato não está confirmado. Texto o melhor possível. A trama em bruto fica no log DEBUG. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Bytes desconhecidos da trama de estado | A identificação regista offsets da payload que não estão descodificados. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| iFeel | Só por infravermelhos (IR Kelon de 168 bits). Não está implementado neste barramento. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| **Potência de arrefecimento (BTU/h)** | Pendente. A potência de aquecimento são os bytes 55–56 de uma trama longa (150 ou 160 bytes), big-endian, em BTU/h, publicada no modo de aquecimento. Esses bytes são só de aquecimento. Próximo passo: põe o AC a arrefecer com o **Byte Hunt** ligado e a página web da placa aberta durante umas horas, descarrega o CSV e anexa-o a um issue. | [Home Assistant](home-assistant.md#potencia-de-aquecimento) |
| Capacidade nominal de chapa (BTU/h) e o modo de relatório de eletricidade | Não estão no barramento RS-485. A potência de aquecimento em funcionamento está descodificada; a capacidade de chapa e a definição da nuvem não. | [Configuração](configuration.md#entidades) |
| Ordem dos pinos da ficha | Varia com o modelo. Uma nota de hardware dá uma ordem típica de 5 V e diz para a confirmares. Mede. | [Ligações](wiring.md#1-medir-a-ficha) |
| Outras marcas | Não são suportadas. À espera de quem as teste com hardware real. | [CONTRIBUTING.md](https://github.com/skydoy/ESPHome_AC/blob/main/CONTRIBUTING.md) |
| Framework Arduino | O componente indica ESP-IDF. Arduino não foi testado. | [Configuração](configuration.md#organizacao-do-repositorio) |
| Fotos dentro de uma máquina | Os nomes dos lugares estão escritos. Não há ficheiros de foto no repositório. | [docs/photos/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/photos/README.md) |
| Modelos só reportados com outro firmware | Listados como candidatos, não confirmados aqui. | [Compatibilidade](compatibility.md#reportado-a-funcionar-com-outro-firmware-por-testar-aqui) |

