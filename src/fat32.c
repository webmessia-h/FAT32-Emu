#include "../include/fat32.h"
#include <stdio.h>
#include <string.h>
// exit from the program
void exit_cmd(image *image) {
  free(image->buffer);
  exit(0);
}

// parse the boot sector. Print the field name and corresponding values
// for each entry, one per line (e.g. Bytes per Sector: 512)
void info_cmd(const image *image) {

  int i;

  printf("\njmp_boot: ");
  for (i = 0; i < jmp_boot_size; i++)
    printf("%x ", image->boot_sector.jmp_boot[i]);
  printf("\n");

  char OEMStr[oem_name_size + 1];
  hex_to_ASCII(image->boot_sector.OEM_name, oem_name_size, OEMStr);
  printf("OEM_name: %s\n", OEMStr);
  printf("byts_per_sec: %d\n",
         hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size));
  printf("sec_per_clust: %d\n",
         hex_str_to_dec(image->boot_sector.sec_per_clus, sec_per_clus_size));
  printf("rsvd_sec_cnt: %d\n",
         hex_str_to_dec(image->boot_sector.rsvd_sec_cnt, rsvd_sec_cnt_size));
  printf("num_fats: %d\n",
         hex_str_to_dec(image->boot_sector.num_fats, num_fats_size));
  printf("root_ent_cnt: %d\n",
         hex_str_to_dec(image->boot_sector.root_ent_cnt, root_ent_cnt_size));
  printf("tot_sec_16: %d\n",
         hex_str_to_dec(image->boot_sector.tot_sec_16, tot_sec_16_size));
  printf("media: %x\n", image->boot_sector.media[0]);
  printf("fat_sz_16: %d\n",
         hex_str_to_dec(image->boot_sector.fat_sz_16, fat_sz_16_size));
  printf("sec_per_trk: %d\n",
         hex_str_to_dec(image->boot_sector.sec_per_trk, sec_per_trk_size));
  printf("num_heads: %d\n",
         hex_str_to_dec(image->boot_sector.num_heads, num_heads_size));
  printf("hidd_sec: %d\n",
         hex_str_to_dec(image->boot_sector.hidd_sec, hidd_sec_size));
  printf("tot_sec_32: %d\n",
         hex_str_to_dec(image->boot_sector.tot_sec_32, tot_sec_32_size));
  printf("fat_sz_32: %d\n",
         hex_str_to_dec(image->boot_sector.fat_sz_32, fat_sz_32_size));
  printf("ext_flags: %d\n",
         hex_str_to_dec(image->boot_sector.ext_flags, ext_flags_size));
  printf("fs_ver: %d\n",
         hex_str_to_dec(image->boot_sector.fs_ver, fs_ver_size));
  printf("root_clust: %d\n",
         hex_str_to_dec(image->boot_sector.root_clust, root_clust_size));
  printf("fs_info: %d\n",
         hex_str_to_dec(image->boot_sector.fs_info, fs_info_size));
  printf("bk_boot_sec: %d\n",
         hex_str_to_dec(image->boot_sector.bk_boot_sec, bk_boot_sec_size));
  printf("reserved: %d\n",
         hex_str_to_dec(image->boot_sector.reserved, reserved_size));
  printf("drv_num: %d\n",
         hex_str_to_dec(image->boot_sector.drv_num, drv_num_size));
  printf("reservedl: %d\n",
         hex_str_to_dec(image->boot_sector.reservedl, reservedl_size));
  printf("boot_sig: %d\n",
         hex_str_to_dec(image->boot_sector.boot_sig, boot_sig_size));
  printf("vol_ID: %d\n",
         hex_str_to_dec(image->boot_sector.vol_ID, vol_id_size));

  char vol_lab_str[vol_lab_size + 1];
  hex_to_ASCII(image->boot_sector.vol_lab, vol_lab_size, vol_lab_str);
  printf("vol_lab: %s\n", vol_lab_str);

  char file_sys_type_str[fil_sys_type_size + 1];
  hex_to_ASCII(image->boot_sector.file_sys_type, fil_sys_type_size,
               file_sys_type_str);
  printf("file_sys_type: %s\n\n", file_sys_type_str);
}

