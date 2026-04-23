#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libusb-1.0/libusb.h>
#include <readline/readline.h>
#include <readline/history.h>

// ╔══════════════════════════════════════════╗
// ║         CONFIGURAÇÕES GLOBAIS            ║
// ╚══════════════════════════════════════════╝
#define VERSION        "0.3 beta"
#define MAX_RESP_SIZE  512
#define ENDPOINT_OUT   0x01
#define ENDPOINT_IN    0x81
#define MAX_ARGS       32

// Estado global da conexão
static libusb_context       *g_ctx       = NULL;
static libusb_device_handle *g_handle    = NULL;
static int                   g_vendor    = 0;
static int                   g_product   = 0;
static int                   g_timeout_ms = 3000;

// ╔══════════════════════════════════════════╗
// ║         PROTOTYPES (declarações)         ║
// ╚══════════════════════════════════════════╝
void cmd_read(int tamanho);
void cmd_disconnect();

// ╔══════════════════════════════════════════╗
// ║             ARTE ASCII                   ║
// ╚══════════════════════════════════════════╝
void print_banner() {
    printf("\n");
    printf("  \033[36m ╔█████╗    ╔█╗   ║   ║  ╔█████╗ █║     █████   █  █████   \033[0m\n");
    printf("  \033[36m █ ═══ █   ╔█ █╗  █   █  █╝   ╚█ █║    █╝ ║ ╚█  ║    █    \033[0m\n");
    printf("  \033[36m █ ═══ █  ╔█   █╗ ╚█ █╝  █╗   ╔█ █║    █  ║  █  █    █     \033[0m\n");
    printf("  \033[36m ██████╝  █═════█  ╔█╗   ██████╝ █║    █  ║  █  █    █     \033[0m\n");
    printf("  \033[36m █║       █║   ║█ ╔█ █╗  █║      █║    █╗ ║ ╔█  █    █     \033[0m\n");
    printf("  \033[36m █║       █║   ║█ █   █  █║      ████   █████   █    █     \033[0m\n");
    printf("\n");
    printf("  \033[33m┌──────────────────────────────────────────────────────┐\033[0m\n");
    printf("  \033[33m│\033[0m PAXPLOIT \033[90mv%s\033[0m — Linux USB exploiter PAX devices \033[33m│\033[0m\n", VERSION);
    printf("  \033[33m└──────────────────────────────────────────────────────┘\033[0m\n");
    printf("  Para ajuda digite '\033[31mhelp\033[0m', para sair digite '\033[31mexit\033[0m'");
    printf("\n\n");
}

