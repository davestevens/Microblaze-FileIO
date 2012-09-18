#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#include "def.h"
#include "pcre_split.h"

int server(char *, int);
int construct_writeConnection(char *, int, int, struct sockaddr_in *);
int deconstruct_connection(int, char *, int);
void constructAddressStruct(struct sockaddr_in *, int, int, int);
void serve(int);

int send_data(int, void *, int);
char *receive_data(int, int);

int main(int argc, char *argv[]) {
	if(argc != 3) {
		fprintf(stderr, "Usage: %s <address> <port>\n", argv[0]);
		return -1;
	}

	server(argv[1], atoi(argv[2]));
	return 0;
}

int server(char *addr, int port) {
	int sock;
	struct sockaddr_in sockAddr;

	fprintf(stderr, "Server:\n");

	sock = construct_writeConnection(addr, inet_addr(addr), port, &sockAddr);
	if(sock == -1) {
		fprintf(stderr, "Issue connecting to %s:%d\n", addr, port);
		return -1;
	}

	/* send an initial payload of 0xbabeface to inform the microblaze we are ready */
	fprintf(stderr, "\tCommunicating with microblaze\n");
	{
		int packet = INIT;
		/* write init */
		write(sock, (void *)(char *)&packet, 4);
	}
	/* then read back */
	{
		int ret = 0x0;
		read(sock, (void *)(char *)&ret, 4);
		if(ret != (int)INIT) {
			fprintf(stderr, "Initialisation failed\n");
			return -1;
		}
		else {
			fprintf(stderr, "--------------------------------------------------------------------------------\n");
		}
	}

	/* at this point we are in the program until we get the DEINIT packet */
	serve(sock);

	fprintf(stderr, "--------------------------------------------------------------------------------\n");
	if(deconstruct_connection(sock, addr, port) == -1) {
		fprintf(stderr, "Issue disconnecting from %s:%d\n", addr, port);
		return -1;
	}

	return 0;
}

int construct_writeConnection(char *addr_string, int addr, int port, struct sockaddr_in *echoServAddr) {
	int sock;

	fprintf(stderr, "\tconstruct_writeConnection: %s:%d\n", addr_string, port);
	/* Create a reliable, stream socket using TCP */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "socket() failed\n");
		return -1;
	}

	constructAddressStruct(echoServAddr, AF_INET, addr, port);

	/* Establish the connection to the echo server */
	if (connect(sock, (struct sockaddr *)echoServAddr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "connect() failed\n");
		return -1;
	}
	return sock;
}

int deconstruct_connection(int sock, char *addr_string, int port) {
	fprintf(stderr, "disconnecting from %s:%d\n", addr_string, port);
	return shutdown(sock, SHUT_RDWR);
}

void constructAddressStruct(struct sockaddr_in *echoServAddr, int family, int addr, int port) {
	/* Construct local address structure */
	memset(echoServAddr, 0, sizeof(struct sockaddr_in));   /* Zero out structure */
	echoServAddr->sin_family = family;                /* Internet address family */
	echoServAddr->sin_addr.s_addr = addr; /* Any incoming interface */
	echoServAddr->sin_port = htons(port);      /* Local port */
	return;
}

