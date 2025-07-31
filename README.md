# Colorviz: sistema embarcado de percepção de cores e simulação de daltonismo

## Visão Geral do Projeto

O **Colorviz** é um projeto desenvolvido utilizando a placa de desenvolvimento **BitDogLab** com a **Raspberry Pi Pico W**integrada que combina a leitura de cores de um sensor TCS34725 com simulações visuais de diferentes tipos de daltonismo e a identificação aproximada de cores. Ele atua como um Access Point (AP) Wi-Fi, servindo uma interface web simples para visualização em tempo real das cores lidas e suas respectivas simulações para protanopia, deuteranopia e tritanopia. Além disso, o dispositivo possui um display OLED e um joystick para interação local.

O objetivo principal é oferecer uma ferramenta compacta e interativa para que pessoas a compreendam como as cores são percebidas em diferentes tipos de daltonismo, bem como identificar cores para o usuário com dicromacia.

## Funcionalidades Principais

* **Leitura de Cores:** Utiliza o sensor TCS34725 para capturar dados de cor ambiente (RGB).
* **Identificação de Cores:** Compara a cor lida com uma base de dados de cores pré-definidas para identificar o nome da cor mais próxima.
* **Simulação de Daltonismo:** Implementa o algoritmo de simulação de daltonismo de Brettel et al. (1997) para Protanopia, Deuteranopia e Tritanopia.
* **Servidor Web Integrado:** O Pico W opera como um Access Point, permitindo que dispositivos (smartphones, computadores) se conectem à sua rede e acessem uma página web simples (`http://192.168.4.1`) para visualizar os dados de cor em tempo real.
* **Interface OLED & Joystick:** Uma interface de usuário local com um display OLED para navegação em menu e um joystick para seleção de modos de daltonismo e visualização de informações.

## Hardware Necessário

* **Raspberry Pi Pico W:** O microcontrolador principal com Wi-Fi.
* **Sensor de Cor TCS34725:** Para leitura de cores (conectado via I2C).
* **Display OLED SSD1306 (128x64):** Para a interface de usuário local (conectado via I2C).
* **Módulo Joystick Analógico:** Para navegação no menu e seleção de opções.
* **Botão Externo (Opcional, GPIO5):** Para interações adicionais.
* **Jumpers/Fios:** Para as conexões.
* **Protoboard (Opcional):** Para montagem fácil.


  Pico W           TCS34725 (Sensor de Cor)
    -------------    -----------------
    GPIO 0 (SDA) --- SDA
    GPIO 1 (SCL) --- SCL
    3V3 (OUT)    --- VCC
    GND          --- GND

    Pico W           SSD1306 OLED (128x64)
    -------------    -----------------
    GPIO 14 (SDA) -- SDA
    GPIO 15 (SCL) -- SCL
    3V3 (OUT)    --- VCC
    GND          --- GND

    Pico W           Joystick Analógico
    -------------    -----------------
    GPIO 26 (ADC0) -- VRy
    GPIO 22          -- SW (Botão do Joystick)
    3V3 (OUT)    --- VCC
    GND          --- GND

    Pico W           Botão Externo (Opcional)
    -------------    -----------------
    GPIO 5           -- Um lado do botão
    GND          --- Outro lado do botão
    (Usar pull-up interno no GPIO5 via código)

## Créditos

Este projeto utiliza referências e trechos de código de outros autores para fins educacionais e de aprendizado. Abaixo estão os devidos créditos:

- Pasta `dhcpserver`: https://github.com/raspberrypi/pico-examples/tree/4c3a3dc0196dd426fddd709616d0da984e027bab/pico_w/wifi/access_point/dhcpserver
- Pasta `dnsserver`: https://github.com/raspberrypi/pico-examples/tree/4c3a3dc0196dd426fddd709616d0da984e027bab/pico_w/wifi/access_point/dnsserver
- Arquivo `tcs34725.c`: https://github.com/ASCCJR/TESTE_tcs34725_BITDOGLAB/blob/072159b1d4ce1397aea98df3dddf168564e9503f/tcs34725.c
- Arquivo `tcs34725.h`: https://github.com/ASCCJR/TESTE_tcs34725_BITDOGLAB/blob/072159b1d4ce1397aea98df3dddf168564e9503f/tcs34725.h
- Arquivo `lwipopts.h`: https://github.com/raspberrypi/pico-examples/blob/4c3a3dc0196dd426fddd709616d0da984e027bab/pico_w/wifi/lwipopts_examples_common.h

  