// ╔══════════════════════════════════════════╗
// ║              AJUDA / HELP                ║
// ╚══════════════════════════════════════════╝
void print_help() {
    printf("\n  \033[1mComandos disponíveis:\033[0m\n\n");

    printf("  \033[33m── DISPOSITIVOS ─────────────────────────────────────────\033[0m\n");
    printf("  \033[32mlist\033[0m                       Lista todos os dispositivos USB\n");
    printf("  \033[32mconnect\033[0m \033[36m<VID> <PID>\033[0m        Conecta ao dispositivo (hex ou dec)\n");
    printf("  \033[32mdisconnect\033[0m                 Desconecta o dispositivo atual\n");
    printf("  \033[32minfo\033[0m                       Exibe detalhes do dispositivo conectado\n\n");

    printf("  \033[33m── COMUNICAÇÃO ──────────────────────────────────────────\033[0m\n");
    printf("  \033[32msend\033[0m \033[36m<HEX...>\033[0m              Envia bytes e lê resposta automaticamente\n");
    printf("  \033[32mread\033[0m \033[36m[tamanho]\033[0m             Lê bytes do dispositivo (padrão: 64)\n");
    printf("  \033[32msendread\033[0m \033[36m<HEX...>\033[0m          Alias para send\n\n");

    printf("  \033[33m── CONFIGURAÇÃO ─────────────────────────────────────────\033[0m\n");
    printf("  \033[32mtimeout\033[0m \033[36m[ms]\033[0m               Exibe ou define o timeout (padrão: 3000ms)\n\n");

    printf("  \033[33m── UTILITÁRIOS ──────────────────────────────────────────\033[0m\n");
    printf("  \033[32mhelp\033[0m                       Exibe esta ajuda\n");
    printf("  \033[32mclear\033[0m                      Limpa a tela\n");
    printf("  \033[32mversion\033[0m                    Exibe a versão\n");
    printf("  \033[32mexit\033[0m / \033[32mquit\033[0m               Encerra o programa\n\n");

    printf("  \033[33m── FORMATOS HEX ACEITOS ─────────────────────────────────\033[0m\n");
    printf("  \033[90musb> send AA BB CC\033[0m\n");
    printf("  \033[90musb> send \\xAA\\xBB\\xCC\033[0m\n");
    printf("  \033[90musb> send AA \\xBB CC\033[0m         ← formatos misturados\n\n");

    printf("  \033[33m── EXEMPLOS ─────────────────────────────────────────────\033[0m\n");
    printf("  \033[90musb> list\033[0m\n");
    printf("  \033[90musb> connect 1234 5678\033[0m\n");
    printf("  \033[90musb> timeout 5000\033[0m\n");
    printf("  \033[90musb> send AA 01 02 03\033[0m\n");
    printf("  \033[90musb> read\033[0m\n");
    printf("  \033[90musb> disconnect\033[0m\n\n");

    printf("  \033[33m── ATALHOS DO TECLADO ───────────────────────────────────\033[0m\n");
    printf("  \033[36m↑ / ↓\033[0m          Navega pelo histórico de comandos\n");
    printf("  \033[36m← / →\033[0m          Move o cursor na linha\n");
    printf("  \033[36mHome / End\033[0m     Início / fim da linha\n");
    printf("  \033[36mCtrl+W\033[0m         Apaga a palavra anterior\n");
    printf("  \033[36mCtrl+U\033[0m         Apaga a linha inteira\n");
    printf("  \033[36mCtrl+R\033[0m         Busca reversa no histórico\n\n");

    printf("  \033[33m────────────────────────────────────────────────────────\033[0m\n\n");
}

// ╔══════════════════════════════════════════╗
// ║        PARSER DE LINHA DE COMANDO        ║
// ╚══════════════════════════════════════════╝
int parse_line(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *token = strtok(line, " \t\r\n");
    while (token && argc < max_args) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    return argc;
}

// ╔══════════════════════════════════════════╗
// ║       EXPANSOR DE BYTES HEX              ║
// ╚══════════════════════════════════════════╝
// Suporta dois formatos:
//   - "AA"            → 1 byte  0xAA
//   - "\xAA\xBB\xCC"  → 3 bytes 0xAA 0xBB 0xCC
// Retorna quantidade de bytes escritos, -1 em erro
int expand_token(const char *token, unsigned char *out, int max) {
    int count = 0;
    const char *p = token;

    while (*p && count < max) {
        if (p[0] == '\\' && p[1] == 'x') {
            if (!isxdigit((unsigned char)p[2]) || !isxdigit((unsigned char)p[3])) {
                printf("  \033[33m[!]\033[0m Sequência \\x inválida: '%.4s'\n", p);
                return -1;
            }
            char tmp[3] = { p[2], p[3], '\0' };
            out[count++] = (unsigned char)strtol(tmp, NULL, 16);
            p += 4;
        } else {
            // Valida que o token inteiro é hex puro (ex: "AA", "0F")
            const char *check = p;
            while (*check) {
                if (!isxdigit((unsigned char)*check)) {
                    printf("  \033[33m[!]\033[0m Token inválido: '\033[1m%s\033[0m' — use HEX (ex: AA, 0F, FF)\n", token);
                    return -1;
                }
                check++;
            }
            out[count++] = (unsigned char)strtol(p, NULL, 16);
            break;
        }
    }
    return count;
}

