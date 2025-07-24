#include <stdio.h>
#include <string.h> // Adicionado para memset
#include <stdlib.h> // Para funções de memória, embora o driver OLED já gerencie
#include <ctype.h>  // Para isalnum etc., se usado em alguma função de texto
#include <math.h>   // Para fminf na normalização RGB

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h" // Adicionado para configurar interrupções GPIO

// --- Inclusões dos Drivers e Módulos ---
#include "tcs34725.h"          // Driver do sensor de cor TCS34725
#include "identificador_cor.h" // Módulo de identificação de cor
#include "filtros_daltonismo.h"
#include "config.h"

// Inclusões para o OLED (baseadas nos arquivos ssd1306.h e ssd1306_i2c.h fornecidos)
#include "inc/ssd1306.h"     // Contém ssd1306_init, ssd1306_draw_string, etc.
#include "inc/ssd1306_i2c.h" // Contém as definições globais como ssd1306_buffer e ssd1306_buffer_length

#include "pico/multicore.h" // Para multicore_launch_core1
#include "pico/sync.h"      // Para mutex_t
#include "shared_data.h"

extern uint8_t ssd1306_buffer[]; // Declaração do buffer global do driver OLED

// A área de renderização do OLED também pode ser global
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

volatile absolute_time_t ultimo_clique_joystick = {0};

// --- Declarações de Funções Auxiliares (que permanecem no main.c) ---
void normalizar_rgb(uint16_t r_bruto, uint16_t g_bruto, uint16_t b_bruto,
                    uint8_t *r_normalizado, uint8_t *g_normalizado, uint8_t *b_normalizado);
int ler_adc(uint gpio_pin);
void limpar_oled();
void desenhar_menu_daltonismo();
void leitura_media_cor(uint8_t *r_out, uint8_t *g_out, uint8_t *b_out);
void desenhar_tela_analise(const char *tipo_daltonismo, const char *nome_cor, uint8_t r, uint8_t g, uint8_t b);

void normalizar_rgb(uint16_t r_bruto, uint16_t g_bruto, uint16_t b_bruto,
                    uint8_t *r_normalizado, uint8_t *g_normalizado, uint8_t *b_normalizado)
{

    float max_bruto_base = 500.0f; // Valor de referência máximo do sensor para um ambiente específico

    // Ajuste do fator de luz para escalar o valor máximo bruto baseado na luminosidade atual
    float max_bruto_para_ambiente_atual = 100.0f;

    // Normaliza cada canal para o intervalo 0-255, garantindo que não exceda 255
    *r_normalizado = (uint8_t)fminf(255.0f, (r_bruto / max_bruto_para_ambiente_atual) * 255.0f);
    *g_normalizado = (uint8_t)fminf(255.0f, (g_bruto / max_bruto_para_ambiente_atual) * 255.0f);
    *b_normalizado = (uint8_t)fminf(255.0f, (b_bruto / max_bruto_para_ambiente_atual) * 255.0f);
}

/**
 * @brief Lê o valor de um pino ADC específico.
 * @param gpio_pin O número do pino GPIO configurado para ADC (ex: 27).
 * @return O valor lido pelo ADC (0-4095).
 */
int ler_adc(uint gpio_pin)
{
    adc_select_input(gpio_pin - 26); // Mapeia GPIO 26 para ADC0, 27 para ADC1, etc.
    return adc_read();
}

/**
 * @brief Verifica se o botão de clique central do joystick (GPIO22) foi pressionado, com debounce.
 * @return true se o botão foi pressionado, false caso contrário.
 */
bool joystick_botao_pressionado()
{
    // Para debounce, verificamos se passou tempo suficiente desde o último evento
    if (absolute_time_diff_us(get_absolute_time(), ultimo_clique_joystick) < DEBOUNCE_MS * 1000)
    {
        return false;
    }
    // Se o botão está pressionado (pino LOW, assumindo pull-up)
    if (gpio_get(JOYSTICK_SW_PIN) == 0)
    {
        ultimo_clique_joystick = get_absolute_time(); // Atualiza o último tempo de clique
        return true;
    }
    return false;
}

/**
 * @brief Verifica o movimento do joystick no eixo Y (para cima/baixo), com debounce.
 * @return -1 para "cima", 1 para "baixo", 0 para "nenhum movimento significativo".
 */
int joystick_eixo_y()
{
    uint16_t val = ler_adc(JOYSTICK_VRY_PIN);
    if (val < 500)
        return -1;
    else if (val > 3500)
        return 1;
    return 0;
}

/**
 * @brief Desenha o menu de seleção de tipo de daltonismo na tela OLED.
 */
