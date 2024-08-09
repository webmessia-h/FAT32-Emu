#ifndef FAT32_H
#define FAT32_H

// includes
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// logging
#define dev 0
// colorful output yippie
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

// global variables for boot sector
enum {
  drive_num = 0x80,
  boot_sig = 0x29,
  jmp_boot_size = 3,
  oem_name_size = 8,
  byts_per_sec_size = 2,
  sec_per_clus_size = 1,
  rsvd_sec_cnt_size = 2,
  num_fats_size = 1,
  root_ent_cnt_size = 2,
  tot_sec_16_size = 2,
  media_size = 1,
  fat_sz_16_size = 2,
  sec_per_trk_size = 2,
  num_heads_size = 2,
  hidd_sec_size = 4,
  tot_sec_32_size = 4,
  fat_sz_32_size = 4,
  ext_flags_size = 2,
  fs_ver_size = 2,
  root_clust_size = 4,
  fs_info_size = 2,
  bk_boot_sec_size = 2,
  reserved_size = 12,
  drv_num_size = 1,
  reservedl_size = 1,
  boot_sig_size = 1,
  vol_id_size = 4,
  vol_lab_size = 11,
  fil_sys_type_size = 8
};

// global variables for directory entry
enum {
  name_size = 11,
  attr_size = 1,
  nt_res_size = 1,
  crt_time_tenth_size = 1,
  crt_time_size = 2,
  crt_date_size = 2,
  lst_acc_date_size = 2,
  fst_clus_hi_size = 2,
  wrt_time_size = 2,
  wrt_date_size = 2,
  fst_clus_lo_size = 2,
  file_size_size = 4
};

// struct for dir entry
typedef struct {
  unsigned char name[name_size];
  unsigned char attr[attr_size];
  unsigned char nt_res[nt_res_size];
  unsigned char crt_time_tenth[crt_time_tenth_size];
  unsigned char crt_time[crt_time_size];
  unsigned char crt_date[crt_date_size];
  unsigned char lst_acc_date[lst_acc_date_size];
  unsigned char fst_clus_hi[fst_clus_hi_size];
  unsigned char wrt_time[wrt_time_size];
  unsigned char wrt_date[wrt_date_size];
  unsigned char fst_clus_lo[fst_clus_lo_size];
  unsigned char file_size[file_size_size];
} dir_entry;

// struct for boot sector
#pragma pack(push, 1)
typedef struct {
  unsigned char jmp_boot[jmp_boot_size];
  unsigned char OEM_name[oem_name_size];
  unsigned char byts_per_sec[byts_per_sec_size];
  unsigned char sec_per_clus[sec_per_clus_size];
  unsigned char rsvd_sec_cnt[rsvd_sec_cnt_size];
  unsigned char num_fats[num_fats_size];
  unsigned char root_ent_cnt[root_ent_cnt_size];
  unsigned char tot_sec_16[tot_sec_16_size];
  unsigned char media[media_size];
  unsigned char fat_sz_16[fat_sz_16_size];
  unsigned char sec_per_trk[sec_per_trk_size];
  unsigned char num_heads[num_heads_size];
  unsigned char hidd_sec[hidd_sec_size];
  unsigned char tot_sec_32[tot_sec_32_size];
  unsigned char fat_sz_32[fat_sz_32_size];
  unsigned char ext_flags[ext_flags_size];
  unsigned char fs_ver[fs_ver_size];
  unsigned char root_clust[root_clust_size];
  unsigned char fs_info[fs_info_size];
  unsigned char bk_boot_sec[bk_boot_sec_size];
  unsigned char reserved[reserved_size];
  unsigned char drv_num[drv_num_size];
  unsigned char reservedl[reservedl_size];
  unsigned char boot_sig[boot_sig_size];
  unsigned char vol_ID[vol_id_size];
  unsigned char vol_lab[vol_lab_size];
  unsigned char file_sys_type[fil_sys_type_size];
} boot_sector;
#pragma pack(pop)

// struct for currently open files
typedef struct {
  char filename[150];
  int first_cluster;
  int mode; // 0 = r, 1 = w, 2 = rw, 3 = wr
} open_file;

// struct for image file
typedef struct image {
  open_file open_files[150];
  int open_files_cnt;
  boot_sector boot_sector;
  unsigned char *buffer;
  unsigned long size;
  int current_cluster;
  char filename[150];
  char cwd[150][150];
  int prompt_depth;
} image;

// main functions
void print_prompt(const image *image);
void parse_input(char *line, image *image);

// custom functions
int get_cluster_size(const image *image);
int get_reserved_size(const image *image);
size_t get_FAT_size(const image *image);
int get_data_size(const image *image);
int get_FAT_start_index(const image *image);
int get_data_start_index(const image *image);

// FAT Region
int get_total_clusters(const image *image);
int find_avail_cluster(const image *image);
int add_additional_cluster(const image *image, int first_cluster);
void set_FAT_table_value(const image *image, int index, unsigned char *value);
void get_associated_clusters(const image *image, int starting_luster,
                             int *clusters);
void get_FAT_region(const image *image, int *FAT_region);

// Data Region
bool add_dir_entry_to_cluster(image *image, dir_entry d, int cluster);
void get_cluster(const image *image, int cluster_number,
                 unsigned char *cluster);
void read_dir_entries_in_dir(const image *image, int dir_clust_num,
                             dir_entry entries[], int *size);
dir_entry read_dir_entry(const unsigned char *cluster, int entry);

// Creation
bool init_boot_sector(image *image);
bool write_boot_sector(image *image);
bool setup_FAT_region(image *image);
bool setup_root_directory(image *image);
dir_entry create_dir_entry(const char *name, unsigned char attr,
                           unsigned int first_cluster);
// General
bool update_image_file(const image *image);
void read_in_image_file(const char *image_file_name, image *image);
boot_sector read_boot_sector(const char *image_file_name);

// Basic functions
void exit_cmd(image *image);
void info_cmd(const image *image);
bool format_cmd(char args[], image *image, char error_msg[]);
bool ls_cmd(char args[], const image *image, char error_msg[]);
bool cd_cmd(char args[], image *image, char error_msg[]);
size_t size_cmd(char args[], const image *image, char error_msg[]);
bool touch_cmd(char args[], image *image, char error_msg[]);
bool mkdir_cmd(char args[], image *image, char error_msg[]);
bool open_cmd(char args[], image *image, char error_msg[]);
bool close_cmd(char args[], image *image, char error_msg[]);
bool read_cmd(char args[], image *image, char error_msg[]);
bool write_cmd(char args[], image *image, char error_msg[]);
bool rm_cmd(char args[], image *image, char error_msg[]);
bool rmdir_cmd(char args[], image *image, char error_msg[]);

// helper functions
void remove_trailing_space(char *str);
void split(char *str, char *delimiter, int *count, char arr[100][100]);
void int_to_u_char(int n, unsigned char bytes[]);
void hex_to_ASCII(const unsigned char *buffer, int buffer_size, char *str);
int hex_str_to_dec(const unsigned char *buffer, int buffer_size);
int power(int base, int raise);
int compare_dirs(const void *p1, const void *p2);

#endif