// ╔══════════════════════════════════════════╗
// ║       HELPER: AUTO-DISCONNECT            ║
// ╚══════════════════════════════════════════╝
void handle_device_error(int ret) {
    if (ret == LIBUSB_ERROR_NO_DEVICE || ret == LIBUSB_ERROR_IO) {
        printf("  \033[31m[✗]\033[0m Dispositivo desconectado inesperadamente.\n");
        printf("  \033[90m    Limpando conexão...\033[0m\n");
        libusb_release_interface(g_handle, 0);
        libusb_close(g_handle);
        g_handle  = NULL;
        g_vendor  = 0;
        g_product = 0;
        printf("  \033[33m[!]\033[0m Use 'connect <VID> <PID>' para reconectar.\n\n");
    } else if (ret < 0) {
        printf("  \033[31m[✗]\033[0m Erro: %s\n\n", libusb_error_name(ret));
    }
}

// ╔══════════════════════════════════════════╗
// ║           FUNÇÕES USB                    ║
// ╚══════════════════════════════════════════╝
void cmd_list() {
    libusb_context *ctx = NULL;
    libusb_device **lista;
    ssize_t total;

    libusb_init(&ctx);
    total = libusb_get_device_list(ctx, &lista);

    printf("\n  \033[1mDispositivos USB encontrados:\033[0m\n");
    printf("  \033[33m──────────────────────────────────────\033[0m\n");

    for (ssize_t i = 0; i < total; i++) {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(lista[i], &desc);

        int ativo = (g_vendor  == (int)desc.idVendor &&
                     g_product == (int)desc.idProduct);

        printf("  %s \033[32m%04x\033[0m:\033[36m%04x\033[0m%s\n",
               ativo ? "\033[33m►\033[0m" : " ",
               desc.idVendor, desc.idProduct,
               ativo ? "  \033[33m← conectado\033[0m" : "");
    }

    printf("  \033[33m──────────────────────────────────────\033[0m\n");
    printf("  Total: \033[1m%zd\033[0m dispositivo(s)\n\n", total);

    libusb_free_device_list(lista, 1);
    libusb_exit(ctx);
}

void cmd_connect(const char *vid_str, const char *pid_str) {
    int vid = (int)strtol(vid_str, NULL, 16);
    int pid = (int)strtol(pid_str, NULL, 16);

    if (g_handle) {
        printf("  \033[33m[!]\033[0m Já conectado a \033[32m%04x\033[0m:\033[33m%04x\033[0m. Use 'disconnect' primeiro.\n\n",
               g_vendor, g_product);
        return;
    }

    if (!g_ctx) libusb_init(&g_ctx);

    g_handle = libusb_open_device_with_vid_pid(g_ctx, vid, pid);
    if (!g_handle) {
        printf("  \033[31m[✗]\033[0m Dispositivo \033[32m%04x\033[0m:\033[36m%04x\033[0m não encontrado.\n\n", vid, pid);
        return;
    }

    if (libusb_kernel_driver_active(g_handle, 0) == 1)
        libusb_detach_kernel_driver(g_handle, 0);

    if (libusb_claim_interface(g_handle, 0) < 0) {
        printf("  \033[31m[✗]\033[0m Erro ao reivindicar interface.\n\n");
        libusb_close(g_handle);
        g_handle = NULL;
        return;
    }

    g_vendor  = vid;
    g_product = pid;
    printf("  \033[32m[✓]\033[0m Conectado a \033[32m%04x\033[0m:\033[33m%04x\033[0m\n\n", vid, pid);
}

void cmd_disconnect() {
    if (!g_handle) {
        printf("  \033[33m[!]\033[0m Nenhum dispositivo conectado.\n\n");
        return;
    }
    printf("  \033[90mDesconectando \033[32m%04x\033[0m\033[90m:\033[33m%04x\033[0m\033[90m...\033[0m\n",
           g_vendor, g_product);
    libusb_release_interface(g_handle, 0);
    libusb_close(g_handle);
    g_handle  = NULL;
    g_vendor  = 0;
    g_product = 0;
    printf("  \033[32m[✓]\033[0m Dispositivo desconectado.\n\n");
}