// print the name field for the directories within the contents of DIRNAME
// including the “.” and “..” directories. For simplicity, you may print each of
// the directory entries on separate lines
bool ls_cmd(char args[], const image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];  // abirtrary amount of dir entries
  int dir_entry_count = 0; // current dir entry count
  int i, j;

  // if no arguments, then use cwd
  if (strcmp(args, "") == 0) {
    read_dir_entries_in_dir(image, image->current_cluster, entries,
                            &dir_entry_count);
  }

  // arguments passed
  else {

    // make sure no spaces in dir_name
    if (strchr(args, ' ') != NULL) {
      sprintf(error_msg, "directory name cannot contain spaces\n");
      return false;
    }

    // finding dirname in current directory
    int dir_index = -1;
    read_dir_entries_in_dir(image, image->current_cluster, entries,
                            &dir_entry_count);
    for (i = 0; i < dir_entry_count; i++) {
      char dir_name[name_size + 1];
      hex_to_ASCII(entries[i].name, name_size, dir_name);
      remove_trailing_space(dir_name);
      if (entries[i].attr[0] == 0x10 && strcmp(args, dir_name) == 0) {
        dir_index = i;
        break;
      }
    }
    dir_entry_count = 0; // reset this

    // handle directory not found
    if (dir_index == -1) {
      sprintf(error_msg, "directory \"%s\" does not exist\n", args);
      return false;
    }

    // get first cluster # of directory
    unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
    for (i = 0; i < fst_clus_lo_size; i++)
      first_cluster_num[i] = entries[dir_index].fst_clus_lo[i];
    for (i = 0; i < fst_clus_hi_size; i++)
      first_cluster_num[i + fst_clus_lo_size] =
          entries[dir_index].fst_clus_hi[i];

    // convert cluster number to integer
    int dir_clust_num =
        hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

    // read dir entries from first_cluster_num
    read_dir_entries_in_dir(image, dir_clust_num, entries, &dir_entry_count);
  }

  // create array of fixed directory entry structs
  dir_entry sort_directories[dir_entry_count];

  // load directory entries into array for sorting
  for (i = 0; i < dir_entry_count; i++)
    sort_directories[i] = entries[i];

  // sort temp array
  qsort(sort_directories, dir_entry_count, sizeof(const dir_entry),
        compare_dirs);

  // print out dir entry names
  printf("\n");
  for (i = 0; i < dir_entry_count; i++) {
    if (sort_directories[i].attr[0] == 0x10)
      printf(ANSI_COLOR_GREEN);
    char name[name_size];
    hex_to_ASCII(sort_directories[i].name, name_size, name);
    remove_trailing_space(name);
    if (strcmp(name, "") == 0)
      continue; // ignore empty dir entries
    printf("%s\n", name);
    if (sort_directories[i].attr[0] == 0x10)
      printf(ANSI_COLOR_RESET);
  }

  // return success
  return true;
}

// changes the current working directory to DIRNAME.
bool cd_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];  // abirtrary amount of dir entries
  int dir_entry_count = 0; // current dir entry count
  int i, j;

  // check arguments
  if (strcmp(args, "") == 0) {
    return true;
  }

  // make sure no spaces in dir_name
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "directory name cannot contain spaces\n");
    return false;
  }

  // finding dirname in current directory
  int dir_index = -1;
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  for (i = 0; i < dir_entry_count; i++) {
    char dir_name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, dir_name);
    remove_trailing_space(dir_name);
    if (entries[i].attr[0] == 0x10 && strcmp(args, dir_name) == 0) {
      dir_index = i;
      break;
    }
  }
  dir_entry_count = 0; // reset this

  // handle directory not found
  if (dir_index == -1) {
    sprintf(error_msg, "directory \"%s\" does not exist\n", args);
    return false;
  }

  // get first cluster # of directory
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] = entries[dir_index].fst_clus_hi[i];

  // convert cluster number to integer
  int dir_clust_num =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // set current_cluster to new cluster
  image->current_cluster = dir_clust_num;

  // set prompt
  if (image->current_cluster <=
      hex_str_to_dec(image->boot_sector.root_clust, root_clust_size))
    image->prompt_depth = 0;
  else {
    if (strcmp(args, "..") != 0 && (strcmp(args, ".") != 0)) {
      char name[name_size + 1];
      hex_to_ASCII(entries[dir_index].name, name_size, name);
      remove_trailing_space(name);
      strcpy(image->cwd[image->prompt_depth++], name);
    } else
      image->prompt_depth--;
  }

  // return success
  return true;
}

// prints the size of the file FILENAME in the current working directory in
// bytes
size_t size_cmd(char args[], const image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];  // abirtrary amount of dir entries
  int dir_entry_count = 0; // current dir entry count
  int i, j;

  // check arguments
  if (strcmp(args, "") == 0) {
    sprintf(error_msg, "no file provided\n");
    return false;
  }

  // make sure no spaces in dir_name
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "file name cannot contain spaces\n");
    return false;
  }

  // finding filename in current directory
  int file_index = -1;
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  for (i = 0; i < dir_entry_count; i++) {
    char filename[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, filename);
    remove_trailing_space(filename);
    if (entries[i].attr[0] == 0x20 && strcmp(args, filename) == 0) {
      file_index = i;
      break;
    }
  }
  dir_entry_count = 0; // reset this

  // handle directory not found
  if (file_index == -1) {
    sprintf(error_msg, "file \"%s\" does not exist\n", args);
    return false;
  }

  // read size variable
  size_t file_size;
  file_size = hex_str_to_dec(entries[file_index].file_size, file_size_size);
  printf("Size of file \"%s\": %zu\n", args, file_size);

  // return success
  return file_size;
}

