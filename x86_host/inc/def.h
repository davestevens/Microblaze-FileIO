/*
  Definitions across both systems
  Used to sync message types
 */
#define INIT       0xbabeface
#define DEINIT     0x6a11ba65
#define ACK        0xdeadbeef

#define MAX_PACKET 1000

#define PRINTF        1
#define FOPEN         2
#define FCLOSE        3
#define FREAD         4
#define FWRITE        5
#define FPRINTF       6
#define FGETC         7
#define FPUTC         8
#define FSCANF        9

#define FEOF         10
#define FFLUSH       11
#define FGETPOS      12
#define FGETS        13
#define FPUTS        14
#define FSEEK        15
#define FSETPOS      16
#define FTELL        17

#define TYPE_D        1
#define TYPE_F        2
#define TYPE_DB       3
#define TYPE_S        4