void cmd_info() {
    if (!g_handle) {
        printf("  \033[33m[!]\033[0m Nenhum dispositivo conectado.\n\n");
        return;
    }
    printf("\n  \033[1mDispositivo atual:\033[0m\n");
    printf("  \033[33m──────────────────────────────\033[0m\n");
    printf("  Vendor ID  : \033[32m0x%04x\033[0m\n", g_vendor);
    printf("  Product ID : \033[36m0x%04x\033[0m\n", g_product);
    printf("  Endpoint → : \033[90m0x%02X\033[0m\n", ENDPOINT_OUT);
    printf("  Endpoint ← : \033[90m0x%02X\033[0m\n", ENDPOINT_IN);
    printf("  Timeout    : \033[90m%d ms\033[0m\n",  g_timeout_ms);
    printf("  \033[33m──────────────────────────────\033[0m\n\n");
}

void cmd_read(int tamanho) {
    if (!g_handle) {
        printf("  \033[31m[✗]\033[0m Nenhum dispositivo conectado. Use 'connect <VID> <PID>'.\n\n");
        return;
    }

    unsigned char *buf = malloc(tamanho);
    memset(buf, 0, tamanho);
    int transferido = 0;

    int ret = libusb_bulk_transfer(g_handle, ENDPOINT_IN, buf, tamanho,
                                   &transferido, g_timeout_ms);

    if (ret == LIBUSB_ERROR_TIMEOUT) {
        if (transferido > 0) {
            // Timeout mas recebeu dados parciais — exibe mesmo assim
            printf("  \033[33m[!]\033[0m Resposta parcial (%d byte(s)):\033[0m ", transferido);
            for (int i = 0; i < transferido; i++)
                printf("\033[36m%02X\033[0m ", buf[i]);
            printf("\n\n");
        } else {
            printf("  \033[33m[!]\033[0m Sem resposta do dispositivo (timeout %dms).\n", g_timeout_ms);
            printf("  \033[90m    Verifique se o endpoint IN está correto: 0x%02X\033[0m\n\n", ENDPOINT_IN);
        }
    } else if (ret == LIBUSB_ERROR_NO_DEVICE || ret == LIBUSB_ERROR_IO) {
        handle_device_error(ret);
    } else if (ret < 0) {
        printf("  \033[31m[✗]\033[0m Erro: %s\n\n", libusb_error_name(ret));
    } else {
        printf("  \033[90m← Resposta (%d byte(s)):\033[0m ", transferido);
        for (int i = 0; i < transferido; i++)
            printf("\033[36m%02X\033[0m ", buf[i]);
        printf("\n\n");
    }

    free(buf);
}

void cmd_send(int argc, char *argv[], int start) {
    if (!g_handle) {
        printf("  \033[31m[✗]\033[0m Nenhum dispositivo conectado. Use 'connect <VID> <PID>'.\n\n");
        return;
    }

    int len = argc - start;
    if (len <= 0) {
        printf("  \033[31m[✗]\033[0m Informe ao menos um byte HEX.\n");
        printf("  \033[90m    Formatos aceitos:  send AA BB CC  |  send \\xAA\\xBB\\xCC\033[0m\n\n");
        return;
    }

    // Expande todos os tokens em um buffer de bytes
    unsigned char buf[MAX_RESP_SIZE];
    int total = 0;

    for (int i = start; i < argc && total < MAX_RESP_SIZE; i++) {
        unsigned char tmp[MAX_RESP_SIZE];
        int n = expand_token(argv[i], tmp, MAX_RESP_SIZE - total);
        if (n < 0) return;   // erro de validação já foi impresso
        memcpy(buf + total, tmp, n);
        total += n;
    }

    if (total == 0) {
        printf("  \033[31m[✗]\033[0m Nenhum byte válido para enviar.\n\n");
        return;
    }

    printf("  \033[90m→ Enviando (%d byte(s)):\033[0m ", total);
    for (int i = 0; i < total; i++)
        printf("\033[32m%02X\033[0m ", buf[i]);
    printf("\n");

    int transferido = 0;
    int ret = libusb_bulk_transfer(g_handle, ENDPOINT_OUT, buf, total,
                                   &transferido, g_timeout_ms);

    if (ret == LIBUSB_ERROR_NO_DEVICE || ret == LIBUSB_ERROR_IO) {
        handle_device_error(ret);
        return;
    } else if (ret < 0) {
        printf("  \033[31m[✗]\033[0m Erro no envio: %s\n\n", libusb_error_name(ret));
        return;
    }

    printf("  \033[32m[✓]\033[0m %d byte(s) enviado(s).\n", transferido);

    // Lê resposta automaticamente após envio bem-sucedido
    cmd_read(MAX_RESP_SIZE);
}