// creates a file in the current working directory with a size of 0 bytes and
// with a name of FILENAME
bool touch_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  int clusters[1000];
  dir_entry entries[150];  // abirtrary amount of dir entries
  int dir_entry_count = 0; // current dir entry count
  int i, j;

  // check to make sure there is a filename given (strcmp(args,"") != 0)
  if (strcmp(args, "") == 0) {
    sprintf(error_msg, "no file name provided\n");
    return false;
  }

  // check to make sure the filename does not contain spaces
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "file name cannot contain spaces\n");
    return false;
  }

  // make sure no other dir entry in current directory has that name
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  for (i = 0; i < dir_entry_count; i++) {
    char filename[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, filename);
    remove_trailing_space(filename);
    if (strcmp(args, filename) == 0) {
      sprintf(error_msg, "\"%s\" already exits", args);
      return false;
    }
  }
  dir_entry_count = 0; // reset this

  // find available cluster
  int first_cluster = find_avail_cluster(image);
  if (first_cluster == -1) { // disk full, no more clusters
    sprintf(error_msg, "no more space on disk");
    return false;
  }
  unsigned char endingVal[] = {0xFF, 0xFF, 0xFF, 0xFF};
  set_FAT_table_value(image, first_cluster, endingVal);

  // create a new dir entry for the file
  dir_entry d = create_dir_entry(args, 0x20, first_cluster);

  // get associated clusters
  get_associated_clusters(image, image->current_cluster, clusters);

  // try to add dir entry to one of the clusters
  i = 0;
  bool added = false;
  while (clusters[i] != -1) {
    if (add_dir_entry_to_cluster(image, d, clusters[i])) {
      added = true;
      break;
    }
    i++;
  }

  // if there wasn't enough room in the available clusters
  // add a new cluster and then add the dir entry to that cluster
  if (!added) {
    int new_cluster = add_additional_cluster(image, image->current_cluster);
    if (new_cluster != -1)
      add_dir_entry_to_cluster(image, d, new_cluster);
  }

  // update image File
  if (!update_image_file(image)) {
    printf("ERROR: image file might be corrupted");
    exit(1);
  }

  // print success message
  printf("File \"%s\" created successfully.\n", args);

  // done - return true
  return true;
}

// creates a new directory in the current working directory with the name
// DIRNAME
bool mkdir_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  int clusters[1000];
  dir_entry entries[150];  // abitrary amount of dir entries
  int dir_entry_count = 0; // current dir entry count
  int i, j;

  // check to make sure there is a directory name given (strcmp(args,"") != 0)
  if (strcmp(args, "") == 0) {
    sprintf(error_msg, "no directory name provided\n");
    return false;
  }

  // check to make sure the directory name does not contain spaces
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "directory name cannot contain spaces\n");
    return false;
  }

  // make sure no other dir entry in current directory has that name
  // also make sure user not passing ".." or "." as directory names
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  for (i = 0; i < dir_entry_count; i++) {
    char dir_name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, dir_name);
    remove_trailing_space(dir_name);
    if (strcmp(args, dir_name) == 0 || strcmp(args, ".") == 0 ||
        strcmp(args, "..") == 0) {
      sprintf(error_msg, "\"%s\" already exits", args);
      return false;
    }
  }
  dir_entry_count = 0; // reset this

  // find available cluster
  int dir_cluster = find_avail_cluster(image);
  if (dir_cluster == -1) { // disk full, no more clusters
    sprintf(error_msg, "no more space on disk");
    return false;
  }
  unsigned char endingVal[] = {0xFF, 0xFF, 0xFF, 0xFF};
  set_FAT_table_value(image, dir_cluster, endingVal);

  // create a new dir entry for the file
  dir_entry d = create_dir_entry(args, 0x10, dir_cluster);

  // get associated clusters
  get_associated_clusters(image, image->current_cluster, clusters);

  // try to add dir entry to one of the clusters
  i = 0;
  bool added = false;
  while (clusters[i] != -1) {
    if (add_dir_entry_to_cluster(image, d, clusters[i])) {
      added = true;
      break;
    }
    i++;
  }

  // if there wasn't enough room in the available clusters
  // add a new cluster and then add the dir entry to that cluster
  if (!added) {
    int new_cluster = add_additional_cluster(image, image->current_cluster);
    if (new_cluster != -1)
      add_dir_entry_to_cluster(image, d, new_cluster);
  }

  // write "." directory
  dir_entry currentDir = create_dir_entry(".", 0x10, dir_cluster);
  add_dir_entry_to_cluster(image, currentDir, dir_cluster);

  // write ".." directory
  dir_entry parentDir = create_dir_entry("..", 0x10, image->current_cluster);
  add_dir_entry_to_cluster(image, parentDir, dir_cluster);

  // update image File
  if (!update_image_file(image)) {
    printf("[!] ERROR: image file might be corrupted");
    exit(1);
  }

  // print success message
  printf("Directory \"%s\" created successfully.\n", args);

  return true;
}

