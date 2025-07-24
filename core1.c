#include <string.h>
#include <stdio.h>
#include <stdlib.h> // Para snprintf

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/sync.h"   // Para mutex

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/err.h"  
#include "lwip/netif.h" 

#include "dhcpserver.h" // Seus arquivos de DHCP/DNS
#include "dnsserver.h"  // Seus arquivos de DHCP/DNS

// Incluir o cabeçalho com as variáveis compartilhadas
#include "shared_data.h"

// --- Definições do Access Point (AP) ---
// Você pode mudar esses valores para o nome e senha da sua rede Wi-Fi que o Pico vai criar.
#define AP_NAME "Coresenxergo_AP"     // Nome do seu Access Point
#define AP_PASSWORD "12345678" // Senha (mínimo 8 caracteres para WPA2)

#define TCP_PORT 80
#define DEBUG_printf printf

// --- Variáveis globais para os dados compartilhados (definições) ---
// Estes são os locais de armazenamento reais das variáveis declaradas como 'extern' em shared_data.h
volatile uint8_t shared_r_corrigido = 0;
volatile uint8_t shared_g_corrigido = 0;
volatile uint8_t shared_b_corrigido = 0;
volatile char shared_color_name[32] = "Aguardando Leitura..."; // Valor inicial
volatile int shared_daltonism_mode = 0; // 0=Normal

// Definição do mutex
mutex_t shared_data_mutex;

// --- Estruturas e Funções do Servidor TCP (adaptadas de picow_access_point.c) ---
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *tcp_server_pcb;
    struct tcp_pcb *tcp_client_pcb;
    bool complete;
    ip_addr_t gw;
    ip_addr_t netmask;
} TCP_SERVER_T;

static TCP_SERVER_T *tcp_server_state;

static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK; 

    if (state->tcp_client_pcb) {
        tcp_arg(state->tcp_client_pcb, NULL);
        tcp_sent(state->tcp_client_pcb, NULL);
        tcp_recv(state->tcp_client_pcb, NULL);
        tcp_err(state->tcp_client_pcb, NULL);
        tcp_poll(state->tcp_client_pcb, NULL, 0); // Desabilita o polling para este PCB de cliente

        err = tcp_close(state->tcp_client_pcb); // Fecha APENAS o PCB do cliente
        if (err != ERR_OK) {
            DEBUG_printf("Failed to close client pcb %d\n", err);
            tcp_abort(state->tcp_client_pcb); // Abortar se fechar falhar (isto é para tratamento de erros)
            return ERR_ABRT; // Use ERR_ABRT para abort, se aplicável
        }
        state->tcp_client_pcb = NULL; // Limpa o ponteiro para o cliente
    }

    return err; // Retorna o status de fechamento do cliente
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_sent %u (dados enviados e reconhecidos)\n", len);

    // Após todos os dados terem sido escritos e reconhecidos, fechamos a conexão.
    // Este callback é acionado *após* o tcp_write anterior ter sido ACKed.
    // Como estamos enviando toda a resposta HTTP em tcp_server_recv de uma vez,
    // este `tcp_server_sent` significa que a resposta foi enviada e reconhecida.
    return tcp_server_close(state);
}

static void tcp_server_error(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_server_error %d\n", err);
    }
}

