#include "fat32.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  char line[10000];                         // for reading command line input
  char *image_filename = NULL;              // string for storing the file name
  image *image = calloc(1, sizeof(*image)); // image struct for image file
  if (image == NULL) {
    fprintf(stderr, "[!] Failed to allocate memory for image struct!\n");
    exit(-1);
  }

  // check for image file in command line arguments
  if (argc != 2) {
    // TL;DR : Create 20MB image if it's not provided as argument
    fprintf(stdout,
            "[NOTE] No image file provided, creating a 20MB empty image:\n");
    // Initialize the image structure
    fprintf(stdout, "[-] initializing image...\n");
    image_filename = "fat32.img";
    image->size = (unsigned long)(20 * 1024 * 1024); // 20MB
    image->buffer = (char *)calloc(image->size, sizeof(char));
    if (image->buffer == NULL) {
      fprintf(stderr, "[!] failed to allocate image->buffer!\n");
      exit(-2);
    }
    strcpy(image->filename, image_filename);
    // create image
    format_cmd(NULL, image);
  } else {
    // get image file name
    image_filename = (char *)calloc(strlen(argv[1]) + 1, sizeof(char));
    strcpy(image_filename, argv[1]);
  }

  // read image file in image struct
  fprintf(stdout, "[-] reading image file...\n");
  read_in_image_file(image_filename, image);
  image->boot_sector = read_boot_sector(image_filename);
  if (image->size == 0 || *(uint32_t *)image->boot_sector.fat_sz_32 == 0) {
    printf("[!] Unknown disk format, use 'format <filename>' to get valid FAT32"
           "image.\n    Or restart application with valid/without image file "
           "argument.\n");
    print_prompt(image);
    if (fgets(line, 256, stdin) != NULL)
      parse_input(line, image);
    else
      printf("[!] Error occured: %s", strerror(errno));
  }
  // read image file in image struct
  // fprintf(stdout, "[-] reading image file...\n");
  read_in_image_file(image_filename, image);
  image->boot_sector = read_boot_sector(image_filename);
  // set root cluster as current cluster
  fprintf(stdout, "[-] setting root cluster...\n");
  int root_cluster =
      hex_str_to_dec(image->boot_sector.root_clust, root_clust_size);
  image->current_cluster = root_cluster;

  int temp[get_total_clusters(image)];
  fprintf(stdout, "[-] obtaining FAT region...\n");
  get_FAT_region(image, temp);

  // Print Prompt
  fprintf(stdout, "[+] All set!\n\n");

  print_prompt(image);
  while (fgets(line, 200, stdin)) {
    if (dev)
      printf("\n\nline: %s\n", line);
    parse_input(line, image);
    print_prompt(image);
  }

  return 1;
};

// print the prompt with the directory
void print_prompt(const image *image) {
  char prompt[150] = " ";
  int i = 0;
  for (i = 0; i < image->prompt_depth; i++) {
    if (i != 0)
      strcat(prompt, " ");
    strcat(prompt, image->cwd[i]);
  }
  printf("\n%s > ", prompt);
}

// parse input from prompt and call the correct function
void parse_input(char *line, image *image) {

  // initial variables
  int line_cnt, i;

  // remove trailing whitespace & split line into array
  remove_trailing_space(line);
  char line_arr[100][100];
  split(line, " ", &line_cnt, line_arr);

  // grab command, first word in input
  char command[strlen(line_arr[0]) + 1];
  strcpy(command, line_arr[0]);

  // get arguments
  char args[10000];
  int start = strlen(command) + 1;
  for (i = start; (unsigned)i < strlen(line); i++) {
    args[i - start] = line[i];
  }
  args[i - start] = '\0';

  // declare error message variable
  char error_msg[150];
  bool check = true;

  // check for commands and call the correct function

  if (strcmp(command, "exit") == 0)
    exit_cmd(image);

  else if (strcmp(command, "info") == 0)
    info_cmd(image);

  else if (strcmp(command, "format") == 0)
    check = format_cmd(args, image);

  else if (strcmp(command, "ls") == 0)
    check = ls_cmd(args, image, error_msg);

  else if (strcmp(command, "cd") == 0)
    check = cd_cmd(args, image, error_msg);

  else if (strcmp(command, "touch") == 0)
    check = touch_cmd(args, image, error_msg);

  else if (strcmp(command, "mkdir") == 0)
    check = mkdir_cmd(args, image, error_msg);

  else if (strcmp(command, "rm") == 0)
    check = rm_cmd(args, image, error_msg);

  else if (strcmp(command, "rmdir") == 0)
    check = rmdir_cmd(args, image, error_msg);

  else
    printf("[] Command not recognized.\n");

  // print error message (if applicable)
  if (!check)
    printf("Error: %s", error_msg);
}
