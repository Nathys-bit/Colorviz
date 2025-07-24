// identificador_cor.h
#ifndef IDENTIFICADOR_COR_H
#define IDENTIFICADOR_COR_H

#include <stdint.h> // Para uint8_t

// Estrutura para definir uma cor de referência com seu nome e valores RGB normalizados
typedef struct {
    const char *nome; // Usamos 'const char *' para strings literais na memória flash
    uint8_t r, g, b;           // Valor lido pelo sensor
    uint8_t r_ideal, g_ideal, b_ideal;  // Valor perceptivo ideal
} CorReferencia;

// Função para identificar a cor mais próxima a partir de valores RGB normalizados
/**
 * @brief Identifica o nome da cor mais próxima a partir de valores RGB normalizados.
 * @param r_norm Valor R normalizado (0-255).
 * @param g_norm Valor G normalizado (0-255).
 * @param b_norm Valor B normalizado (0-255).
 * @return Um ponteiro para uma string constante contendo o nome da cor identificada.
 * Retorna "Desconhecida" se nenhuma cor próxima for encontrada (limiar não implementado ainda).
 */
const char* identificar_cor(uint8_t *r_norm, uint8_t *g_norm, uint8_t *b_norm);
void aplicar_calibacao_rgb(uint8_t *r, uint8_t *g, uint8_t *b);
#endif // IDENTIFICADOR_COR_H