// opens a file named FILENAME in the current working directory. A file can only
// be read from or written to if it is opened first. You will need to maintain a
// table of opened files and add FILENAME to it when open is called MODE is a
// string and is only valid if it is one of the following:
bool open_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];
  int i, mode, argsCount = 0, dir_entry_count = 0;
  char arguments[100][100];
  char filename[150];

  // check to make sure there is a filename given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg,
            "command \"open\" is missing arguments: FILENAME MODE\n");
    return false;
  }

  // split arguments
  split(args, " ", &argsCount, arguments);
  if (argsCount != 2) {
    sprintf(error_msg, "too many or too few arguments passed\n");
    return false;
  }

  // get filename and mode
  strcpy(filename, arguments[0]);
  if (strcmp(arguments[1], "r") == 0)
    mode = 0;
  else if (strcmp(arguments[1], "w") == 0)
    mode = 1;
  else if (strcmp(arguments[1], "rw") == 0)
    mode = 2;
  else if (strcmp(arguments[1], "wr") == 0)
    mode = 3;
  else {
    sprintf(error_msg,
            "an invalid mode has been used. MODE may be: r w rw wr\n");
    return false;
  }

  // confirm file is not already open
  for (i = 0; i < image->open_files_cnt; i++) {
    if (strcmp(filename, image->open_files[i].filename) == 0) {
      sprintf(error_msg, "file \"%s\" is already open\n", filename);
      return false;
    }
  }

  // read in all dir entries from current directory
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);

  // find specified file index in current directory
  int file_index = -1;
  for (i = 0; i < dir_entry_count; i++) {
    char name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, name);
    remove_trailing_space(name);
    if (entries[i].attr[0] == 0x20 && strcmp(filename, name) == 0) {
      file_index = i;
      break;
    }
  }

  // handle directory not found
  if (file_index == -1) {
    sprintf(error_msg, "file \"%s\" does not exist\n", filename);
    return false;
  }

  // get cluster number of specified file
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[file_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] =
        entries[file_index].fst_clus_hi[i];

  // convert cluster number to integer
  int clusNumber =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // add open file to array of open files in image struct
  strcpy(image->open_files[image->open_files_cnt].filename, filename);
  image->open_files[image->open_files_cnt].first_cluster = clusNumber;
  image->open_files[image->open_files_cnt].mode = mode;
  image->open_files_cnt++;

  // print success message
  printf("File opened successfully: %s\n", filename);

  // print open files
  printf("Open Files: ");
  for (i = 0; i < image->open_files_cnt; i++)
    printf("%s ", image->open_files[i].filename);
  printf("\n");

  return true;
}

// closes a file named FILENAME. Needs to remove the file entry from the open
// file table
bool close_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  int i;

  // check to make sure there is a filename given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg, "command \"close\" is missing argument: FILENAME\n");
    return false;
  }

  // check to make sure the file name does not contain spaces
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "file name cannot contain spaces\n");
    return false;
  }

  // look for file in currently open files
  int index = -1;
  for (i = 0; i < image->open_files_cnt; i++) {
    if (strcmp(args, image->open_files[i].filename) == 0) {
      index = i;
      break;
    }
  }

  // handle file does not exists
  if (index == -1) {
    sprintf(error_msg, "file is not currently open\n");
    return false;
  }

  // remove file from list and shift list over
  for (i = index; i < image->open_files_cnt - 1; i++) {
    strcpy(image->open_files[i].filename, image->open_files[i + 1].filename);
    image->open_files[i].first_cluster = image->open_files[i + 1].first_cluster;
    image->open_files[i].mode = image->open_files[i + 1].mode;
  }
  image->open_files_cnt--;

  // printf success message
  printf("File closed successfully: %s\n", args);

  // print open files
  printf("Open Files: ");
  for (i = 0; i < image->open_files_cnt; i++)
    printf("%s ", image->open_files[i].filename);
  printf("\n");

  return true;
}

