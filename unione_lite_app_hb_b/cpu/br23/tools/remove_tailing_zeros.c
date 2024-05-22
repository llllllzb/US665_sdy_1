#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


void usage() {
  printf("usage: -i input -o output -mark ff\n");
}

unsigned char str2hex(char c) {
  unsigned char n = 16;
  if (c >= '0' && c <= '9') {
    n = c - '0';
  } else if (c >= 'a' && c <= 'f') {
    n = c + 10 - 'a';
  } else if (c >= 'A' && c <= 'F') {
    n = c + 10 - 'A';
  }
  return n;
}

int parse_arg(int argc, char *argv[], int index, char *input, char *output, unsigned char *tail) {
  static unsigned char arg_num = 0x0;
  if (strcmp(argv[index], "-i") == 0 && (arg_num & 0x1) == 0) {
    strcpy(input, argv[index + 1]);
    arg_num |= 0x1;
    printf("input: %s\n", input);
    return 0;
  } else if (strcmp(argv[index], "-o") == 0 && (arg_num & 0x2) == 0) {
    strcpy(output, argv[index + 1]);
    arg_num |= 0x2;
    printf("output: %s\n", output);
    return 0;
  } else if (strcmp(argv[index], "-mark") == 0  && (arg_num & 0x4) == 0) {
    unsigned char n1, n2;
    char *hs = argv[index + 1];
    if (strlen(hs) == 2) {
      n1 = str2hex(hs[0]);
      n2 = str2hex(hs[1]);
      if (n1 < 16 && n2 < 16) {
        *tail = n1 * 16 + n2;
        arg_num |= 0x4;
        printf("mark: 0x%02x\n", *tail);
        return 0;
      }
    }
  }
  printf("parse arg \"%s %s\" failed\n", argv[index], argv[index + 1]);
  return -1;
}

int main(int argc, char *argv[]) {
#if 0
  //generate test file
  unsigned char buf[10] = {0};
  unsigned char buf1[10] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
  unsigned char buf2[10] = {1,1,1,1,1,1,1,1,1,1};
  FILE *f = fopen("test.dat", "w");
  fwrite(buf, 1, 10, f);
  fwrite(buf1, 1, 10, f);
  fwrite(buf, 1, 10, f);
  fwrite(buf1, 1, 10, f);
  fwrite(buf2, 1, 10, f);
  fclose(f);
  return 0;
#endif
  char input[256] = {0};
  char output[256] = {0};
  unsigned char tail;

  if (argc != 7) {
    usage();
    exit(-1);
  }
  if (parse_arg(argc, argv, 1, input, output, &tail) != 0 ||
      parse_arg(argc, argv, 3, input, output, &tail) != 0 ||
      parse_arg(argc, argv, 5, input, output, &tail) != 0) {
    usage();
    exit(-2);
  }

  FILE *fi, *fo;
  int fi_len;
  unsigned char *data;
  fi = fopen(input, "rb");
  if (fi == NULL) {
    printf("open file %s failed\n", input);
    exit(-3);
  }
  fo = fopen(output, "wb");
  if (fi == NULL) {
    fclose(fi);
    printf("open file %s failed\n", output);
    exit(-4);
  }
  //process file
  fseek(fi, 0L, SEEK_END);
  fi_len = ftell(fi); 
  fseek(fi, 0L, SEEK_SET);
  if (fi_len == 0) {
    //exit without process
    fclose(fi);
    fclose(fo);
    return 0;
  }
  data = (unsigned char *)malloc(fi_len);
  if (data == NULL) {
    fclose(fi);
    fclose(fo);
    printf("malloc memory of %d byte failed\n", fi_len);
    exit(-5);
  }
  fread(data, fi_len, 1, fi);
  while (fi_len > 0 && data[fi_len - 1] != tail) {
    fi_len--;
  }
  printf("left size %d\n", fi_len);
  fwrite(data, fi_len, 1, fo);
  fclose(fi);
  fclose(fo);
  return 0;
}