void desenhar_menu_daltonismo()
{
    limpar_oled();
    ssd1306_draw_string(ssd1306_buffer, 0, 0, "Selecione o Tipo:");

    for (int i = 0; i < NUM_OPCOES_MENU; i++)
    {
        char buffer[30];
        // Adiciona um '>' na frente da opção selecionada
        sprintf(buffer, "%s %s", (i == opcao_selecionada_menu ? "--" : " "), menu_opcoes[i]);
        ssd1306_draw_string(ssd1306_buffer, 0, 10 + (i * 8), buffer);
    }
    ssd1306_send_buffer(ssd1306_buffer, ssd1306_buffer_length);
}

/**
 * @brief Desenha a tela de análise de cor na tela OLED.
 * @param tipo_daltonismo String que descreve o modo de daltonismo.
 * @param nome_cor String com o nome da cor identificada.
 * @param r Valor do canal vermelho (0-255).
 * @param g Valor do canal verde (0-255).
 * @param b Valor do canal azul (0-255).
 */
void desenhar_tela_analise(const char *tipo_daltonismo, const char *nome_cor, uint8_t r, uint8_t g, uint8_t b)
{
    limpar_oled(); // Limpa o buffer do OLED

    char buffer[40]; // Buffer para as strings

    // Modo de daltonismo
    sprintf(buffer, "%s", tipo_daltonismo);
    ssd1306_draw_string(ssd1306_buffer, 0, 0, buffer);

    // Cor Identificada (Nome)
    sprintf(buffer, "%s", nome_cor);
    ssd1306_draw_string(ssd1306_buffer, 0, 16, buffer); 

    // Valores RGB Transformados (Hexadecimal)
    // Usar "%02X" para formatar como hexadecimal com 2 dígitos e preenchimento com zero
    sprintf(buffer, "HEX: #%02X%02X%02X", r, g, b);     
    ssd1306_draw_string(ssd1306_buffer, 0, 32, buffer); 

    ssd1306_draw_string(ssd1306_buffer, 0, 56, "Voltar: btn 5");
    ssd1306_send_buffer(ssd1306_buffer, ssd1306_buffer_length);
}

void leitura_media_cor(uint8_t *r_out, uint8_t *g_out, uint8_t *b_out)
{
    // Variável local para armazenar os dados brutos de uma única leitura do sensor TCS34725.
    // 'tcs34725_color_data_t' é uma estrutura definida pelo driver 'tcs34725.h'.
    tcs34725_color_data_t dados_sensor_brutos;

    uint32_t acumulador_red_leitura = 0;
    uint32_t acumulador_green_leitura = 0;
    uint32_t acumulador_blue_leitura = 0;
    // Realiza a leitura do sensor TCS34725.
    for (int i = 0; i < NUM_LEITURAS_MEDIA; i++)
    {
        // Realiza uma única leitura do sensor TCS34725.
        tcs34725_read_colors(I2C_PORT_COR, &dados_sensor_brutos);
        // Acumula os valores brutos de cada leitura nos acumuladores globais.
        acumulador_red_leitura += dados_sensor_brutos.red;
        acumulador_green_leitura += dados_sensor_brutos.green;
        acumulador_blue_leitura += dados_sensor_brutos.blue;

        sleep_ms(10);
    }

    uint32_t red_media = acumulador_red_leitura / NUM_LEITURAS_MEDIA;
    uint32_t green_media = acumulador_green_leitura / NUM_LEITURAS_MEDIA;
    uint32_t blue_media = acumulador_blue_leitura / NUM_LEITURAS_MEDIA;

    // --- Normalizar e Armazenar Resultados ---
    // Chama a função 'normalizar_rgb' (sua versão), passando as médias calculadas.
    // 'r_out', 'g_out', 'b_out' são ponteiros para onde os valores normalizados (0-255) serão gravados.
    // É importante passar r_out, g_out, b_out DIRETAMENTE aqui, pois eles JÁ SÃO ponteiros.
    normalizar_rgb(red_media, green_media, blue_media, r_out, g_out, b_out);
}