// read the data from a file in the current working directory with the name
// FILENAME. Start reading from the file at OFFSET bytes and stop after reading
// SIZE bytes. If the OFFSET+SIZE is larger than the size of the file, just read
// size-OFFSET bytes starting at OFFSET
bool read_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];
  int i, mode, argsCount = 0, dir_entry_count = 0;
  int offset, size, first_cluster;
  int clusters[150];
  char arguments[100][100];
  char filename[150];

  // check to make sure there is a filename given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg,
            "command \"read\" is missing arguments: FILENAME OFFSET SIZE\n");
    return false;
  }

  // split arguments
  split(args, " ", &argsCount, arguments);
  if (argsCount != 3) {
    sprintf(error_msg, "too many or too few arguments passed\n");
    return false;
  }

  // get filename
  strcpy(filename, arguments[0]);

  // convert offset and size to ints
  offset = atoi(arguments[1]);
  size = atoi(arguments[2]);

  // look for file in currently open files
  if (image->open_files_cnt == 0) {
    sprintf(error_msg, "file is not currently open or is not readable\n");
    return false;
  }
  for (i = 0; i < image->open_files_cnt; i++) {
    if (strcmp(filename, image->open_files[i].filename) == 0 &&
        image->open_files[i].mode != 1) {
      break;
    } else if (i == image->open_files_cnt - 1) {
      sprintf(error_msg, "file is not currently open or is not readable\n");
      return false;
    }
  }

  // read in all dir entries from current directory
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);

  // find specified file index in current directory
  int file_index = -1;
  for (i = 0; i < dir_entry_count; i++) {
    char name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, name);
    remove_trailing_space(name);
    if (entries[i].attr[0] == 0x20 && strcmp(filename, name) == 0) {
      file_index = i;
      break;
    }
  }

  // handle file not found
  if (file_index == -1) {
    sprintf(error_msg, "file \"%s\" does not exist\n", filename);
    return false;
  }

  // check offset and size
  int file_size = hex_str_to_dec(entries[file_index].file_size, file_size_size);
  if (offset > file_size) {
    sprintf(error_msg, "offset is larger than file size\n");
    return false;
  }

  // get starting clusters of specified file
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[file_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] =
        entries[file_index].fst_clus_hi[i];
  first_cluster =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // get associated clusters
  get_associated_clusters(image, first_cluster, clusters);

  // get bytes per sector
  int bytesPerSector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);

  // loop through clusters
  unsigned char fileBuffer[size];
  int fileBufferIndex = 0;
  int tempOffset = offset;
  i = 0;
  while (clusters[i] != -1) {

    // skip cluster if offset is bigger than cluster size
    if (tempOffset >= bytesPerSector) {
      tempOffset -= bytesPerSector;
      continue;
    }

    // set cluster position
    int currentClusPos;
    if (fileBufferIndex == 0)
      currentClusPos = tempOffset;
    else
      currentClusPos = 0;

    // get buffer position of cluster
    int bufferPos =
        get_data_start_index(image) + bytesPerSector * (clusters[i] - 2);

    // loop through cluster and read bytes
    while ((fileBufferIndex < size) && (currentClusPos < bytesPerSector) &&
           (offset + fileBufferIndex < file_size)) {
      fileBuffer[fileBufferIndex] = image->buffer[bufferPos + currentClusPos];
      fileBufferIndex++;
      currentClusPos++;
    }

    // if index is equal to size, then break from loop
    if (fileBufferIndex >= size)
      break;

    i++;
  }

  // convert byteBuffer to a string
  char str[fileBufferIndex + 1];
  hex_to_ASCII(fileBuffer, fileBufferIndex, str);

  // print what was read (character by character)
  for (i = 0; i < fileBufferIndex; i++)
    printf("%c", str[i]);
  printf("\n");

  // return success
  return true;
}

bool format_cmd(char args[], image *image, char *error_msg) {
  if (args != NULL) {
    // if user provided some file as input
    image = calloc(1, sizeof(*image)); // image struct for image file
    if (image == NULL) {
      fprintf(stderr, "[!] Failed to allocate memory for image struct!\n");
      exit(-1);
    }
    // copy provided filename, open file, calculate size, write
    remove_trailing_space(args);
    strcpy(image->filename, args);
    // initial variables
    FILE *image_file;
    int bytes;

    // open file
    image_file = fopen(image->filename, "rb");
    if (!image_file) { // check if file open correctly
      fprintf(stderr, "[!] ERROR: Unable to open file\n");
      exit(1);
    }
    // get file size
    fseek(image_file, 0, SEEK_END);  // traverse file
    image->size = ftell(image_file); // get size of file
    rewind(image_file);              // rewind to file start
    fclose(image_file);
    if (image->size < (20 * 1024 * 1024)) {
      printf("[NOTE] provided file is smaller than 20MB, adjusting...\n");
      image->size = (20 * 1024 * 1024);
    }
    image->buffer = calloc(image->size, sizeof(unsigned char));
    // format
    if (!init_boot_sector(image) || !write_boot_sector(image) ||
        !setup_FAT_region(image) || !setup_root_directory(image) ||
        !update_image_file(image)) {
      sprintf(error_msg, "%s", stderr);
      return false;
    } else
      return true;
  } // if user started application with no args
  if (!init_boot_sector(image) || !write_boot_sector(image) ||
      !setup_FAT_region(image) || !setup_root_directory(image) ||
      !update_image_file(image)) {
    sprintf(error_msg, "%s", stderr);
    return false;
  } else
    return true;
}

