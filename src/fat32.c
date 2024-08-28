#include "../include/fat32.h"
#include <stdio.h>
#include <string.h>
// exit from the program
void exit_cmd(image *image) {
  free(image->buffer);
  exit(0);
}

/* @brief
 * parse the boot sector. Print the field name and corresponding values
 * for each entry, one per line (e.g. Bytes per Sector: 512)
 */
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

/* @brief
 * print the name field for the directories within the contents of DIRNAME
 * including the “.” and “..” directories. For simplicity, you may print each of
 * the directory entries on separate lines
*/
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
    unsigned char first_cluster_num[fst_clust_hi_size + fst_clust_lo_size];
    for (i = 0; i < fst_clust_lo_size; i++)
      first_cluster_num[i] = entries[dir_index].fst_clust_lo[i];
    for (i = 0; i < fst_clust_hi_size; i++)
      first_cluster_num[i + fst_clust_lo_size] =
          entries[dir_index].fst_clust_hi[i];

    // convert cluster number to integer
    int dir_clust_num = hex_str_to_dec(first_cluster_num,
                                       fst_clust_lo_size + fst_clust_hi_size);

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
  unsigned char first_cluster_num[fst_clust_hi_size + fst_clust_lo_size];
  for (i = 0; i < fst_clust_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clust_lo[i];
  for (i = 0; i < fst_clust_hi_size; i++)
    first_cluster_num[i + fst_clust_lo_size] =
        entries[dir_index].fst_clust_hi[i];

  // convert cluster number to integer
  int dir_clust_num =
      hex_str_to_dec(first_cluster_num, fst_clust_lo_size + fst_clust_hi_size);

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

/* @brief
 * creates a file in the current working directory
 * with a size of 0 bytes and with a name of FILENAME
 */
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
  unsigned char end_val[] = {0xFF, 0xFF, 0xFF, 0xFF};
  set_FAT_table_value(image, first_cluster, end_val);

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

/* @brief
 * creates a new directory in the current working directory
 * with the name DIRNAME
 */
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
  unsigned char end_val[] = {0xFF, 0xFF, 0xFF, 0xFF};
  set_FAT_table_value(image, dir_cluster, end_val);

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

/* @brief
 * opens a file named FILENAME in the current working directory. A file can only
 * be read from or written to if it is opened first. You will need to maintain a
 * table of opened files and add FILENAME to it when open is called MODE is a
 * string and is only valid if it is one of the following:
 */
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
  unsigned char first_cluster_num[fst_clust_hi_size + fst_clust_lo_size];
  for (i = 0; i < fst_clust_lo_size; i++)
    first_cluster_num[i] = entries[file_index].fst_clust_lo[i];
  for (i = 0; i < fst_clust_hi_size; i++)
    first_cluster_num[i + fst_clust_lo_size] =
        entries[file_index].fst_clust_hi[i];

  // convert cluster number to integer
  int clusNumber =
      hex_str_to_dec(first_cluster_num, fst_clust_lo_size + fst_clust_hi_size);

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

/* @brief
 * formats FILENAME file to a valid FAT32 image.
 * If provided file size is less than 32MB, it will grow
 * to match this size.
 * Initializes boot_sector, sets up FAT region,
 * creates root directory, sets up root directory
 * AND //writes// it to a FILENAME file.
 * FILENAME is either provided as argument or as image->filename
 * struct member field.
 *  TODO: add options so formatting is not so hardcoded
 */
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
    if (image->size < (32 * 1024 * 1024)) {
      printf("[NOTE] provided file is smaller than 32MB, adjusting...\n");
      image->size = (32 * 1024 * 1024);
    }
    image->buffer = calloc(image->size, sizeof(unsigned char));
    // format
    if (!init_boot_sector(image) || !write_boot_sector(image) ||
        !setup_FAT_region(image) || !setup_root_directory(image) ||
        !update_image_file(image)) {
      return false;
    } else
      return true;
  } // if user started application with no args
  if (!init_boot_sector(image) || !write_boot_sector(image) ||
      !setup_FAT_region(image) || !setup_root_directory(image) ||
      !update_image_file(image)) {
    return false;
  } else
    return true;
}

/* @brief
 * deletes the file named FILENAME from the current working directory.
 * This needs remove the entry in the directory
 * as well as reclaiming the actual file data
 */
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
  unsigned char first_cluster_num[fst_clust_hi_size + fst_clust_lo_size];
  for (i = 0; i < fst_clust_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clust_lo[i];
  for (i = 0; i < fst_clust_hi_size; i++)
    first_cluster_num[i + fst_clust_lo_size] =
        entries[dir_index].fst_clust_hi[i];

  // convert cluster number to integer
  int clusNumber =
      hex_str_to_dec(first_cluster_num, fst_clust_lo_size + fst_clust_hi_size);

  // calculate files cluster index inside data region
  int bytes_per_sect =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);

  // get all clusters associated with file
  get_associated_clusters(image, clusNumber, clusters);

  // loop through all clusters associated with file
  while (clusters[index] != -1) {
    int cluster_pos =
        get_data_start_index(image) + (clusters[index] - 2) * bytes_per_sect;
    int fatPos = get_FAT_start_index(image) + (clusters[index] * 4);

    // remove dirEntry from data region
    for (i = 0; i < 512; i++)
      image->buffer[cluster_pos + i] = 0x00;

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
    int cluster_pos = get_data_start_index(image) +
                      (clusters[index] - 2) * bytes_per_sect +
                      (dir_index - 1) * 64;
    for (i = 0; i < 64; i++)
      image->buffer[cluster_pos + i] = 0x00;
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

/* @brief removes a directory by the name of DIRNAME
 * from the current working directory.
 * Make sure to remove the entry from the current working directory
 * and to remove the data DIRNAME points to
 */
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
  unsigned char first_cluster_num[fst_clust_hi_size + fst_clust_lo_size];
  for (i = 0; i < fst_clust_lo_size; i++)
    first_cluster_num[i] = entries[dir_index].fst_clust_lo[i];
  for (i = 0; i < fst_clust_hi_size; i++)
    first_cluster_num[i + fst_clust_lo_size] =
        entries[dir_index].fst_clust_hi[i];

  // convert cluster number to integer
  int dir_clust_num =
      hex_str_to_dec(first_cluster_num, fst_clust_lo_size + fst_clust_hi_size);

  // get all directory entries inside specified directory
  read_dir_entries_in_dir(image, dir_clust_num, entries, &dir_entry_count);

  // handle directory-not-empty
  if (dir_entry_count > 2) {
    sprintf(error_msg, "directory \"%s\" is not empty\n", args);
    return false;
  }

  // calculate dirs cluster index inside data region
  int bytes_per_sect =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);
  int cluster_pos =
      get_data_start_index(image) + (dir_clust_num - 2) * bytes_per_sect;
  int fatPos = get_FAT_start_index(image) + dir_clust_num * 4;

  // remove "." and ".." from data region
  for (i = 0; i < bytes_per_sect; i++)
    image->buffer[cluster_pos + i] = 0x00;

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
    int cluster_pos = get_data_start_index(image) +
                      (clusters[index] - 2) * bytes_per_sect +
                      (dir_index - 1) * 64;
    for (i = 0; i < 64; i++)
      image->buffer[cluster_pos + i] = 0x00;
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
