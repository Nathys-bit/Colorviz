// identificador_cor.c
#include "identificador_cor.h" // <<< Inclui o novo cabeçalho
#include <math.h>              // Para sqrtf (square root float)
#include <float.h>             // Para FLT_MAX (maior valor float)
#include <stdio.h>

static float cal_r_m = 1.0826f;  // Inclinação para o canal Vermelho
static float cal_r_b = -26.065f; // Intercepto para o canal Vermelho

static float cal_g_m = 1.0279f;  // Inclinação para o canal Verde
static float cal_g_b = -39.116f; // Intercepto para o canal Verde

static float cal_b_m = 1.6014f; // Inclinação para o canal Azul
static float cal_b_b = -55.049f;

// --- Base de Dados de Cores de Referência ---
// ESTA É A SEÇÃO ONDE VOCÊ VAI DEFINIR SUAS 100 CORES.
// Use valores RGB (0-255) que são *representativos* das cores que seu sensor LÊ.
// Exemplo: Se ao ler um objeto vermelho, seu sensor normalizado dá R=200, G=30, B=20, use esses.
const CorReferencia base_dados_cores[] = { 
    {"rosa choque", 191, 96, 147, 255, 20, 147},
    {"vermelho", 168, 73, 76, 255, 0, 0},
    {"vinho", 71, 66, 66, 90, 0, 0},

    {"laranja claro", 252, 193, 147, 255, 156, 64},
    {"laranja", 234, 137, 104, 255, 119, 0},
    {"laranja escuro", 206, 96, 84, 255, 77, 0},

    {"amarelo claro", 255, 255, 206, 247, 255, 99},
    {"amarelo", 255, 255, 168, 255, 255, 0},
    {"mostarda", 196, 193, 114, 205, 173, 0},

    {"verde claro", 175, 255, 239, 152, 255, 152},
    {"verde", 112, 209, 140, 7, 245, 7},
    {"verde militar", 63, 96, 76, 58, 105, 22},
    {"verde escuro", 50, 78, 232, 6, 59, 8},
    {"verde água", 99, 186, 153, 21, 189, 116},

    {"azul bebe", 155, 255, 255, 135, 206, 235},
    {"azul ceu", 71, 147, 232, 0, 94, 255},
    {"azul marinho", 40, 84, 127, 0, 0, 128},
    {"azul petroleo", 45, 71, 79, 3, 17, 41},

    {"lilas", 188, 242, 255, 200, 162, 200},
    {"roxo", 96, 91, 147, 128, 0, 128},
    {"violeta", 58, 86, 112, 58, 23, 87},

    {"cinza claro", 173, 249, 252, 172, 176, 174},
    {"cinza", 81, 119, 117, 87, 97, 88},
    {"cinza escuro", 63, 91, 94, 49, 59, 59},
    {"preto", 40, 61, 58, 0, 0, 0},

    {"areia", 219, 255, 232, 222, 203, 164},
    {"marrom", 89, 84, 71, 139, 69, 19},
    {"marrom escuro", 58, 73, 68, 79, 44, 21},

    {"rosa chiclete", 211, 150, 193, 255, 105, 180},
    {"rosa bebe", 245, 245, 235, 247, 181, 173},
    {"branco", 255, 255, 255, 255, 255, 255}

};

// Calcula o número de cores na base de dados automaticamente
const int NUM_CORES_NA_BASE_DADOS = sizeof(base_dados_cores) / sizeof(CorReferencia); // <<< Renomeado

// Função para identificar a cor mais próxima
const char *identificar_cor(uint8_t *r_norm, uint8_t *g_norm, uint8_t *b_norm)
{
    float menor_distancia = FLT_MAX;
    const char *nome_cor_identificada = "Desconhecida";
    int indice_cor_mais_proxima = -1;

    for (int i = 0; i < NUM_CORES_NA_BASE_DADOS; i++)
    {
        float dr = (float)(*r_norm) - base_dados_cores[i].r;
        float dg = (float)(*g_norm) - base_dados_cores[i].g;
        float db = (float)(*b_norm) - base_dados_cores[i].b;

        float distancia = sqrtf(dr * dr + dg * dg + db * db);

        if (distancia < menor_distancia)
        {
            menor_distancia = distancia;
            nome_cor_identificada = base_dados_cores[i].nome;
            indice_cor_mais_proxima = i;
        }
    }

    if (indice_cor_mais_proxima >= 0)
    {
        *r_norm = base_dados_cores[indice_cor_mais_proxima].r_ideal;
        *g_norm = base_dados_cores[indice_cor_mais_proxima].g_ideal;
        *b_norm = base_dados_cores[indice_cor_mais_proxima].b_ideal;
    }

    return nome_cor_identificada;
}

void aplicar_calibacao_rgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Aplica a função linear (y = m*x + b) e garante que o valor permaneça entre 0 e 255.
    *r = (uint8_t)fminf(255.0f, fmaxf(0.0f, (*r * cal_r_m) + cal_r_b));
    *g = (uint8_t)fminf(255.0f, fmaxf(0.0f, (*g * cal_g_m) + cal_g_b));
    *b = (uint8_t)fminf(255.0f, fmaxf(0.0f, (*b * cal_b_m) + cal_b_b));
}