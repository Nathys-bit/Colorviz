#include <math.h>   // Para roundf e fminf/fmaxf
#include <stdint.h> // Para uint8_t
#include <stdio.h> // Para printf de depuração, se necessário

// Se estiver em um arquivo separado, inclua o cabeçalho
// #include "filtros_daltonismo.h"

// --- Matrizes de Transformação ---
// RGB linear para LMS (Smith & Pokorny, 1975)
static const float sRGB_to_LMS_matrix[3][3] = {
    {0.4002f, 0.7076f, -0.0808f},
    {-0.2263f, 1.1653f, 0.0457f},
    {0.0000f, 0.0000f, 0.9182f}
};

// LMS para RGB linear (Inversa de Smith & Pokorny, 1975)
static const float LMS_to_sRGB_matrix[3][3] = {
    {1.8601f, -1.1396f, 0.2782f},
    {0.3612f, 0.6388f, 0.0000f},
    {-0.0000f, 0.0000f, 1.0890f}
};

// Matrizes de projeção para daltonismo (Brettel, Viénot & Mollon, 1999)
static const float protanopia_matrix[3][3] = {
    {0.0000f, 2.0234f, -2.5258f},
    {0.0000f, 1.0000f,  0.0000f},
    {0.0000f, 0.0000f,  1.0000f}
};

static const float deuteranopia_matrix[3][3] = {
    {1.0000f, 0.0000f, 0.0000f},
    {0.4942f, 0.0000f, 0.4854f},
    {0.0000f, 0.0000f, 1.0000f}
};

static const float tritanopia_matrix[3][3] = {
    {1.0000f,  0.0000f, 0.0000f},
    {0.0000f,  1.0000f, 0.0000f},
    {-0.0393f, 0.2319f, 0.0000f}
};

/**
 * @brief Multiplica uma matriz 3x3 por um vetor 3x1.
 * @param matrix A matriz 3x3.
 * @param vector O vetor 3x1 de entrada.
 * @param result O vetor 3x1 de saída (resultado da multiplicação).
 */
static void multiply_matrix_vector(const float matrix[3][3], const float vector[3], float result[3]) {
    for (int i = 0; i < 3; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

/**
 * @brief Aplica um filtro de daltonismo genérico (Protanopia, Deuteranopia, Tritanopia)
 * nos valores RGB.
 * @param r Ponteiro para o componente Vermelho (0-255).
 * @param g Ponteiro para o componente Verde (0-255).
 * @param b Ponteiro para o componente Azul (0-255).
 * @param daltonism_matrix A matriz de projeção específica para o tipo de daltonismo.
 */
static void aplicar_filtro_generico(uint8_t *r, uint8_t *g, uint8_t *b, const float daltonism_matrix[3][3]) {
    // 1. Converter RGB (0-255) para RGB linear (0.0-1.0)
    float r_float = (float)*r / 255.0f;
    float g_float = (float)*g / 255.0f;
    float b_float = (float)*b / 255.0f;

    r_float = (r_float <= 0.04045f) ? (r_float / 12.92f) : powf((r_float + 0.055f) / 1.055f, 2.4f);
    g_float = (g_float <= 0.04045f) ? (g_float / 12.92f) : powf((g_float + 0.055f) / 1.055f, 2.4f);
    b_float = (b_float <= 0.04045f) ? (b_float / 12.92f) : powf((b_float + 0.055f) / 1.055f, 2.4f);

    float rgb_linear[3] = {r_float, g_float, b_float};
    float lms_original[3];
    float lms_simulated[3];
    float rgb_simulated_linear[3];

    // 2. Converter RGB linear para LMS
    multiply_matrix_vector(sRGB_to_LMS_matrix, rgb_linear, lms_original);

    // 3. Aplicar a transformação de daltonismo no espaço LMS
    multiply_matrix_vector(daltonism_matrix, lms_original, lms_simulated);

    // 4. Converter LMS simulado de volta para RGB linear
    multiply_matrix_vector(LMS_to_sRGB_matrix, lms_simulated, rgb_simulated_linear);

    // 5. Clamping e conversão de volta para RGB (0-255)
    // Converte de linear para sRGB (inverso do passo 1)
    for (int i = 0; i < 3; i++) {
        float val = rgb_simulated_linear[i];
        val = (val <= 0.0031308f) ? (val * 12.92f) : (1.055f * powf(val, 1.0f / 2.4f) - 0.055f);

        // Clamping para garantir que os valores fiquem entre 0 e 1
        val = fmaxf(0.0f, fminf(1.0f, val));
        
        // Converte para 0-255 e arredonda
        if (i == 0) *r = (uint8_t)roundf(val * 255.0f);
        else if (i == 1) *g = (uint8_t)roundf(val * 255.0f);
        else *b = (uint8_t)roundf(val * 255.0f);
    }
}

// --- Funções Específicas para Cada Tipo de Daltonismo ---

void aplicar_filtro_protanopia(uint8_t *r, uint8_t *g, uint8_t *b) {
    aplicar_filtro_generico(r, g, b, protanopia_matrix);
}

void aplicar_filtro_deuteranopia(uint8_t *r, uint8_t *g, uint8_t *b) {
    aplicar_filtro_generico(r, g, b, deuteranopia_matrix);
}

void aplicar_filtro_tritanopia(uint8_t *r, uint8_t *g, uint8_t *b) {
    aplicar_filtro_generico(r, g, b, tritanopia_matrix);
}