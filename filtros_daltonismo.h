#ifndef FILTROS_DALTONISMO_H
#define FILTROS_DALTONISMO_H

#include <stdint.h> // Para uint8_t

#ifdef __cplusplus
extern "C" {
#endif

// Funções para aplicar os filtros de daltonismo
void aplicar_filtro_protanopia(uint8_t *r, uint8_t *g, uint8_t *b);
void aplicar_filtro_deuteranopia(uint8_t *r, uint8_t *g, uint8_t *b);
void aplicar_filtro_tritanopia(uint8_t *r, uint8_t *g, uint8_t *b); 
// Corrected signature, should be:
void aplicar_filtro_tritanopia(uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif

#endif // FILTROS_DALTONISMO_H