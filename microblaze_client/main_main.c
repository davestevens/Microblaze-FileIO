/*
  Tests of implemented functions
 */

#include <stdio.h>
#include <stdarg.h>

#define printf(...) my_printf(__VA_ARGS__)
#define fopen(...) my_fopen(__VA_ARGS__)
#define fclose(...) my_fclose(__VA_ARGS__)
#define fread(...) my_fread(__VA_ARGS__)
#define fwrite(...) my_fwrite(__VA_ARGS__)
#define fprintf(...) my_fprintf(__VA_ARGS__)
#define fgetc(...) my_fgetc(__VA_ARGS__)
#define fputc(...) my_fputc(__VA_ARGS__)
#define fscanf(...) my_fscanf(__VA_ARGS__)
#define feof(...) my_feof(__VA_ARGS__)
#define fflush(...) my_fflush(__VA_ARGS__)

int main_main(void) {
	printf("\nprintf test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "Hello, world!\n";
		int c = printf("%s", str);
		if(c != strlen(str)) {
			printf("printf test failed\n");
			return -1;
		}
		else {
			printf("printf test passed\n");
		}
	}

	printf("\nfopen test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "w";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("fopen test failed\n");
			return -1;
		}
		else {
			printf("fopen test passed (0x%08x)\n", (int)fp);

			printf("\nfclose test:\n");
			printf("--------------------------------------------------------------------------------\n");
			{
				printf("closing: %s (0x%08x)\n", str, (int)fp);
				if(fclose(fp) != 0) {
					printf("fclose test failed\n");
					return -1;
				}
				else {
					printf("fclose test passed\n");
				}
			}
		}
	}

	printf("\nfwrite test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "w";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		printf("populating int array with values 0...1000\n");
		int array[1000];
		int i;
		for(i=0;i<1000;i++) {
			array[i] = i;
		}
		printf("writing array to file\n");
		if(fwrite((void *)&array[0], 4, 1000, fp) != 1000) {
			printf("fwrite test failed\n");
			return -1;
		}
		else {
			printf("fwrite test passed\n");
		}
		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfread test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "r";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		int array[1000];
		printf("reading file into array\n");
		if(fread((void *)&array[0], 4, 1000, fp) != 1000) {
			printf("fread test failed\n");
			return -1;
		}
		else {
			printf("fread test passed\n");
		}
		printf("checking contents read\n");
		int i;
		for(i=0;i<1000;i++) {
			if(array[i] != i) {
				printf("[%d] element incorrect\n", i);
				printf("array[%d] = %d\n", i, array[i]);
				return -1;
			}
		}
		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfprintf test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "w";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		printf("printing int, float, double and string to file\n");
		int a = 42;
		float b = 984.2385;
		double c = 290856.8912491;

		char _str[256];
		char *fmt = "int: %d, float: %f, string: %s double: %lf\n";
		int len = printf(fmt, a, b, str, c);
		int flen = fprintf(fp, fmt, a, b, str, c);

		if(len != flen) {
			printf("fprintf test failed\n");
			return -1;
		}
		else {
			printf("fprintf test passed\n");
		}

		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfscanf test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "r";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		printf("scanning int, float, double and string to file\n");
		int e_a = 42;
		float e_b = 984.2385;
		double e_c = 290856.8912491;
		char *s_e = "testing";
		int a;
		float b;
		double c;
		char s[256] = {'\0'};

		char *fmt = "int: %d, float: %f, string: %s double: %lf\n";
		int len = fscanf(fp, fmt, &a, &b, s, &c);

		printf("int: %d\n", a);
		printf("float: %f\n", b);
		printf("double: %lf\n", c);
		printf("string: %s\n", s);

		if(len != 4) {
			printf("fscanf test failed\n");
			return -1;
		}
		printf("fscanf test passed\n");

		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfputc test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "w";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		printf("writing values 0...255 to file\n");
		int i;
		for(i=0;i<256;i++) {
			if(fputc(i, fp) != i) {
				printf("fputc test failed\n");
				return -1;
			}
		}
		printf("fputc test passed\n");

		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfgetc test:\n");
	printf("--------------------------------------------------------------------------------\n");
	{
		char *str = "/tmp/fopen_test.txt";
		char *mode = "r";
		printf("opening: %s in %s mode \n", str, mode);
		FILE *fp = fopen(str, mode);
		if(fp == NULL) {
			printf("failed to open file\n");
			return -1;
		}
		printf("reading values from file\n");
		int i;
		for(i=0;i<256;i++) {
			if(fgetc(fp) != i) {
				printf("fgetc test failed\n");
				return -1;
			}
		}
		printf("fgetc test passed\n");

		if(fclose(fp) != 0) {
			printf("failed to close file\n");
			return -1;
		}
	}

	printf("\nfeof test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfflush test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfgetpos test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfgets test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfputs test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfseek test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nfsetpos test:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("\nftell test:\n");
	printf("--------------------------------------------------------------------------------\n");

	return 0;
}
