/*
  Implementations of functions to send and receive data from x86 host
 */

#include "glob.h"

int my_printf(const char *fmt, ...) {
	va_list args;
	char str[256];
	va_start(args, fmt);
	vsprintf(str, fmt, args);
	va_end(args);

	/* setup header and body */
	int header = PRINTF << 24;
	header |= (strlen(str) + 1); /* make sure padded with \0 */

	buff_size = 4 + strlen(str) + 1;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	int u;
	for(u=0;u<strlen(str);u++) {
		buff[4 + u] = str[u];
	}

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

FILE *my_fopen(const char *filename, const char *mode) {
	int header = FOPEN << 24;
	header |= (strlen(filename) + strlen(mode) + 8);

	buff_size = 12 + strlen(filename) + strlen(mode);
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP(strlen(filename));
	*(int *)(&buff[8]) = ENDIAN_FLIP(strlen(mode));
	int u, v;
	for(u=0;u<strlen(filename);u++) {
		buff[12 + u] = filename[u];
	}
	for(v=0;u<(strlen(filename) + strlen(mode));u++,v++) {
		buff[12 + u] = mode[v];
	}

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	_ret = ENDIAN_FLIP(_ret);
	return (FILE *)_ret;
}

int my_fclose(FILE *stream) {
	int header = FCLOSE << 24;
	header |= 4;

	buff_size = 8;
	buff = (char *)calloc(buff_size, 1);

	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 4);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

size_t my_fread(void *ptr, size_t size, size_t nitems, FILE *stream) {
	int header = FREAD << 24;
	header |= 12;

	buff_size = 16;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)size);
	*(int *)(&buff[8]) = ENDIAN_FLIP((int)nitems);
	*(int *)(&buff[12]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len, j;
	char *pack = receive_data(&len, (nitems * size) + 4);

	char *ptrC = (char *)ptr;
	for(j=4;j<len;j++) {
		ptrC[j-4] = pack[j];
	}
	int _ret = (int)*(int *)pack;
	free(pack);
	/* acknowledge the host that packet has been received */
	send_data((void *)(char *)&ret, 4, 1);
	free(buff);
	return ENDIAN_FLIP((int)_ret);
}

size_t my_fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream) {
	int header = FWRITE << 24;
	header |= 12 + (size * nitems);

	buff_size = 16 + (size * nitems);
	buff = (char *)calloc(buff_size, 1);
	if(buff == NULL) {
		xil_printf("buff == NULL\n");
		exit(-1);
	}
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)size);
	*(int *)(&buff[8]) = ENDIAN_FLIP((int)nitems);
	*(int *)(&buff[12]) = ENDIAN_FLIP((int)stream);

	int u;
	char *ptrC = (char *)ptr;
	for(u=0;u<(size * nitems);u++) {
		buff[16 + u] = ptrC[u];
	}

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP((int)_ret);
}

int my_fprintf(FILE *stream, const char *fmt, ...) {
	va_list args;
	char str[256];
	va_start(args, fmt);
	vsprintf(str, fmt, args);
	va_end(args);

	int header = FPRINTF << 24;
	header |= 4 + strlen(str) + 1;

	buff_size = 8 + strlen(str) + 1;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);
	int u;
	for(u=0;u<strlen(str);u++) {
		buff[8 + u] = str[u];
	}

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

int my_fgetc(FILE *stream) {
	int header = FGETC << 24;
	header |= 4;

	buff_size = 8;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

int my_fputc(int c, FILE *stream) {
	int header = FPUTC << 24;
	header |= 8;

	buff_size = 12;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP(c);
	*(int *)(&buff[8]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

int my_fscanf(FILE *stream, const char *fmt, ...) {
	int header = FSCANF << 24;
	header |= 4 + strlen(fmt);

	buff_size = 8 + strlen(fmt);
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);
	int u;
	for(u=0;u<strlen(fmt);u++) {
		buff[8 + u] = fmt[u];
	}

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int i;
	int _ret = (int)*(int *)(pack);

	/* read the packets into the variables */
	va_list pArgs;
	va_start(pArgs, fmt);

	/* loop over the buffer */
	for(i=4;i<(len);i+=4) {
		/* read the header */
		int header = *(int *)(&pack[i]);
		header = (ENDIAN_FLIP(header));

		/* move pointer */
		i += 4;

		/* read */
		int *ptr = (void *)va_arg(pArgs, int);
		if((header & 0xffffff) > 0) {
			switch(header >> 24) {
				case TYPE_D:
					*ptr = ENDIAN_FLIP(*(int *)(&pack[i]));
					break;
				case TYPE_F:
					*ptr = ENDIAN_FLIP(*(int *)(&pack[i]));
					break;
				case TYPE_DB:
					*(ptr+1) = ENDIAN_FLIP(*(int *)(&pack[i]));
					i+=4;
					*ptr = ENDIAN_FLIP(*(int *)(&pack[i]));
					break;
				case TYPE_S:
					strncpy((char *)ptr, (char *)&pack[i], (header & 0xffffff));
					i+=((header & 0xffffff)-4);
					break;
				default:
					xil_printf("Unknown type: %d\n", (header >> 24));
					break;
			}
		}
	}

	/* acknowledge the host that packet has been received */
	send_data((void *)(char *)&ret, 4, 1);

	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

int my_feof(FILE *stream) {
	int header = FEOF << 24;
	header |= 4;

	buff_size = 8;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}

int my_fflush(FILE *stream) {
	int header = FFLUSH << 24;
	header |= 4;

	buff_size = 8;
	buff = (char *)calloc(buff_size, 1);
	*(int *)(&buff[0]) = ENDIAN_FLIP(header);
	*(int *)(&buff[4]) = ENDIAN_FLIP((int)stream);

	/* send data */
	send_data((void *)buff, buff_size, 1);
	/* receive data */
	int len;
	char *pack = receive_data(&len, 0);
	int _ret = (int)*(int *)pack;
	free(pack);
	free(buff);
	return ENDIAN_FLIP(_ret);
}
