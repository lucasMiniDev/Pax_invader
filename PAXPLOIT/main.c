
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// ╔══════════════════════════════════════════╗
// ║         CONFIGURAÇÕES GLOBAIS            ║
// ╚══════════════════════════════════════════╝
#define VERSION        "1.0.0"
#define MAX_RESP_SIZE  512
#define TIMEOUT_MS     1000
#define ENDPOINT_OUT   0x01
#define ENDPOINT_IN    0x81

// Estado global da conexão
static libusb_context       *g_ctx    = NULL;
static libusb_device_handle *g_handle = NULL;
static int                   g_vendor = 0;
static int                   g_product= 0;

// ╔══════════════════════════════════════════╗
// ║             ARTE ASCII                   ║
// ╚══════════════════════════════════════════╝
//█ ╗ ║ ╚ ═ ╝ ╔
void print_banner() {
    printf("\n");
    printf("  \033[36m ╔█████╗    ╔█╗   ║   ║  ╔█████╗ █║     █████   █  █████   \033[0m\n");
    printf("  \033[36m █ ═══ █   ╔█ █╗  █   █  █╝   ╚█ █║    █╝ ║ ╚█  ║    █    \033[0m\n");
    printf("  \033[36m █ ═══ █  ╔█   █╗ ╚█ █╝  █╗   ╔█ █║    █  ║  █  █    █     \033[0m\n");
    printf("  \033[36m ██████╝  █═════█  ╔█╗   ██████╝ █║    █  ║  █  █    █     \033[0m\n");
    printf("  \033[36m █║       █║   ║█ ╔█ █╗  █║      █║    █╗ ║ ╔█  █    █     \033[0m\n");
    printf("  \033[36m █║       █║   ║█ █   █  █║      ████   █████   █    █     \033[0m\n");
    printf("\n");
    printf("  \033[33m┌────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[33m│\033[0m PAXPLOIT \033[90mv%s\033[0m — Linux USB exploiter PAX devices  \033[33m│\033[0m\n", VERSION);
    printf("  \033[33m└────────────────────────────────────────────────────────────┘\033[0m\n");
    printf("\n");
}

// ╔══════════════════════════════════════════╗
// ║              AJUDA / HELP                ║
// ╚══════════════════════════════════════════╝
void print_help() {
    printf("  \033[1mBem-vindo ao USBTool — argumentos disponíveis:\033[0m\n\n");

    printf("  \033[33m── DISPOSITIVOS ─────────────────────────────────────────\033[0m\n");
    printf("  \033[32mlist\033[0m                       Lista todos os dispositivos USB\n");
    printf("  \033[32mconnect\033[0m \033[36m<VID> <PID>\033[0m        Conecta ao dispositivo (hex ou dec)\n");
    printf("  \033[32mdisconnect\033[0m                 Desconecta o dispositivo atual\n");
    printf("  \033[32minfo\033[0m                       Exibe detalhes do dispositivo conectado\n\n");

    printf("  \033[33m── COMUNICAÇÃO ──────────────────────────────────────────\033[0m\n");
    printf("  \033[32msend\033[0m \033[36m<HEX...>\033[0m              Envia bytes HEX (ex: send AA 01 FF)\n");
    printf("  \033[32mread\033[0m \033[36m[tamanho]\033[0m             Lê bytes do dispositivo (padrão: 64)\n");
    printf("  \033[32msendread\033[0m \033[36m<HEX...>\033[0m          Envia e aguarda resposta\n\n");

    printf("  \033[33m── UTILITÁRIOS ──────────────────────────────────────────\033[0m\n");
    printf("  \033[32mhelp\033[0m                       Exibe esta ajuda\n");
    printf("  \033[32mversion\033[0m                    Exibe a versão\n");
    printf("  \033[32mexit\033[0m / \033[32mquit\033[0m               Encerra o programa\n\n");

    printf("  \033[33m── EXEMPLOS ─────────────────────────────────────────────\033[0m\n");
    printf("  \033[90m./usbtool list\033[0m\n");
    printf("  \033[90m./usbtool connect 0x1234 0x5678\033[0m\n");
    printf("  \033[90m./usbtool send AA 01 02 03\033[0m\n");
    printf("  \033[90m./usbtool sendread AA 01 02 03\033[0m\n\n");

    printf("  \033[33m────────────────────────────────────────────────────────\033[0m\n");
    printf("  \033[90mDica: use lsusb para descobrir VID e PID dos dispositivos\033[0m\n\n");
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

        int dest = (g_vendor  == desc.idVendor &&
                    g_product == desc.idProduct);

        printf("  %s \033[32m%04x\033[0m:\033[36m%04x\033[0m\n",
               dest ? "\033[33m►\033[0m" : " ",
               desc.idVendor, desc.idProduct);
    }

    printf("  \033[33m──────────────────────────────────────\033[0m\n");
    printf("  Total: %zd dispositivo(s)\n\n", total);

    libusb_free_device_list(lista, 1);
    libusb_exit(ctx);
}

