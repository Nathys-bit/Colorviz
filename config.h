// config.h

#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h" // Necessário para tipos como uint e para funções sleep_ms
#include "pico/time.h"   // Necessário para absolute_time_t

// --- Definições de Pinos e Constantes ---

// Configuração I2C para o sensor de cor (TCS34725)
#define I2C_PORT_COR i2c0
extern const uint I2C_SDA_PIN_COR;
extern const uint I2C_SCL_PIN_COR;

// Configuração I2C para o OLED
#define I2C_PORT_OLED i2c1
#define OLED_SDA_PIN 14
#define OLED_SCL_PIN 15

// Configuração do Joystick
#define JOYSTICK_VRY_PIN 26
#define JOYSTICK_SW_PIN 22 // GPIO22 (Digital Input para o botão de clique central do joystick)
#define BOTAO 5            // GPIO5 (Digital Input para o botão 'voltar' / Botão externo)

// Limiares para o joystick analógico (valores lidos de 0 a 4095)
#define JOYSTICK_THRESHOLD_HIGH 3500 // Leitura ADC acima deste valor (ex: para cima)
#define JOYSTICK_THRESHOLD_LOW 500  // Leitura ADC abaixo deste valor (ex: para baixo)

// Constante para o número de leituras para a média (para leitura do sensor de cor)
#define NUM_LEITURAS_MEDIA 5

// Tempo de debounce em milissegundos para botões e joystick
// Manter DEBOUNCE_MS em config.h, pois é uma constante usada para debounce.
#define DEBOUNCE_MS 200

// --- Estados do Programa (para a máquina de estados) ---
typedef enum
{
    ESTADO_MENU_DALTONISMO,      // Estado inicial: seleção do tipo de daltonismo
    ESTADO_ANALISE_PROTANOPIA,   // Estado de análise com filtro de protanopia
    ESTADO_ANALISE_DEUTERANOPIA, // Estado de análise com filtro de deuteranopia
    ESTADO_ANALISE_TRITANOPIA    // Estado de análise com filtro de tritanopia
} EstadoPrograma;

// --- Variáveis Globais (declaradas como extern para serem acessíveis externamente) ---
extern volatile EstadoPrograma estado_atual;
extern int opcao_selecionada_menu;
extern const char *menu_opcoes[]; // As opções do menu
extern const int NUM_OPCOES_MENU; // O número de opções do menu

extern volatile absolute_time_t ultimo_clique_btn5;
extern volatile bool flag; // Flag para indicar que o OLED precisa ser redesenhado, por exemplo

// --- Protótipos de Funções ---

/**
 * @brief Inicializa todos os periféricos e sensores do sistema (I2C, ADC, GPIOs, Sensores, OLED).
 */
void iniciar_sistema();

/**
 * @brief Função de callback para o botão de retorno (Botão 5).
 * Esta função é configurada como ISR para mudar o estado do programa.
 */
void btn5_callback(uint gpio, uint32_t events);

#endif // CONFIG_H