int main()
{
    iniciar_sistema(); // Inicializa todos os componentes
    mutex_init(&shared_data_mutex);
    multicore_launch_core1(core1_entry);
    printf("Core 1 lançado com a função core1_entry().\n");

    desenhar_menu_daltonismo(); // Desenha o menu inicial no display OLED uma vez

    while (1) // Loop principal do programa
    {

        uint8_t r_norm, g_norm, b_norm;
        leitura_media_cor(&r_norm, &g_norm, &b_norm);

        uint8_t r_corrigido = r_norm; // Estes serão os valores R, G, B que poderão ser filtrados
        uint8_t g_corrigido = g_norm;
        uint8_t b_corrigido = b_norm;
        const char *nome_cor_identificada = identificar_cor(&r_corrigido, &g_corrigido, &b_corrigido);

        printf("\nRGB Normalizada: R:%3u G:%3u B:%3u | ", r_corrigido, g_corrigido, b_corrigido);
        printf("\nRGB Cor sensor le: R:%3u G:%3u B:%3u | ", r_norm, g_norm, b_norm);
        
        switch (estado_atual)
        {
        case ESTADO_MENU_DALTONISMO:
        {
            uint16_t val_y = ler_adc(JOYSTICK_VRY_PIN);
            if (flag)
            {
                desenhar_menu_daltonismo();
                flag = false;
            }
            if (val_y < 1000)
            { // Joystick para cima
                opcao_selecionada_menu--;
                if (opcao_selecionada_menu < 0)
                {
                    opcao_selecionada_menu = NUM_OPCOES_MENU - 1;
                }
                desenhar_menu_daltonismo();
                sleep_ms(50); // Debounce simples
            }
            else if (val_y > 3000)
            { // Joystick para baixo
                opcao_selecionada_menu++;
                if (opcao_selecionada_menu >= NUM_OPCOES_MENU)
                {
                    opcao_selecionada_menu = 0;
                }
                desenhar_menu_daltonismo();
                sleep_ms(50); // Debounce simples
            }

            if (gpio_get(JOYSTICK_SW_PIN) == 0)
            {                 // Botão do joystick pressionado
                sleep_ms(50); // Debounce do botão
                // printf("Opcao selecionada: %s\n", menu_opcoes[opcao_selecionada_menu]);
                switch (opcao_selecionada_menu)
                {
                case 0:
                    estado_atual = ESTADO_ANALISE_PROTANOPIA;
                    break;
                case 1:
                    estado_atual = ESTADO_ANALISE_DEUTERANOPIA;
                    break;
                case 2:
                    estado_atual = ESTADO_ANALISE_TRITANOPIA;
                    break;
                }
                desenhar_tela_analise(menu_opcoes[opcao_selecionada_menu], nome_cor_identificada, r_corrigido, g_corrigido, b_corrigido);
            }
            break;
        }

        case ESTADO_ANALISE_PROTANOPIA:
        {
            desenhar_tela_analise(menu_opcoes[opcao_selecionada_menu], nome_cor_identificada, r_corrigido, g_corrigido, b_corrigido);
            aplicar_filtro_protanopia(&r_corrigido, &g_corrigido, &b_corrigido);

            mutex_enter_blocking(&shared_data_mutex);
            shared_r_corrigido = r_corrigido;
            shared_g_corrigido = g_corrigido;
            shared_b_corrigido = b_corrigido;
            strncpy((char *)shared_color_name, nome_cor_identificada, sizeof(shared_color_name) - 1);
            ((char *)shared_color_name)[sizeof(shared_color_name) - 1] = '\0';
            shared_daltonism_mode = 1; // 1 para Protanopia
            mutex_exit(&shared_data_mutex);

            break;
        }
        case ESTADO_ANALISE_DEUTERANOPIA:
        {
            desenhar_tela_analise(menu_opcoes[opcao_selecionada_menu], nome_cor_identificada, r_corrigido, g_corrigido, b_corrigido);
            aplicar_filtro_deuteranopia(&r_corrigido, &g_corrigido, &b_corrigido);
            
            mutex_enter_blocking(&shared_data_mutex);
            shared_r_corrigido = r_corrigido;
            shared_g_corrigido = g_corrigido;
            shared_b_corrigido = b_corrigido;
            strncpy((char *)shared_color_name, nome_cor_identificada, sizeof(shared_color_name) - 1);
            ((char *)shared_color_name)[sizeof(shared_color_name) - 1] = '\0';
            shared_daltonism_mode = 2; // 2 para Deuteranopia
            mutex_exit(&shared_data_mutex);

            break;
        }
        case ESTADO_ANALISE_TRITANOPIA:
        {
            desenhar_tela_analise(menu_opcoes[opcao_selecionada_menu], nome_cor_identificada, r_corrigido, g_corrigido, b_corrigido);
            aplicar_filtro_tritanopia(&r_corrigido, &g_corrigido, &b_corrigido);
           
            mutex_enter_blocking(&shared_data_mutex);
            shared_r_corrigido = r_corrigido;
            shared_g_corrigido = g_corrigido;
            shared_b_corrigido = b_corrigido;
            strncpy((char *)shared_color_name, nome_cor_identificada, sizeof(shared_color_name) - 1);
            ((char *)shared_color_name)[sizeof(shared_color_name) - 1] = '\0';
            shared_daltonism_mode = 3; // 3 para Tritanopia
            mutex_exit(&shared_data_mutex);

            break;
        }
        }

        // Imprime informações no monitor serial para depuração (ainda útil!)
        printf("Estado: %s ", (estado_atual == ESTADO_MENU_DALTONISMO ? "MENU" : menu_opcoes[opcao_selecionada_menu]));
        printf("Cor Identificada: %s\n", nome_cor_identificada);

        sleep_ms(50); // Pequeno atraso para não sobrecarregar o loop e a atualização da tela
    }
    return 0;
}