// writes to a file in the current working directory with the name FILENAME.
// Start writing at OFFSET bytes and stop after writing SIZE bytes. If
// OFFSET+SIZE is larger than the size of the file, you will need to extend the
// length of the file to at least hold the data being written
bool write_cmd(char args[], image *image, char error_msg[]) {

  // initial variables
  dir_entry entries[150];
  int i, j, mode, argsCount = 0, dir_entry_count = 0;
  int offset, size, first_cluster;
  int clusters[100000];
  char filename[150];
  char str[10000];
  char *temp_args;
  char arguments[100][100];
  bool edgeCase = false;
  int clusIndex;
  int clusterPos, fatPos, oldClusters, clusBytes, bufferPos, index,
      currentClusPos;

  // check to make sure there is a filename given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg, "command \"write\" is missing arguments: FILENAME "
                       "OFFSET SIZE STRING\n");
    return false;
  }

  // confirm string is in command and extract it
  int quote_count = 0;
  j = 0;
  for (i = 0; i < strlen(args); i++) {
    if (args[i] == '"')
      quote_count++;
    if (quote_count == 1 && args[i] != '"')
      str[j++] = args[i];
  }
  if (quote_count != 2) {
    sprintf(error_msg, "no string passed\n");
    return false;
  }
  str[j] = '\0';

  // trim off string from input
  temp_args = strtok(args, "\"");
  remove_trailing_space(temp_args);

  // split arguments
  split(temp_args, " ", &argsCount, arguments);
  if (argsCount != 3) {
    sprintf(error_msg, "too many or too few arguments passed\n");
    return false;
  }

  // get filename and string
  strcpy(filename, arguments[0]);

  // convert offset and size to ints
  offset = atoi(arguments[1]);
  size = atoi(arguments[2]);

  // look for file in currently open files
  if (image->open_files_cnt == 0) {
    sprintf(error_msg, "file is not currently open or is not writeable\n");
    return false;
  }
  for (i = 0; i < image->open_files_cnt; i++) {
    if (strcmp(filename, image->open_files[i].filename) == 0) {
      if (image->open_files[i].mode >= 1) { // check is writable
        break;
      }
    } else if (i == image->open_files_cnt - 1) {
      sprintf(error_msg, "file is not currently open or is not writeable\n");
      return false;
    }
  }

  // read in all dir entries from current directory
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);

  // find specified file index in current directory
  int file_index = -1;
  for (i = 0; i < dir_entry_count; i++) {
    char name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, name);
    remove_trailing_space(name);
    if (entries[i].attr[0] == 0x20 && strcmp(filename, name) == 0) {
      file_index = i;
      break;
    }
  }

  // handle directory not found
  if (file_index == -1) {
    sprintf(error_msg, "file \"%s\" does not exist\n", filename);
    return false;
  }

  // get starting clusters of specified file
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[file_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] =
        entries[file_index].fst_clus_hi[i];
  first_cluster =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // get bytes per sector
  int bytesPerSector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);

  // calculate the number of new clusters (if any) needed to hold new data
  int file_size = hex_str_to_dec(entries[file_index].file_size, file_size_size);
  if (offset > file_size) {
    sprintf(error_msg, "offset is larger than file size");
    return false;
  }

  // calculate the number of new clusters (if any) needed to hold new data
  int additionalBytes = (offset + size) - file_size;
  int additionalClusters = 0;
  if (additionalBytes > 0)
    additionalClusters = ((file_size + additionalBytes) / bytesPerSector) -
                         (file_size / bytesPerSector);

  // add additional clusters to file (if needed)
  for (i = 0; i < additionalClusters; i++) {
    int check = add_additional_cluster(image, first_cluster);
    if (check == -1) {
      sprintf(error_msg, "no more space on disk\n");
      return false;
    }
  }

  // check for edge case
  if ((offset + size) < file_size)
    edgeCase = true;

  // get associated clusters
  get_associated_clusters(image, first_cluster, clusters);

  // loop through clusters and write string
  int stringIndex = 0;
  int tempOffset = offset;
  i = 0;
  while (clusters[i] != -1) {

    // skip cluster if offset is bigger than cluster size
    if (tempOffset >= bytesPerSector) {
      tempOffset -= bytesPerSector;
      continue;
    }

    // set cluster position
    int currentClusPos;
    if (stringIndex == 0)
      currentClusPos = tempOffset;
    else
      currentClusPos = 0;

    // overwrite buffer with string
    while (stringIndex < size && currentClusPos < bytesPerSector) {
      bufferPos = get_data_start_index(image);
      bufferPos += bytesPerSector * (clusters[i] - 2);
      bufferPos += currentClusPos;

      if (stringIndex < strlen(str))
        image->buffer[bufferPos] = (unsigned char)str[stringIndex];
      else
        image->buffer[bufferPos] = 0x00;

      stringIndex++;
      currentClusPos++;
    }

    i++;
  }

  // handle edge case
  // remove clusters and unlink FAT table entries if use case encountered
  if (edgeCase) {
    // get the clusters to remove
    int clusToRemove = (1.0 * (file_size - (offset + size)) / bytesPerSector);

    // get cluster count
    int totalClusters = 0;
    while (clusters[totalClusters] != -1)
      totalClusters++;

    // loop through clusters
    for (i = totalClusters - 1; i > totalClusters - clusToRemove; i--) {
      clusterPos =
          get_data_start_index(image) + (clusters[i] - 2) * bytesPerSector;
      fatPos = get_FAT_start_index(image) + (clusters[i] * 4);
      for (j = 0; j < bytesPerSector; j++)
        image->buffer[clusterPos + j] = 0x00;
      for (j = 0; j < 4; j++)
        image->buffer[fatPos + j] = 0x00;
    }

    // clear out the new "final" cluster
    int finalCluster = clusters[totalClusters - clusToRemove];
    int leftover = (offset + size) % bytesPerSector;

    // clear out the unecessary bytes in the cluster
    clusterPos =
        get_data_start_index(image) + (finalCluster - 2) * bytesPerSector;
    fatPos = get_FAT_start_index(image) + (finalCluster * 4);
    for (j = leftover; j < bytesPerSector; j++)
      image->buffer[clusterPos + j] = 0x00;
    for (j = 0; j < 4; j++)
      image->buffer[fatPos + j] = 0xFF;

    // update file size
    file_size = offset + size;
    bufferPos = get_data_start_index(image);
    bufferPos += (image->current_cluster - 2) * bytesPerSector;
    bufferPos += file_index * 64 + 32 + 28;
    for (i = 0; i < sizeof(file_size); i++)
      image->buffer[bufferPos + i] = *((unsigned char *)&file_size + i);
  }

  // update file size
  if (!edgeCase) {
    if (additionalBytes > 0) {
      unsigned char bytes[4];
      int bufferPos = get_data_start_index(image);
      bufferPos += (image->current_cluster - 2) * bytesPerSector;
      bufferPos += file_index * 64 + 32 + 28;
      file_size += additionalBytes;
      int_to_u_char(file_size, bytes);
      for (i = 0; i < 4; i++)
        image->buffer[bufferPos + i] = bytes[i];
    }
  } else {
    get_associated_clusters(image, image->current_cluster, clusters);
    bool found = false;
    index = 0;
    while (clusters[index] != -1) {
      // store the cluster
      unsigned char cluster[get_cluster_size(image)];
      get_cluster(image, clusters[index], cluster);

      // loop through dir entries in specific cluster
      for (i = 1; i <= 8; i++) {
        dir_entry d = read_dir_entry(cluster, i);
        char dir_name[name_size + 1];
        hex_to_ASCII(d.name, name_size, dir_name);
        remove_trailing_space(dir_name);
        if (strcmp(args, dir_name) == 0) {
          found = true;
          file_index = i;
          break;
        }
      }
      if (found)
        break;

      index++;
    }

    unsigned char bytes[4];
    int bufferPos = get_data_start_index(image) +
                    (clusters[index] - 2) * bytesPerSector +
                    (file_index - 1) * 64;
    file_size = offset + size;
    int_to_u_char(file_size, bytes);
    for (i = 0; i < 4; i++)
      image->buffer[bufferPos + i] = bytes[i];
  }

  // update image file
  if (!update_image_file(image)) {
    printf("ERROR: image file might be corrupted");
    exit(1);
  }

  // print success message
  printf("Successfully wrote to file %s.\n", filename);

  // return succes
  return true;
}

