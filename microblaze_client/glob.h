#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "xparameters.h"
#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"

#include "lwip/err.h"
#include "lwip/tcp.h"

#include "def.h"

#define PORT 1337
#define SERV_0 192
#define SERV_1 168
#define SERV_2 0
#define SERV_3 1

#define ENDIAN_FLIP(x) ((x >> 24) & 0xff) | (((x >> 16) & 0xff) << 8) | (((x >> 8) & 0xff) << 16) | (((x >> 0) & 0xff) << 24)

int init;
int ret;
struct pbuf *packet;
int buff_size;
char *buff;
struct tcp_pcb *pcb;
struct netif *netif, server_netif;
struct tcp_pcb *Wpcb;
struct ip_addr netmask, gw;
unsigned char *mac_ethernet_address;

/* missing declaration in lwIP */
void lwip_init();

int main_main(void);
int main(void);
err_t recv_callback(void *, struct tcp_pcb *, struct pbuf *, err_t);
err_t accept_callback(void *, struct tcp_pcb *, err_t);
int start_connection(void);
int start_writeConnection(int);
char *receive_data(int *, int);
err_t connected_callback(void *, struct tcp_pcb *, err_t);
int send_data(void *, int, int);

int my_printf(const char *, ...);
FILE *my_fopen(const char *, const char *);
int my_fclose(FILE *);
size_t my_fread(void *, size_t, size_t, FILE *);
size_t my_fwrite(const void *, size_t, size_t, FILE *);
int my_fprintf(FILE *, const char *, ...);
int my_fgetc(FILE *);
int my_fputc(int, FILE *);
int my_fscanf(FILE *, const char *, ...);
int my_feof(FILE *);
int my_fflush(FILE *);