// ╔══════════════════════════════════════════╗
// ║           ROTEADOR DE COMANDOS           ║
// ╚══════════════════════════════════════════╝
int dispatch(int argc, char *argv[]) {
    if (argc == 0) return 0;

    char *cmd = argv[0];

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        return 1;

    } else if (strcmp(cmd, "help") == 0) {
        print_help();

    } else if (strcmp(cmd, "clear") == 0) {
        printf("\033[2J\033[H");
        print_banner();

    } else if (strcmp(cmd, "version") == 0) {
        printf("  PAXPLOIT v%s\n\n", VERSION);

    } else if (strcmp(cmd, "list") == 0) {
        cmd_list();

    } else if (strcmp(cmd, "connect") == 0) {
        if (argc < 3)
            printf("  \033[31m[✗]\033[0m Uso: connect <VID> <PID>   ex: connect 1234 5678\n\n");
        else
            cmd_connect(argv[1], argv[2]);

    } else if (strcmp(cmd, "disconnect") == 0) {
        cmd_disconnect();

    } else if (strcmp(cmd, "info") == 0) {
        cmd_info();

    } else if (strcmp(cmd, "timeout") == 0) {
        if (argc < 2) {
            printf("  Timeout atual: \033[33m%d ms\033[0m\n\n", g_timeout_ms);
        } else {
            int t = atoi(argv[1]);
            if (t <= 0) {
                printf("  \033[31m[✗]\033[0m Valor inválido. Use milissegundos (ex: timeout 5000)\n\n");
            } else {
                g_timeout_ms = t;
                printf("  \033[32m[✓]\033[0m Timeout definido para \033[33m%d ms\033[0m\n\n", g_timeout_ms);
            }
        }

    } else if (strcmp(cmd, "send") == 0 || strcmp(cmd, "sendread") == 0) {
        cmd_send(argc, argv, 1);

    } else if (strcmp(cmd, "read") == 0) {
        int tam = (argc >= 2) ? atoi(argv[1]) : 64;
        cmd_read(tam);

    } else {
        printf("  \033[31m[✗]\033[0m Comando desconhecido: '\033[1m%s\033[0m'  —  tente \033[32mhelp\033[0m\n\n", cmd);
    }

    return 0;
}

// ╔══════════════════════════════════════════╗
// ║                  MAIN                    ║
// ╚══════════════════════════════════════════╝
int main() {
    char *line;
    char *argv[MAX_ARGS];
    int   argc;

    // Histórico persiste entre sessões
    read_history("~/.paxploit_history");

    print_banner();

    while (1) {
        // Prompt dinâmico conforme estado da conexão
        char prompt[128];
        if (g_handle)
            snprintf(prompt, sizeof(prompt),
                     "\033[36musb\033[0m:\033[32m%04x\033[0m:\033[33m%04x\033[0m> ",
                     g_vendor, g_product);
        else
            snprintf(prompt, sizeof(prompt), "\033[36musb\033[0m> ");

        line = readline(prompt);

        if (!line) {        // Ctrl+D → sair
            printf("\n");
            break;
        }

        if (*line)          // não salva linhas vazias no histórico
            add_history(line);

        // Cópia necessária pois strtok modifica a string in-place
        char buf[4096];
        strncpy(buf, line, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        free(line);

        argc = parse_line(buf, argv, MAX_ARGS);

        if (dispatch(argc, argv) == 1)
            break;
    }

    // Salva histórico ao sair
    write_history("~/.paxploit_history");

    printf("  \033[90mEncerrando...\033[0m\n");
    if (g_handle) cmd_disconnect();
    if (g_ctx)    libusb_exit(g_ctx);
    printf("  \033[32m[✓]\033[0m PAXPLOIT terminado.\n\n");

    return 0;
}