// deletes the file named FILENAME from the current working directory. This
// needs remove the entry in the directory as well as reclaiming the actual file
// data
bool rm_cmd(char args[], image *image, char error_msg[]) {
  dir_entry entries[150];
  int clusters[150];
  int dir_clusters[5];
  int dir_entry_count = 0, index = 0;
  int i;

  // check to make sure there is a filename given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg, "no file name specified");
    return false;
  }

  // check to make sure the file name does not contain spaces
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "file name cannot contain spaces\n");
    return false;
  }

  // look for file in currently open files
  for (i = 0; i < image->open_files_cnt; i++) {
    if (strcmp(args, image->open_files[i].filename) == 0) {
      sprintf(error_msg, "file is currently open\n");
      return false;
    }
  }

  // get all directory entries in current directory
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  int dir_index = -1;

  // find specified dirs index in current directory
  for (i = 0; i < dir_entry_count; i++) {
    char dir_name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, dir_name);
    remove_trailing_space(dir_name);
    if (entries[i].attr[0] == 0x20 && strcmp(args, dir_name) == 0) {
      dir_index = i;
      break;
    }
  }

  // handle directory not found
  if (dir_index == -1) {
    sprintf(error_msg, "file \"%s\" does not exist\n", args);
    return false;
  }

  // get cluster number of specified file
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] = entries[dir_index].fst_clus_hi[i];

  // convert cluster number to integer
  int clusNumber =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // calculate files cluster index inside data region
  int bytesPerSector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);

  // get all clusters associated with file
  get_associated_clusters(image, clusNumber, clusters);

  // loop through all clusters associated with file
  while (clusters[index] != -1) {
    int clusterPos =
        get_data_start_index(image) + (clusters[index] - 2) * bytesPerSector;
    int fatPos = get_FAT_start_index(image) + (clusters[index] * 4);

    // remove dirEntry from data region
    for (i = 0; i < 512; i++)
      image->buffer[clusterPos + i] = 0x00;

    // remove specified file from FAT region
    for (i = 0; i < 4; i++)
      image->buffer[fatPos + i] = 0x00;

    index++;
  }

  // get all clusters associated with file
  get_associated_clusters(image, image->current_cluster, clusters);

  // loop through all clusters associated with file
  bool found = false;
  index = 0;
  while (clusters[index] != -1) {
    // store the cluster
    unsigned char cluster[get_cluster_size(image)];
    get_cluster(image, clusters[index], cluster);

    // loop through dir entries in specific cluster
    for (i = 1; i <= 8; i++) {
      dir_entry d = read_dir_entry(cluster, i);
      char dir_name[name_size + 1];
      hex_to_ASCII(d.name, name_size, dir_name);
      remove_trailing_space(dir_name);
      if (strcmp(args, dir_name) == 0) {
        found = true;
        dir_index = i;
        break;
      }
    }
    if (found)
      break;

    index++;
  }

  // remove dir entry from parent directory
  if (found) {
    int clusterPos = get_data_start_index(image) +
                     (clusters[index] - 2) * bytesPerSector +
                     (dir_index - 1) * 64;
    for (i = 0; i < 64; i++)
      image->buffer[clusterPos + i] = 0x00;
  }

  // update image file
  if (!update_image_file(image)) {
    printf("ERROR: image file might be corrupted");
    exit(1);
  }

  // print success message
  printf("File \"%s\" removed successfully.\n", args);

  return true;
}

