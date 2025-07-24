#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>
#include "pico/sync.h" // Para mutex_t

// Variáveis globais voláteis para os dados de cor
// 'volatile' é crucial para garantir que o compilador não otimize o acesso
// a essas variáveis, pois elas podem ser alteradas por outro núcleo.
extern volatile uint8_t shared_r_corrigido;
extern volatile uint8_t shared_g_corrigido;
extern volatile uint8_t shared_b_corrigido;
extern volatile char shared_color_name[32]; // Buffer para o nome da cor
extern volatile int shared_daltonism_mode; // 0=Normal, 1=Protanopia, 2=Deuteranopia, 3=Tritanopia

// Mutex para proteger o acesso aos dados compartilhados
extern mutex_t shared_data_mutex;

#ifdef __cplusplus
extern "C" {
#endif

// Protótipo da função que será executada no Core 1 (web server)
void core1_entry(); // Nome da função atualizado para 'core1_entry'

#ifdef __cplusplus
}
#endif

#endif // SHARED_DATA_H