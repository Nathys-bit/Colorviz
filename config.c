// config.c

#include "config.h"       // Inclui o próprio cabeçalho
#include "tcs34725.h"     // Driver do sensor de cor TCS34725
#include "inc/ssd1306.h"  // Driver do OLED SSD1306
#include "inc/ssd1306_i2c.h" // Driver OLED para I2C

#include "hardware/adc.h" // Necessário para adc_init() e adc_gpio_init()
#include "hardware/gpio.h" // Já deve estar presente, mas garante funções GPIO
#include "hardware/i2c.h"  // Já deve estar presente, mas garante funções I2C

#include <stdio.h>    // Para printf
#include <string.h>   // Para memset, se usado

// --- Definição das Variáveis Globais (sem 'extern' aqui, pois esta é a definição) ---
const uint I2C_SDA_PIN_COR = 0;
const uint I2C_SCL_PIN_COR = 1;

volatile EstadoPrograma estado_atual = ESTADO_MENU_DALTONISMO; // O programa sempre começa no menu
int opcao_selecionada_menu = 0;                                           // Índice da opção atualmente selecionada (0 = "Protanopia")

const char *menu_opcoes[] = {
    "Protanopia",
    "Deuteranopia",
    "Tritanopia"
};
const int NUM_OPCOES_MENU = sizeof(menu_opcoes) / sizeof(menu_opcoes[0]);

volatile absolute_time_t ultimo_clique_btn5 = {0}; // Inicializa aqui
volatile bool flag = false;

// --- Implementação da Função de Callback para o Botão 5 ---
// Esta função é chamada quando o botão GPIO5 é pressionado (borda de descida).
void btn5_callback(uint gpio, uint32_t events)
{
    // Para debounce, verificamos se passou tempo suficiente desde o último evento válido
    if (absolute_time_diff_us(ultimo_clique_btn5, get_absolute_time()) < DEBOUNCE_MS * 1000)
    {
        // Se não passou tempo suficiente, é um "bounce" do botão. Ignore o evento.
        return;
    }
    ultimo_clique_btn5 = get_absolute_time(); // Atualiza o último tempo de clique válido
    estado_atual = ESTADO_MENU_DALTONISMO;   // Volta para o menu principal
    flag = true; // Sinaliza que a tela do OLED precisa ser atualizada (redesenhar o menu)
}

// --- Implementação da Função de Inicialização do Sistema ---
void iniciar_sistema()
{
    stdio_init_all(); // Inicializa USB serial
    printf("--- Iniciando Sistema Colorviz ---\n");

    // Inicializa o tempo do último clique do botão para o debounce
    ultimo_clique_btn5 = get_absolute_time();

    // --- Inicialização do I2C para Sensores de Cor e Luz ---
    i2c_init(I2C_PORT_COR, 100 * 1000); // Frequência de 100 kHz
    gpio_set_function(I2C_SDA_PIN_COR, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN_COR, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN_COR);
    gpio_pull_up(I2C_SCL_PIN_COR);

    // --- Inicialização do I2C para o OLED ---
    // A frequência do OLED é definida em ssd1306_i2c.h (ssd1306_i2c_clock)
    i2c_init(I2C_PORT_OLED, ssd1306_i2c_clock * 1000);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    // --- Inicialização do Joystick (ADC e GPIOs digitais) ---
    adc_init();
    adc_gpio_init(JOYSTICK_VRY_PIN); // Apenas o Eixo Y

    gpio_init(JOYSTICK_SW_PIN);
    gpio_set_dir(JOYSTICK_SW_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_SW_PIN); // Ativa pull-up para o botão do joystick

    // --- Configuração do Botão Externo (BOTAO / GPIO5) com Interrupção (Callback) ---
    gpio_init(BOTAO);
    gpio_set_dir(BOTAO, GPIO_IN);
    gpio_pull_up(BOTAO); // Ativa o pull-up interno para o botão externo

    // Configura a interrupção para borda de descida (quando o botão é pressionado)
    gpio_set_irq_enabled_with_callback(BOTAO, GPIO_IRQ_EDGE_FALL, true, &btn5_callback);

    // --- Inicialização do Sensor de Cor TCS34725 ---
    uint8_t meu_atime = 0xEB; // Tempo de integração (ajuste conforme necessário)
    uint8_t meu_ganho = 0x00; // Ganho (0x00 = 1x, 0x01 = 4x, 0x02 = 16x, 0x03 = 60x)

    printf("Inicializando sensor de cor TCS34725...\n");
    if (!tcs34725_init(I2C_PORT_COR, meu_atime, meu_ganho))
    {
        printf("ERRO: Falha ao inicializar o sensor TCS34725.\n");
        // Em um sistema real, você pode querer adicionar um LED de erro ou travar aqui.
        while (1);
    }

    // --- Inicialização do OLED SSD1306 ---
    ssd1306_init();
    printf("Display OLED SSD1306 inicializado na I2C1.\n");

    // Limpa o OLED inicialmente (o buffer é definido externamente pelo driver)
    // Usamos ssd1306_buffer, o buffer global do driver
    memset(ssd1306_buffer, 0, SSD1306_BUFFER_LENGTH);
    ssd1306_send_buffer(ssd1306_buffer, SSD1306_BUFFER_LENGTH);

    printf("Sistema pronto.\n");
}