// removes a directory by the name of DIRNAME from the current working
// directory. Make sure to remove the entry from the current working directory
// and to remove the data DIRNAME points to
bool rmdir_cmd(char args[], image *image, char error_msg[]) {

  dir_entry entries[150];
  int clusters[150];
  int dir_clusters[5];
  int dir_entry_count = 0, index = 0;

  // check to make sure there is a directory name given
  if ((strcmp(args, "") == 0)) {
    sprintf(error_msg, "no directory name specified");
    return false;
  }

  // check to make sure the directory name does not contain spaces
  if (strchr(args, ' ') != NULL) {
    sprintf(error_msg, "directory name cannot contain spaces\n");
    return false;
  }

  // make sure user isn't passing in "." or ".." as directory names
  if (strcmp(args, ".") == 0 || strcmp(args, "..") == 0) {
    sprintf(error_msg, "failed to remove \"%s\": Invalid argument\n", args);
    return false;
  }

  // get all directory entries in current directory
  read_dir_entries_in_dir(image, image->current_cluster, entries,
                          &dir_entry_count);
  int i;
  int dir_index = -1;

  // find specified dirs index in current directory
  for (i = 0; i < dir_entry_count; i++) {
    char dir_name[name_size + 1];
    hex_to_ASCII(entries[i].name, name_size, dir_name);
    remove_trailing_space(dir_name);
    if (entries[i].attr[0] == 0x10 && strcmp(args, dir_name) == 0) {
      dir_index = i;
      break;
    }
  }

  // handle directory not found
  if (dir_index == -1) {
    sprintf(error_msg, "directory \"%s\" does not exist\n", args);
    return false;
  }

  // get cluster number of specified directory
  unsigned char first_cluster_num[fst_clus_hi_size + fst_clus_lo_size];
  for (i = 0; i < fst_clus_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clus_lo[i];
  for (i = 0; i < fst_clus_hi_size; i++)
    first_cluster_num[i + fst_clus_lo_size] = entries[dir_index].fst_clus_hi[i];

  // convert cluster number to integer
  int dir_clust_num =
      hex_str_to_dec(first_cluster_num, fst_clus_lo_size + fst_clus_hi_size);

  // get all directory entries inside specified directory
  read_dir_entries_in_dir(image, dir_clust_num, entries, &dir_entry_count);

  // handle directory-not-empty
  if (dir_entry_count > 2) {
    sprintf(error_msg, "directory \"%s\" is not empty\n", args);
    return false;
  }

  // calculate dirs cluster index inside data region
  int bytesPerSector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);
  int clusterPos =
      get_data_start_index(image) + (dir_clust_num - 2) * bytesPerSector;
  int fatPos = get_FAT_start_index(image) + dir_clust_num * 4;

  // remove "." and ".." from data region
  for (i = 0; i < bytesPerSector; i++)
    image->buffer[clusterPos + i] = 0x00;

  // remove specified directory from FAT region
  for (i = 0; i < 4; i++)
    image->buffer[fatPos + i] = 0x00;

  // get all clusters associated with file
  get_associated_clusters(image, image->current_cluster, clusters);

  // loop through all clusters associated with file
  bool found = false;
  index = 0;
  while (clusters[index] != -1) {
    // store the cluster
    unsigned char cluster[get_cluster_size(image)];
    get_cluster(image, clusters[index], cluster);

    // loop through dir entries in specific cluster
    for (i = 1; i <= 8; i++) {
      dir_entry d = read_dir_entry(cluster, i);
      char dir_name[name_size + 1];
      hex_to_ASCII(d.name, name_size, dir_name);
      remove_trailing_space(dir_name);
      if (strcmp(args, dir_name) == 0) {
        found = true;
        dir_index = i;
        break;
      }
    }
    if (found)
      break;

    index++;
  }

  // remove dir entry from parent directory
  if (found) {
    int clusterPos = get_data_start_index(image) +
                     (clusters[index] - 2) * bytesPerSector +
                     (dir_index - 1) * 64;
    for (i = 0; i < 64; i++)
      image->buffer[clusterPos + i] = 0x00;
  }

  // update image file
  if (!update_image_file(image)) {
    printf("ERROR: image file might be corrupted");
    exit(1);
  }

  // print success message
  printf("Directory \"%s\" removed successfully.\n", args);

  return true;
}