// Função para gerar o HTML dinamicamente
char* generate_color_html() {
    static char http_response_content[512]; // Buffer para a resposta HTML
    uint8_t r, g, b;
    char color_name[32];
    int daltonism_mode;
    const char* daltonism_type_str = "";

    // Proteger acesso aos dados compartilhados ao ler
    mutex_enter_blocking(&shared_data_mutex);
    r = shared_r_corrigido;
    g = shared_g_corrigido;
    b = shared_b_corrigido;
    strncpy(color_name, (const char*)shared_color_name, sizeof(color_name) - 1);
    color_name[sizeof(color_name) - 1] = '\0'; // Garantir terminação nula
    daltonism_mode = shared_daltonism_mode;
    mutex_exit(&shared_data_mutex);

    // Mapear o modo de daltonismo para uma string legível
    switch (daltonism_mode) {
        case 1: daltonism_type_str = "Protanopia"; break;
        case 2: daltonism_type_str = "Deuteranopia"; break;
        case 3: daltonism_type_str = "Tritanopia"; break;
        default: daltonism_type_str = "Normal"; break; // Caso 0 ou outro valor inesperado
    }

    // HTML muito simples, sem CSS externo, usando inline style para a cor de fundo.
    // O meta refresh recarrega a página a cada segundo para exibir a cor mais recente.
    snprintf(http_response_content, sizeof(http_response_content),
             "<html>"
             "<head>"
             "<title>Coresenxergo</title>"
             "<meta http-equiv=\"refresh\" content=\"2\">"
             "</head>"
             "<body style=\"background-color:rgb(%u,%u,%u); color:white; text-align:center; font-family:sans-serif;\">"
             "<h1>Colorviz: Simulação de Daltonismo</h1>"
             "<p>Modo de visualização: <b>%s</b></p>"
             "<p>Cor Identificada: <b>%s</b></p>"
             "<p>RGB: (%u, %u, %u)</p>"
             "</body>"
             "</html>",
             r, g, b, daltonism_type_str, color_name, r, g, b);

    return http_response_content;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        DEBUG_printf("tcp_server_recv: No pbuf, closing connection.\n");
        return tcp_server_close(arg);
    }
    // Indicate that the pbuf has been received
    tcp_recved(tpcb, p->tot_len);

    if (err == ERR_OK) {
        char *req_data = (char *)p->payload;
        int req_len = p->tot_len;

        // Simplificado: Assumimos que qualquer requisição é um GET e respondemos com a cor HTML.
        // Em um servidor real, você verificaria o método HTTP e o caminho da URL.
        if (req_len >= 3 && strncmp(req_data, "GET", 3) == 0) {
            char *response_body = generate_color_html();
            int body_len = strlen(response_body);

            char http_response_headers[256];
            snprintf(http_response_headers, sizeof(http_response_headers),
                     "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n",
                     body_len);

            // Envia cabeçalhos e corpo
            err = tcp_write(tpcb, http_response_headers, strlen(http_response_headers), 1);
            if (err == ERR_OK) {
                err = tcp_write(tpcb, response_body, body_len, 1);
            }
            if (err == ERR_OK) {
                tcp_output(tpcb); // Força o envio dos dados
            }
        }
    } else {
        DEBUG_printf("tcp_server_recv error %d\n", err);
    }
    pbuf_free(p); // Liberar o pbuf
    return ERR_OK;
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("tcp_server_accept error %d\n", err);
        return ERR_VAL;
    }
    DEBUG_printf("TCP client accepted\n");
    state->tcp_client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_err(client_pcb, tcp_server_error);
    tcp_sent(client_pcb, tcp_server_sent);
    return ERR_OK;
}

static bool tcp_server_open(void *arg, const char *ap_name) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
 //   DEBUG_printf("Starting server at %s on port %d\n", ip4addr_ntoa(netif_ip4_addr(netif_get_default())), TCP_PORT);

    struct tcp_pcb *tpcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!tpcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(tpcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n", err);
        return false;
    }

    state->tcp_server_pcb = tcp_listen_with_backlog(tpcb, 5); // Permite mais conexões pendentes
    if (!state->tcp_server_pcb) {
        DEBUG_printf("failed to listen\n");
        return false;
    }

    tcp_arg(state->tcp_server_pcb, state);
    tcp_accept(state->tcp_server_pcb, tcp_server_accept);

    return true;
}

// --- Função Principal do Core 1 ---
void core1_entry() {
    // Inicializa o mutex para dados compartilhados (somente uma vez, idealmente no main)
    // No entanto, é seguro inicializar aqui também se o main garantir que não há raça.
    // Para simplificar, vamos inicializar aqui, mas o main também o fará.
    mutex_init(&shared_data_mutex);
    // Inicialização do Wi-Fi
    if (cyw43_arch_init()) {
        printf("ERRO: Falha ao inicializar o Wi-Fi (CYW43).\n");
        return;
    }

    cyw43_arch_enable_ap_mode(AP_NAME, AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    // Configura IP estático para o AP
    ip4_addr_t gw, netmask;
    IP4_ADDR(&gw, 192, 168, 4, 1); // IP do Access Point
    IP4_ADDR(&netmask, 255, 255, 255, 0);

    tcp_server_state = calloc(1, sizeof(TCP_SERVER_T));
    if (!tcp_server_state) {
        DEBUG_printf("Falha ao alocar memória para o estado do servidor TCP.\n");
        return;
    }
    tcp_server_state->gw = gw;
    tcp_server_state->netmask = netmask;

    // Inicia o servidor DHCP (para dar IPs aos clientes)
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gw, &netmask);

    // Inicia o servidor DNS (opcional, mas bom para resolução de nomes)
    dns_server_t dns_server;
    dns_server_init(&dns_server, &gw);

    if (!tcp_server_open(tcp_server_state, AP_NAME)) {
        DEBUG_printf("Falha ao abrir o servidor TCP.\n");
        return;
    }

    printf("Servidor web no Core 1 pronto! Conecte-se à rede '%s' e acesse http://192.168.4.1\n", AP_NAME);

    while (true) {
        // Este loop é essencial para o polling do Wi-Fi e lwIP.
        // Ele garante que o stack de rede processe pacotes.
        cyw43_arch_poll();
        // Um pequeno atraso pode ser útil para não saturar o CPU,
        // mas com cyw43_arch_poll(), o ideal é que ele seja chamado o mais rápido possível.
        // sleep_ms(1); // Descomente se encontrar problemas de performance no Core 0
    }
}