void cmd_connect(const char *vid_str, const char *pid_str) {
    int vid = (int)strtol(vid_str, NULL, 16);
    int pid = (int)strtol(pid_str, NULL, 16);

    if (g_handle) {
        printf("  \033[33m[!]\033[0m Já conectado. Use 'disconnect' primeiro.\n\n");
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
    printf("  \033[32m[✓]\033[0m Conectado ao dispositivo \033[32m%04x\033[0m:\033[36m%04x\033[0m\n\n", vid, pid);
}

void cmd_disconnect() {
    if (!g_handle) {
        printf("  \033[33m[!]\033[0m Nenhum dispositivo conectado.\n\n");
        return;
    }
    libusb_release_interface(g_handle, 0);
    libusb_close(g_handle);
    g_handle  = NULL;
    g_vendor  = 0;
    g_product = 0;
    printf("  \033[32m[✓]\033[0m Dispositivo desconectado.\n\n");
}

void cmd_send(int argc, char *argv[], int start) {
    if (!g_handle) {
        printf("  \033[31m[✗]\033[0m Nenhum dispositivo conectado.\n\n");
        return;
    }

    int len = argc - start;
    if (len <= 0) {
        printf("  \033[31m[✗]\033[0m Informe ao menos um byte HEX.\n\n");
        return;
    }

    unsigned char *buf = malloc(len);
    printf("  \033[90mEnviando:\033[0m ");
    for (int i = 0; i < len; i++) {
        buf[i] = (unsigned char)strtol(argv[start + i], NULL, 16);
        printf("\033[32m%02X\033[0m ", buf[i]);
    }
    printf("\n");

    int transferido = 0;
    int ret = libusb_bulk_transfer(g_handle, ENDPOINT_OUT, buf, len, &transferido, TIMEOUT_MS);
    if (ret < 0)
        printf("  \033[31m[✗]\033[0m Erro: %s\n\n", libusb_error_name(ret));
    else
        printf("  \033[32m[✓]\033[0m %d byte(s) enviado(s).\n\n", transferido);

    free(buf);
}

void cmd_read(int tamanho) {
    if (!g_handle) {
        printf("  \033[31m[✗]\033[0m Nenhum dispositivo conectado.\n\n");
        return;
    }

    unsigned char *buf = malloc(tamanho);
    memset(buf, 0, tamanho);
    int transferido = 0;

    int ret = libusb_bulk_transfer(g_handle, ENDPOINT_IN, buf, tamanho, &transferido, TIMEOUT_MS);
    if (ret < 0) {
        printf("  \033[31m[✗]\033[0m Erro: %s\n\n", libusb_error_name(ret));
    } else {
        printf("  \033[90mResposta (%d bytes):\033[0m ", transferido);
        for (int i = 0; i < transferido; i++)
            printf("\033[36m%02X\033[0m ", buf[i]);
        printf("\n\n");
    }

    free(buf);
}

void cmd_sendread(int argc, char *argv[], int start) {
    cmd_send(argc, argv, start);
    cmd_read(MAX_RESP_SIZE);
}

// ╔══════════════════════════════════════════╗
// ║                  MAIN                    ║
// ╚══════════════════════════════════════════╝
int main(int argc, char *argv[]) {

    // Sem argumentos → banner + ajuda
    if (argc < 2) {
        print_banner();
        print_help();
        return 0;
    }

    // Roteamento de comandos
    char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_banner();
        print_help();

    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
        printf("USBTool v%s\n", VERSION);

    } else if (strcmp(cmd, "list") == 0) {
        cmd_list();

    } else if (strcmp(cmd, "connect") == 0) {
        if (argc < 4) {
            printf("  \033[31m[✗]\033[0m Uso: connect <VID> <PID>  (ex: connect 0x1234 0x5678)\n\n");
            return 1;
        }
        cmd_connect(argv[2], argv[3]);

    } else if (strcmp(cmd, "disconnect") == 0) {
        cmd_disconnect();

    } else if (strcmp(cmd, "send") == 0) {
        cmd_send(argc, argv, 2);

    } else if (strcmp(cmd, "read") == 0) {
        int tam = (argc >= 3) ? atoi(argv[2]) : 64;
        cmd_read(tam);

    } else if (strcmp(cmd, "sendread") == 0) {
        cmd_sendread(argc, argv, 2);

    } else {
        printf("\n  \033[31m[✗]\033[0m Comando desconhecido: '\033[1m%s\033[0m'\n", cmd);
        printf("  Use \033[32m./usbtool help\033[0m para ver os comandos disponíveis.\n\n");
        return 1;
    }

    // Cleanup se ficou conectado
    if (g_handle) cmd_disconnect();
    if (g_ctx)    libusb_exit(g_ctx);

    return 0;
}
