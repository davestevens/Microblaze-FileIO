#include "glob.h"

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                    struct pbuf *p, err_t err)
{
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

	if(!init) {
		/* read the packet (first should be just 4 bytes) */
		char *pack;
		int j;

		pack = (char *)calloc(p->len, 1);
		for(j=0;j<p->len;j++) {
			pack[j] = *(unsigned char*)(p->payload + j);
		}

		if(((int)*(int *)pack) == (ENDIAN_FLIP(0xbabeface))) {
			if (tcp_sndbuf(tpcb) > 4) {
				err = tcp_write(tpcb, p->payload, 4, 1);
				tcp_output(tpcb);
			} else
				print("no space in tcp_sndbuf\n\r");

			/* free the received pbuf */
			pbuf_free(p);

			pcb = tpcb;
			init = 1;
		}
	}
	else {
		ret = 1;
		packet = p;
	}

	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	return ERR_OK;
}

int start_connection(void)
{
	struct tcp_pcb *_pcb;
	err_t err;

	/* create new TCP PCB structure */
	_pcb = tcp_new();
	if (!_pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(_pcb, IP_ADDR_ANY, (unsigned short)PORT);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", PORT, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(_pcb, NULL);

	/* listen for connections */
	_pcb = tcp_listen(_pcb);
	if (!_pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(_pcb, accept_callback);

	return 0;
}

char *receive_data(int *len, int expected) {
	char *pack;
	if((expected != 0) && (expected > MAX_PACKET)) {
		pack = (char *)calloc(expected, 1);
		if(pack == NULL) {
			return NULL;
		}
		int received = 0;
		do {
			char *chunk = calloc(MAX_PACKET, 1);
			int t = 0;
			do {
				while (1) {
					xemacif_input(netif);
					if(ret) {
						break;
					}
				}
				ret = 0;

				int j;
				for(j=0;j<packet->len;j++) {
					chunk[j] = *(unsigned char*)(packet->payload + j);
				}
				t += packet->len;
				pbuf_free(packet);
			} while(t < MAX_PACKET);

			if((received + t) > expected) {
				memcpy(pack + (unsigned)received, chunk, (expected - received));
			}
			else {
				memcpy(pack + (unsigned)received, chunk, t);
			}

			free(chunk);
			received += t;

			/* send an acknowledge flag */
			int ack = (ENDIAN_FLIP(ACK));
			send_data((void *)&ack, 4, 1);

		} while(received < expected);
		*len = expected;
	}
	else {
		while (1) {
			xemacif_input(netif);
			if(ret) {
				break;
			}
		}
		ret = 0;
		int j;
		*len = packet->len;
		pack = (char *)calloc(packet->len, 1);
		if(pack == NULL) {
			return NULL;
		}
		for(j=0;j<packet->len;j++) {
			pack[j] = *(unsigned char*)(packet->payload + j);
		}
		pbuf_free(packet);
	}
	return pack;
}

int send_data(void *buffer, int size, int mode) {
	if(size > MAX_PACKET) {
		int d;
		for(d=0;d<buff_size;d+=MAX_PACKET) {
			tcp_write(pcb, (void *)(char *)(buffer + (unsigned)d), MAX_PACKET, 1);
			tcp_output(pcb);

			/* wait for acknowledgement */
			int len;
			char *pack = receive_data(&len, 0);
			int _ret = (int)*(int *)pack;
			if((ENDIAN_FLIP(_ret)) != ACK) {
				xil_printf("Acknowledge flag not what I was expecting\n");
				return -1;
			}
			free(pack);
		}
	}
	else {
		tcp_write(pcb, (void *)buffer, size, 1);
		tcp_output(pcb);
	}
	return 0;
}