/* this function waits for connections */
void serve(int sock) {
	int header;
	char *tmp = NULL;

	for(;;) {
		char *head = receive_data(sock, 4);
		header = *(int *)(&head[0]);
		free(head);

		if(header == DEINIT) {
			write(sock, (void *)(char *)&header, 4);
			return;
		}

		/* pull rest of buffer from the socket */
		tmp = receive_data(sock, (header & 0xffffff));

		switch(((header >> 24) & 0xff)) {
			case PRINTF:
			{
				int ret;
				ret = printf("%s", tmp);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FOPEN:
			{
				int filenameSize, modeSize;
				char *filename, *mode;
				FILE *ret;
				int *tmpP = (int *)tmp;
				filenameSize = (int)*tmpP;
				modeSize = (int)*(tmpP+1);

				filename = (char *)calloc((filenameSize + 1), 1);
				memcpy(filename, (tmp+8), filenameSize);
				mode = (char *)calloc((modeSize + 1), 1);
				memcpy(mode, (void *)((tmp+8) + (unsigned)filenameSize), modeSize);
				ret = fopen(filename, mode);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FCLOSE:
			{
				int stream = (int)*(int *)tmp;
				int ret = fclose((FILE *)stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FREAD:
			{
				int *tmpP = (int *)tmp;
				size_t size = (size_t) *tmpP;
				size_t nitems = (size_t) *(tmpP+1);
				FILE *stream = (FILE *) *(tmpP+2);

				char *ptr = (char *)calloc((size * nitems), 1);
				size_t ret = fread(ptr, size, nitems, stream);

				char *ptr2 = (char *)calloc((size * nitems) + 4, 1);
				*(int *)(&ptr2[0]) = ret;
				memcpy((ptr2+4), ptr, (size * nitems));

				if(send_data(sock, (void *)(char *)ptr2, ((size * nitems) + 4)) == -1) {
					return;
				}

				/* wait for acknowledgement */
				{
					char *ack = receive_data(sock, 4);
					if(ack == NULL) {
						fprintf(stderr, "Error allocating memory for receive_data\n");
						return;
					}
					free(ack);
				}

				free(ptr);
			}
			break;
			case FWRITE:
			{
				int *tmpP = (int *)tmp;
				size_t size = (size_t) *tmpP;
				size_t nitems = (size_t) *(tmpP+1);
				FILE *stream = (FILE *) *(tmpP+2);
				/* the rest is then the data to write */

				size_t ret = fwrite((void *)(char *)(tmpP+3), size, nitems, stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FPRINTF:
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = fprintf(stream, "%s", (tmp+4));
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FPUTC:
			{
				int *tmpP = (int *)tmp;
				int c = (int) *(tmpP);
				FILE *stream = (FILE *) *(tmpP+1);

				int ret = fputc(c, stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FGETC:
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = fgetc(stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FSCANF:
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);
				char *fmt = (tmp + 4);

				/* perform pcre_split */
				split_t *_test, *_temp;
				int ret = 0;

				char *buff = (char *)calloc(100, 1);
				int buff_pos = 1;

				/* RegEx to match any printf format */
				_test = pcre_split("%[\\+#-]?(\\d+|\\*)?\\.?\\d*([hlLzjt]|[hl]{2})?([csuxXfFeEpgGdionz])", (char *)fmt);
				_temp = _test;
				if(_test == NULL) {
					fprintf(stderr, "Error: Nothing to print\n");
				}
				else {
					char _str[256] = {'\0'};
					do {
						if(_test->string != NULL) {
							strcat(_str, _test->string);
						}

						if(_test->match != NULL) {
							strcat(_str, _test->match);
							switch(*(char *)((unsigned)_test->match + strlen(_test->match) - 1)) {
								case 's':
								case 'S':
									{
										char str[256] = {'\0'};
										int _ret = fscanf(stream, _str, str);
										int t = strlen(str);
										while(t % 4) { t++; } /* pad to word */

										*(int *)(&buff[(buff_pos++)<<2]) = (TYPE_S) << 24 | t;
										if(_ret != -1) {
											memcpy((buff+(buff_pos<<2)), str, strlen(str));
											buff_pos += t>>2;
											ret += _ret;
										}
										else {
										}
									}
								break;
								case 'f':
								case 'F':
									{
										switch(*(char *)((unsigned)_test->match + strlen(_test->match) - 2)) {
											case 'l':
											{
												double d;
												int _ret = fscanf(stream, _str, &d);
												int *dP = (int *)&d;
												*(int *)(&buff[(buff_pos++)<<2]) = (TYPE_DB) << 24 | (8);
												if(_ret != -1) {
													*(int *)(&buff[(buff_pos++)<<2]) = (int)*dP;
													*(int *)(&buff[(buff_pos++)<<2]) = (int)*(dP+1);
													ret += _ret;
												}
												else {
													*(int *)(&buff[(buff_pos++)<<2]) = 0;
													*(int *)(&buff[(buff_pos++)<<2]) = 0;
												}
											}
											break;
											default:
											{
												float d;
												int _ret = fscanf(stream, _str, &d);
												*(int *)(&buff[(buff_pos++)<<2]) = (TYPE_F) << 24 | (4);
												if(_ret != -1) {
													*(int *)(&buff[(buff_pos++)<<2]) = *(int *)&d;
													ret += _ret;
												}
												else {
													*(int *)(&buff[(buff_pos++)<<2]) = 0;
												}
											}
											break;
										}
									}
								break;
								default:
								{
									int t;
									int _ret = fscanf(stream, _str, &t);
									*(int *)(&buff[(buff_pos++)<<2]) = (TYPE_D) << 24 | (4);
									if(_ret != -1) {
										*(int *)(&buff[(buff_pos++)<<2]) = t;
										ret += _ret;
									}
									else {
										*(int *)(&buff[(buff_pos++)<<2]) = 0;
									}
								}
								break;
							}
							_str[0] = '\0';
						}
					} while((_test = _test->next) != NULL);
				}
				pcre_split_free(_temp);

				*(int *)(&buff[0]) = ret;

				if((buff_pos << 2) > MAX_PACKET) {
					/* need to write it in packets */
					int full_write = 0;
					do {
						int t = 0;
						do {
							t += write(sock, (void *)(buff + (unsigned)full_write), MAX_PACKET);
						} while(t < MAX_PACKET);

						/* wait for acknowledgement */
						{
							char *ack = receive_data(sock, 4);
							if(ack == NULL) {
								fprintf(stderr, "Error allocating memory for receive_data\n");
								return;
							}
							if(*(int *)(&ack[0]) != (int)ACK) {
								fprintf(stderr, "Acknowledge flag not what i was expecting.\n");
								return;
							}
							free(ack);
						}

						full_write += t;
					} while(full_write < (buff_pos << 2));
				}
				else {
					write(sock, (void *) buff, (buff_pos << 2));
				}

				/* wait for acknowledgement */
				{
					char *ack = receive_data(sock, 4);
					if(ack == NULL) {
						fprintf(stderr, "Error allocating memory for receive_data\n");
						return;
					}
					free(ack);
				}

				free(buff);
			}
			break;
			/* TODO: test on system */
			case FEOF:
				/* passed stream, returns int */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = feof(stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FFLUSH:
				/* passed stream, returns int */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = fflush(stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FGETPOS:
				/* passed strem, returns int and fpos_t */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);
				fpos_t pos;

				int ret = fgetpos(stream, &pos);
				write(sock, (void *)(char *)&ret, 4);
				write(sock, (void *)(char *)&pos, 8);
			}
			break;
			case FGETS:
				/* passed number of chars and stream, returns string and pointer to string */
			{
				int *tmpP = (int *)tmp;
				int n = (int) *(tmpP);
				FILE *stream = (FILE *) *(tmpP+1);

				int size = (n+1) + 4;
				char *buff = (char *)calloc(size, 1);
				char *str = (buff+4);
				char *ret = fgets(str, n, stream);

				*(int *)(&buff[0]) = (int)ret;
				if(size > MAX_PACKET) {
					/* need to write it in packets */
					int full_write = 0;
					do {
						int t = 0;
						do {
							t += write(sock, (void *)(buff + (unsigned)full_write), MAX_PACKET);
						} while(t < MAX_PACKET);

						/* wait for acknowledgement */
						{
							char *ack = receive_data(sock, 4);
							if(ack == NULL) {
								fprintf(stderr, "Error allocating memory for receive_data\n");
								return;
							}
							if(*(int *)(&ack[0]) != (int)ACK) {
								fprintf(stderr, "Acknowledge flag not what i was expecting.\n");
								return;
							}
							free(ack);
						}

						full_write += t;
					} while(full_write < size);
				}
				else {
					write(sock, (void *) buff, size);
				}

				/* wait for acknowledgement */
				{
					char *ack = receive_data(sock, 4);
					if(ack == NULL) {
						fprintf(stderr, "Error allocating memory for receive_data\n");
						return;
					}
					free(ack);
				}

				free(buff);
			}
			break;
			case FPUTS:
				/* passed stream and string, returns int */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = fputs((tmp+4), stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FSEEK:
				/* passed stream, long & int, returns int */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);
				long offset = (long) *(tmpP+1);
				int whence = (int) *(tmpP+2);

				int ret = fseek(stream, offset, whence);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FSETPOS:
				/* passed stream and fpos_t, returns int */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);
				fpos_t *pos = (fpos_t *) *(tmpP+1);

				int ret = fsetpos(stream, pos);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			case FTELL:
				/* passed stream, returns in */
			{
				int *tmpP = (int *)tmp;
				FILE *stream = (FILE *) *(tmpP);

				int ret = ftell(stream);
				write(sock, (void *)(char *)&ret, 4);
			}
			break;
			/* /TODO: test on system */
			default:
				fprintf(stderr, "Error: unknown option %d\n", 0);
				break;
		}
		free(tmp);
	}
	return;
}

int send_data(int sock, void *buffer, int size) {
	if(size > MAX_PACKET) {
		/* need to write it in packets */
		int full_write = 0;
		do {
			int t = 0;
			do {
				t += write(sock, (void *)((char *)buffer + (unsigned)full_write), MAX_PACKET);
			} while(t < MAX_PACKET);
			{
				/* then wait for ack */
				char *ack = receive_data(sock, 4);
				if(ack == NULL) {
					fprintf(stderr, "Error allocating memory for receive_data\n");
					return -1;
				}
				if(*(int *)(&ack[0]) != (int)ACK) {
					fprintf(stderr, "Acknowledge flag not what i was expecting.\n");
					return -1;
				}
				free(ack);
			}
			full_write += t;
		} while(full_write < size);
	}
	else {
		write(sock, (void *)buffer, size);
	}
	return 0;
}

char *receive_data(int sock, int expected) {
	char *pack = (char *)calloc(expected, 1);
	if(expected > MAX_PACKET) {
		/* need to read in in sections */
		int full_read = 0;
		do {
			char *chunk = (char *)calloc(MAX_PACKET, 1);
			int t = 0;
			do {
				t += read(sock, (void *)(char *)(chunk+(unsigned)t), ((full_read) ? (MAX_PACKET) : (MAX_PACKET-4)));
			} while(t < ((full_read)? (MAX_PACKET) : (MAX_PACKET-4)));

			if((full_read + t) > expected) {
				memcpy(pack + (unsigned)full_read, chunk, expected - full_read);
			}
			else {
				/* copy this into tmp */
				memcpy(pack + (unsigned)full_read, chunk, t);
			}
			/* free chunk */
			free(chunk);
			full_read += t;

			/* send ack */
			{
				int ret;
				ret = ACK;
				if(send_data(sock, (void *)(char *)&ret, 4) == -1) {
					fprintf(stderr, "Did not receive correct acknowledge packet\n");
					return NULL;
				}
			}
		} while(full_read < expected);
	}
	else {
		/* read in a single go */
		int t = 0;
		do {
			t += read(sock, (void *)(char *)(pack+(unsigned)t), expected);
		} while(t < expected);
	}
	return pack;
}
