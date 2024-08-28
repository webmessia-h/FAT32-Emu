#include "../include/fat32.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
// function for getting the index of the beginning of the FAT Region
// simply return the size of the reserved region
int get_FAT_start_index(const image *image) { return get_reserved_size(image); }

// function for getting the index of beginning of the DATA Region
// return the size of the reserved region + FAT region
int get_data_start_index(const image *image) {
  return get_reserved_size(image) + get_FAT_size(image);
}

// Initialize boot sector
bool init_boot_sector(image *image) {
  memcpy(image->boot_sector.jmp_boot, "\xEB\x58\x90", jmp_boot_size);
  memcpy(image->boot_sector.OEM_name, "FAT32emu", oem_name_size);
  uint16_t bytes_per_sector = 512;
  uint8_t sectors_per_cluster = 8;
  uint16_t reserved_sectors = 32;
  uint8_t number_of_fats = 2;
  *(uint16_t *)image->boot_sector.byts_per_sec = bytes_per_sector;
  image->boot_sector.sec_per_clus[0] = 0x08; // 8 sectors per cluster
  *(uint16_t *)image->boot_sector.rsvd_sec_cnt = reserved_sectors;
  image->boot_sector.num_fats[0] = number_of_fats;
  image->boot_sector.root_ent_cnt[0] = 0x00;
  image->boot_sector.root_ent_cnt[1] = 0x00; // 0 root directory entries (FAT32)
  image->boot_sector.tot_sec_16[0] = 0x00;
  image->boot_sector.tot_sec_16[1] = 0x00; // 0 total sectors (16-bit)
  image->boot_sector.media[0] = 0xF8;      // media descriptor
  image->boot_sector.fat_sz_16[0] = 0x00;
  image->boot_sector.fat_sz_16[1] = 0x00; // 0 sectors per FAT (16-bit)
  image->boot_sector.sec_per_trk[0] = 0x3F;
  image->boot_sector.sec_per_trk[1] = 0x00; // 63 sectors per track
  image->boot_sector.num_heads[0] = 0xFF;
  image->boot_sector.num_heads[1] = 0x00; // 255 number of heads
  image->boot_sector.hidd_sec[0] = 0x00;
  image->boot_sector.hidd_sec[1] = 0x00;
  image->boot_sector.hidd_sec[2] = 0x00;
  image->boot_sector.hidd_sec[3] = 0x00; // 0 hidden sectors
  *(uint32_t *)image->boot_sector.tot_sec_32 = image->size / bytes_per_sector;
  size_t fat_size = get_FAT_size(image);
  *(uint32_t *)image->boot_sector.fat_sz_32 =
      fat_size / bytes_per_sector / number_of_fats;
  image->boot_sector.ext_flags[0] = 0x00;
  image->boot_sector.ext_flags[1] = 0x00; // No mirroring, active FAT is 0
  image->boot_sector.fs_ver[0] = 0x00;
  image->boot_sector.fs_ver[1] = 0x00;     // File system version 0.0
  image->boot_sector.root_clust[0] = 0x02; //
  image->boot_sector.root_clust[1] = 0x00; // Root directory cluster 2
  image->boot_sector.root_clust[2] = 0x00; //
  image->boot_sector.root_clust[3] = 0x00; //
  image->boot_sector.fs_info[0] = 0x01;
  image->boot_sector.fs_info[1] = 0x00; // FS info sector 1
  image->boot_sector.bk_boot_sec[0] = 0x06;
  image->boot_sector.bk_boot_sec[1] = 0x00; // Backup boot sector 6
  for (int i = 0; i < reserved_size; i++) {
    image->boot_sector.reserved[i] = 0x00; // reserved space, all 0s
  }
  image->boot_sector.drv_num[0] = 0x80;   // Drive number 0x80 (first hard disk)
  image->boot_sector.reservedl[0] = 0x00; // reserved, 0
  image->boot_sector.boot_sig[0] = 0x29;  // Extended boot signature 0x29
  image->boot_sector.vol_ID[0] = 0x12;
  image->boot_sector.vol_ID[1] = 0x34;
  image->boot_sector.vol_ID[2] = 0x56;
  image->boot_sector.vol_ID[3] = 0x78; // Volume ID 0x12345678
  memcpy(image->boot_sector.vol_lab, "NO NAME   ", vol_lab_size);
  image->boot_sector.vol_lab[vol_lab_size - 1] = '\0';
  memcpy(image->boot_sector.file_sys_type, "FAT32  ", fil_sys_type_size);
  image->boot_sector.vol_lab[fil_sys_type_size - 1] = '\0';
  return true;
}

bool setup_FAT_region(image *image) {
  if (dev)
    printf("[-] setup_FAT_region(image ptr: 0x%p)\n", image);

  int FAT_start = get_FAT_start_index(image);
  size_t total_FAT_size = get_FAT_size(image);
  int bytes_per_sector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);
  int number_of_fats =
      hex_str_to_dec(image->boot_sector.num_fats, num_fats_size);
  size_t FAT_size = total_FAT_size / number_of_fats;

  if (FAT_start + total_FAT_size > image->size) {
    fprintf(stderr, "Error: Calculated FAT size exceeds image size\n");
    return false;
  }

  // Set up the first FAT
  unsigned char media_descriptor = image->boot_sector.media[0];
  image->buffer[FAT_start] = media_descriptor;
  image->buffer[FAT_start + 1] = 0xFF;
  image->buffer[FAT_start + 2] = 0xFF;
  image->buffer[FAT_start + 3] = 0x0F;

  if (dev)
    printf("[+] Set up the first FAT\n");

  // Mark root directory cluster as end-of-chain
  int root_cluster =
      hex_str_to_dec(image->boot_sector.root_clust, root_clust_size);
  int root_cluster_entry = FAT_start + (root_cluster * 4);
  image->buffer[root_cluster_entry] = 0xFF;
  image->buffer[root_cluster_entry + 1] = 0xFF;
  image->buffer[root_cluster_entry + 2] = 0xFF;
  image->buffer[root_cluster_entry + 3] = 0x0F;

  if (dev) {
    printf("[+] Mark root directory cluster as EOC\n");
    printf("[-] Buffer size: %zu\n", image->size);
    printf("[-] FAT start: %d\n", FAT_start);
    printf("[-] FAT size: %zu\n", FAT_size);
    printf("[-] FAT sectors: %zu\n", FAT_size / bytes_per_sector);
  }

  // Copy first FAT to second FAT (if applicable)
  if (number_of_fats > 1) {
    memcpy(image->buffer + FAT_start + FAT_size, image->buffer + FAT_start,
           FAT_size);
    if (dev)
      printf("[+] Copy first FAT to second FAT\n");
  }
  return true;
}

bool setup_root_directory(image *image) {
  int root_cluster =
      hex_str_to_dec(image->boot_sector.root_clust, root_clust_size);
  int cluster_size = get_cluster_size(image);
  int data_start = get_data_start_index(image);
  int root_dir_start = data_start + ((root_cluster - 2) * cluster_size);

  // Create volume label entry if needed
  if (image->boot_sector.vol_lab[0] != ' ') {
    dir_entry vol_label =
        create_dir_entry((char *)image->boot_sector.vol_lab, 0x08, 0);
    memcpy(image->buffer + root_dir_start, &vol_label, sizeof(dir_entry));
  }
  if (dev)
    printf("[+] Create volume label entry\n");

  // Mark root directory cluster as end-of-chain in FAT
  set_FAT_table_value(image, root_cluster,
                      (unsigned char[]){0xFF, 0xFF, 0xFF, 0x0F});
  if (dev)
    printf("[+] Mark root directory cluster as end-of-chain in FAT\n");
  return true;
}

// get total clusters in image file
int get_total_clusters(const image *image) {
  if (dev)
    printf("[-] get_total_clusters(image pointer: 0x%p)\n", image);
  return get_FAT_size(image) / 4;
}

// return the number of bytes in a cluster
int get_cluster_size(const image *image) {
  unsigned long bytes_per_sector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);
  unsigned long sectors_per_cluster =
      hex_str_to_dec(image->boot_sector.sec_per_clus, sec_per_clus_size);
  return bytes_per_sector * sectors_per_cluster;
}

// get the size of the reserved region by multiplying
// the bytes per sector by the number of reserved sectors
int get_reserved_size(const image *image) {
  if (dev)
    printf("[-] get_reserved_size(image pointer: 0x%p)\n", image);
  unsigned long bytes_per_sector =
      hex_str_to_dec(image->boot_sector.byts_per_sec, byts_per_sec_size);
  unsigned long reserved_sectors =
      hex_str_to_dec(image->boot_sector.rsvd_sec_cnt, rsvd_sec_cnt_size);
  if (dev)
    printf("[-] bytes per sector: %lu | reserved sector: %lu\n",
           bytes_per_sector, reserved_sectors);
  return bytes_per_sector * reserved_sectors;
}

size_t get_FAT_size(const image *img) {
  if (img == NULL) {
    if (dev)
      fprintf(stderr, "[-] get_FAT_size(image pointer: 0x%p)\n", img);
    return 0;
  }

  // Read values from boot sector
  uint32_t total_sectors =
      img->size /
      hex_str_to_dec(img->boot_sector.byts_per_sec, byts_per_sec_size);
  uint16_t bytes_per_sector =
      hex_str_to_dec(img->boot_sector.byts_per_sec, byts_per_sec_size);
  uint8_t sectors_per_cluster =
      hex_str_to_dec(img->boot_sector.sec_per_clus, sec_per_clus_size);
  uint16_t reserved_sectors =
      hex_str_to_dec(img->boot_sector.rsvd_sec_cnt, rsvd_sec_cnt_size);
  uint8_t number_of_fats =
      hex_str_to_dec(img->boot_sector.num_fats, num_fats_size);

  // Start with an estimate of 1% of total sectors for FAT
  uint32_t fat_size_sectors = total_sectors / 100;
  uint32_t data_sectors;
  uint32_t total_clusters;

  do {
    data_sectors =
        total_sectors - reserved_sectors - (number_of_fats * fat_size_sectors);
    total_clusters = data_sectors / sectors_per_cluster;
    uint32_t required_fat_entries =
        total_clusters + 2; // +2 for reserved clusters
    uint32_t required_fat_sectors =
        (required_fat_entries * 4 + bytes_per_sector - 1) / bytes_per_sector;

    if (required_fat_sectors <= fat_size_sectors) {
      fat_size_sectors = required_fat_sectors;
      break;
    }

    fat_size_sectors = required_fat_sectors;
  } while (1);

  size_t total_fat_size = fat_size_sectors * bytes_per_sector * number_of_fats;

  if (NULL) {
    printf("[-] Total sectors: %u\n", total_sectors);
    printf("[-] Data sectors: %u\n", data_sectors);
    printf("[-] Total clusters: %u\n", total_clusters);
    printf("[-] Calculated FAT size: %zu bytes (%u sectors per FAT)\n",
           total_fat_size, fat_size_sectors);
  }

  return total_fat_size;
}

// get the size of the Data Region by subtracting the total
// image size by the size of the reserved and FAT regions
int get_data_size(const image *image) {
  return image->size - get_reserved_size(image) - get_FAT_size(image);
}

// function for getting the bytes from a cluster
void get_cluster(const image *image, int cluster_num, unsigned char *cluster) {
  // print tracing message
  if (dev)
    printf("[-] get_cluster(image pointer: 0x%p, cluster number: %d, cluster "
           "array: %d)\n",
           image, cluster_num, *cluster);
  // error check cluster number
  cluster_num = (cluster_num < 2) ? 2 : cluster_num;
  cluster_num = (cluster_num > get_total_clusters(image) + 2)
                    ? get_total_clusters(image)
                    : cluster_num;
  /* error check cluster number
  if (*cluster < 2 || *cluster > get_total_clusters(image) + 2) {
    printf("[!] Invalid cluster number: %d\n", *cluster);
    exit(1);
  }*/
  // read in cluster
  int start_index =
      get_data_start_index(image) + (cluster_num - 2) * get_cluster_size(image);
  int i;
  for (i = 0; i < get_cluster_size(image); i++)
    cluster[i] = image->buffer[start_index + i];
}

// search the fat region for a free cluster
int find_avail_cluster(const image *image) {
  // print tracing message
  if (dev)
    printf("[-] find_avail_cluster(image pointer: 0x%p)\n", image);
  // variables for reading in FAT Region
  int FAT_region[get_total_clusters(image)]; // 4 bytes = cluster in FAT region
  get_FAT_region(image, FAT_region);         // stores FAT region into int array
  // loop through and find free cluster (= 0)
  int i;
  int root = hex_str_to_dec(image->boot_sector.root_clust, root_clust_size);
  for (i = root; i < get_total_clusters(image); i++)
    if (FAT_region[i] == 0)
      return i;
  return -1;
}

// add new clusters to directories or files that take up
// more than a single cluster
int add_additional_cluster(const image *image, int first_cluster) {
  // print tracing message
  if (dev)
    printf("[-] add_additional_cluster(image pointer: 0x%p, first_cluster: "
           "%d)\n",
           image, first_cluster);

  // useful variables
  int FAT_start_index = get_FAT_start_index(image);
  int i, pos;

  // error check firs_cluster
  if (first_cluster <
      hex_str_to_dec(image->boot_sector.root_clust, root_clust_size))
    return -1;
  else if (first_cluster > get_total_clusters(image))
    return -1;

  // store all current clusters representing file/directory
  int clusters[1000];
  get_associated_clusters(image, first_cluster, clusters);

  // find available cluster
  int new_cluster = find_avail_cluster(image);
  if (new_cluster == -1) {
    printf("[!] No available clusters.");
    return -1;
  }
  // convert new cluster value to bytes
  unsigned char bytes[4];
  int_to_u_char(new_cluster, bytes);

  // find last cluster
  i = 0;
  while (clusters[i] != -1)
    i++;

  // set old end marker to be pointed at new cluster
  pos = clusters[i - 1] * 4 + FAT_start_index;

  // replace bytes
  for (i = 0; i < 4; i++) {
    image->buffer[pos + i] = bytes[i];
  }

  // change pos to represent new cluster
  pos = FAT_start_index + new_cluster * 4;

  // set to end marker
  for (i = 0; i < 4; i++)
    image->buffer[pos + i] = 0xFF;

  // update image file
  if (!update_image_file(image)) {
    printf("[!] image file might be corrupted");
    exit(1);
  }

  return new_cluster;
}

// set value in FAT Table
void set_FAT_table_value(const image *image, int index, unsigned char *value) {
  // print tracing message
  if (dev)
    printf("[-] set_FAT_table_value(image pointer: 0x%p, index:%d, %x%x%x%x)\n",
           image, index, value[0], value[1], value[2], value[3]);
  // get position in buffer to change FAT table
  int FAT_start = get_FAT_start_index(image);
  int buffer_pos = FAT_start + index * 4;
  // loop through and change FAT table values
  int i;
  for (i = 0; i < 4; i++)
    image->buffer[buffer_pos + i] = value[i];
}

// get array of associated characters
void get_associated_clusters(const image *image, int starting_cluster,
                             int *clusters) {
  // print tracing message
  if (dev)
    printf("[-] get_associated_clusters(image pointer: 0x%p, starting cluster: "
           "%d, clusters array: %d)\n",
           image, starting_cluster, *clusters);

  // error check cluster number
  starting_cluster = (starting_cluster < 2) ? 2 : starting_cluster;
  starting_cluster = (starting_cluster > get_total_clusters(image) + 2)
                         ? get_total_clusters(image)
                         : starting_cluster;

  // read the fat image into an array
  int FAT_region[get_total_clusters(image)]; // 4 bytes = cluster in FAT region
  get_FAT_region(image, FAT_region);         // stores FAT region into int array

  // traverse through clusters
  int i = 0;
  while (FAT_region[starting_cluster] < 0x0FFFF8 &&
         FAT_region[starting_cluster] != 0) { // while not at ending cluster
    clusters[i++] = starting_cluster;         // add current cluster to array
    starting_cluster = FAT_region[starting_cluster]; // change starting cluster
  }
  clusters[i++] = starting_cluster;

  // set end point (since we don't return a size)
  clusters[i] = -1;
}

// function for reading in the FAT region of the image file
// reading in clusters and store them in the FAT Region array
void get_FAT_region(const image *image, int *FAT_region) {
  // print tracing message
  if (dev)
    printf("[-] get_FAT_region(image pointer: 0x%p, int array:%d )\n", image,
           *FAT_region);
  // array for storing cluster value
  unsigned char temp[4];
  // loop through clusters, convert every 4 bytes to an int
  int i, j, count = 0;
  for (i = get_FAT_start_index(image); i < get_data_start_index(image);
       i = i + 4) {
    for (j = 0; j < 4; j++)
      temp[j] = image->buffer[i + j];
    FAT_region[count++] = hex_str_to_dec(temp, 4);
  }
}

// add a directory to a specific cluster
bool add_dir_entry_to_cluster(image *image, dir_entry d, int cluster) {
  // print tracing message
  if (dev)
    printf("[-] add_dir_entry_to_cluster(image pointer: 0x%p, dir entry %d, "
           "cluster: %d)\n",
           image, d, cluster);

  // initial variables
  int i, j;

  // error check cluster number
  cluster = (cluster < 2) ? 2 : cluster;
  if (cluster > get_total_clusters(image) + 2) {
    if (dev)
      printf("[!] cluster %d exceeds the total number of clusters (%d)\n",
             cluster, get_total_clusters(image) + 2);
    return false;
  }

  // get buffer position
  int buffer_pos =
      get_data_start_index(image) + (cluster - 2) * get_cluster_size(image);
  if (NULL) {
    printf("[-] buffer position: %d\n", buffer_pos);
    printf("[-] cluster size: %d\n", get_cluster_size(image));
    printf("[-] data start index: %d\n", get_data_start_index(image));
  }

  // search for empty spot
  int cluster_pos = -1;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 64; j++) {
      if (image->buffer[buffer_pos + j + (i * 64)] != 0x00 &&
          image->buffer[buffer_pos + j + (i * 64)] != EOF)
        break;
      else if (j == 63)
        cluster_pos = i;
    }
    if (cluster_pos != -1)
      break;
  }

  // error checking
  if (cluster_pos == -1) {
    if (dev)
      printf("[!] No empty space found in the cluster to add the directory "
             "entry.\n");
    return false;
  }
  // change buffer pos
  buffer_pos += (cluster_pos * 64) + 32;
  if (dev)
    printf("[-] Updated buffer position: %d\n", buffer_pos);
  // copy values into buffer
  for (i = 0; i < name_size; i++)
    image->buffer[buffer_pos++] = d.name[i];
  for (i = 0; i < attr_size; i++)
    image->buffer[buffer_pos++] = d.attr[i];
  for (i = 0; i < nt_res_size; i++)
    image->buffer[buffer_pos++] = d.nt_res[i];
  for (i = 0; i < crt_time_tenth_size; i++)
    image->buffer[buffer_pos++] = d.crt_time_tenth[i];
  for (i = 0; i < crt_time_size; i++)
    image->buffer[buffer_pos++] = d.crt_time[i];
  for (i = 0; i < crt_date_size; i++)
    image->buffer[buffer_pos++] = d.crt_date[i];
  for (i = 0; i < lst_acc_date_size; i++)
    image->buffer[buffer_pos++] = d.lst_acc_date[i];
  for (i = 0; i < fst_clust_hi_size; i++)
    image->buffer[buffer_pos++] = d.fst_clust_hi[i];
  for (i = 0; i < wrt_time_size; i++)
    image->buffer[buffer_pos++] = d.wrt_time[i];
  for (i = 0; i < wrt_date_size; i++)
    image->buffer[buffer_pos++] = d.wrt_date[i];
  for (i = 0; i < fst_clust_lo_size; i++)
    image->buffer[buffer_pos++] = d.fst_clust_lo[i];
  for (i = 0; i < file_size_size; i++)
    image->buffer[buffer_pos++] = d.file_size[i];

  return true;
}

// read directory entries in the current directory into entry array
void read_dir_entries_in_dir(const image *image, int dir_clust_num,
                             dir_entry entries[], int *size) {
  // print tracing message
  if (dev)
    printf(
        "[-] read_dir_entries_in_dir(image pointer, %d, dir entry array, %d)\n",
        dir_clust_num, *size);

  // error check dir_clust_num
  dir_clust_num = (dir_clust_num < 2) ? 2 : dir_clust_num;
  dir_clust_num = (dir_clust_num > get_total_clusters(image) + 2)
                      ? get_total_clusters(image)
                      : dir_clust_num;

  // read in clusters to search
  int clusters[150], i, j;
  get_associated_clusters(image, dir_clust_num, clusters);

  // loop through clusters
  i = 0;
  *size = 0;
  while (clusters[i] != -1) {
    // store the cluster
    unsigned char cluster[get_cluster_size(image)];
    get_cluster(image, clusters[i], cluster);
    // loop through each d in the cluster
    for (j = 1; j <= 8; j++) {
      dir_entry d = read_dir_entry(cluster, j);
      if (d.attr[0] != 0x00) // if attr is 0, then the dir is empty
        entries[(*size)++] = d;
    }
    i++;
  }
}

// create a dir entry off certain parameters
dir_entry create_dir_entry(const char *name, unsigned char attr,
                           unsigned int firs_cluster) {
  // print tracing message
  if (dev)
    printf("[-] create_dir_entry(%s, %x, %d)\n", name, attr, firs_cluster);

  // initial variables
  dir_entry d;
  int i;

  // set name
  int size = (strlen(name) < name_size) ? strlen(name) : name_size;
  for (i = 0; i < size; i++)
    d.name[i] = (unsigned char)name[i];
  for (i = size; i < name_size; i++)
    d.name[i] = 0x00;

  // set attr
  d.attr[0] = attr;

  // set NTRes
  d.nt_res[0] = 0x00;

  // set first cluster low and hi
  unsigned char bytes[4];
  int_to_u_char(firs_cluster, bytes);
  d.fst_clust_lo[0] = bytes[0];
  d.fst_clust_lo[1] = bytes[1];
  d.fst_clust_hi[0] = bytes[2];
  d.fst_clust_hi[1] = bytes[3];

  // set file size
  for (i = 0; i < file_size_size; i++)
    d.file_size[i] = 0x00;

  // ignore timestamps
  d.crt_time_tenth[0] = 0x00;
  d.crt_time[0] = 0x00;
  d.crt_time[1] = 0x00;
  d.crt_date[0] = 0x00;
  d.crt_date[1] = 0x00;
  d.lst_acc_date[0] = 0x00;
  d.lst_acc_date[1] = 0x00;
  d.wrt_time[0] = 0x00;
  d.wrt_date[1] = 0x00;

  // return
  return d;
}

// read in directory entry given 64 bit directory entry
// cluster = cluster of data from data region
// entry = which dir entry in the cluster 1-8 to read
dir_entry read_dir_entry(const unsigned char *cluster, int entry) {
  // print tracing message
  if (dev)
    printf("[-] readdir_entry(cluster array, %d)\n", entry);

  // initial variables
  dir_entry d;

  // skip first 32 bytes
  int start_index = (64 * --entry) + 32;

  // read in dir_entry values
  memcpy(d.name, cluster + start_index, name_size);
  start_index += name_size;
  memcpy(d.attr, cluster + start_index, attr_size);
  start_index += attr_size;
  memcpy(d.nt_res, cluster + start_index, nt_res_size);
  start_index += nt_res_size;
  memcpy(d.crt_time_tenth, cluster + start_index, crt_time_tenth_size);
  start_index += crt_time_tenth_size;
  memcpy(d.crt_time, cluster + start_index, crt_time_size);
  start_index += crt_time_size;
  memcpy(d.crt_date, cluster + start_index, crt_date_size);
  start_index += crt_date_size;
  memcpy(d.lst_acc_date, cluster + start_index, lst_acc_date_size);
  start_index += lst_acc_date_size;
  memcpy(d.fst_clust_hi, cluster + start_index, fst_clust_hi_size);
  start_index += fst_clust_hi_size;
  memcpy(d.wrt_time, cluster + start_index, wrt_time_size);
  start_index += wrt_time_size;
  memcpy(d.wrt_date, cluster + start_index, wrt_date_size);
  start_index += wrt_date_size;
  memcpy(d.fst_clust_lo, cluster + start_index, fst_clust_lo_size);
  start_index += fst_clust_lo_size;
  memcpy(d.file_size, cluster + start_index, file_size_size);
  start_index += file_size_size;

  // return directory entry after loading values
  return d;
}

// write boot sector into image buffer
bool write_boot_sector(image *image) {
  return memcpy(image->buffer, &image->boot_sector, sizeof(boot_sector));
}

// update the image file
// write buffer into image file
bool update_image_file(const image *image) {
  // print tracing message
  if (dev)
    printf("[-] update_image_file(image pointer: 0x%p)\n", image);
  // initial variables
  FILE *image_file;
  int bytes;
  // attempt to write file
  image_file = fopen(image->filename, "wb");
  if (image_file == NULL)
    fprintf(stderr,
            "[!] failed to open file for writing (pointer: 0x%p | filename: "
            "%s)\n",
            image_file, image->filename);

  bytes = fwrite(image->buffer, sizeof(unsigned char), image->size, image_file);
  fclose(image_file);
  // return if it was updated successfully
  return (bytes == image->size);
}

// read in image file and store it into an image struct
void read_in_image_file(const char *image_filename, image *image) {

  // initial variables
  FILE *image_file;
  int read;

  // open file
  image_file = fopen(image_filename, "rb");
  if (!image_file) { // check if file open correctly
    fprintf(stderr, "[!] ERROR: Unable to open image file\n");
    exit(1);
  }

  // get file size
  fseek(image_file, 0, SEEK_END);  // traverse file
  image->size = ftell(image_file); // get size of file
  rewind(image_file);              // rewind to file start

  // read contents into buffer
  image->buffer = (unsigned char *)calloc(image->size, sizeof(unsigned char));
  read = fread(image->buffer, sizeof(unsigned char), image->size, image_file);

  // confirm file was read correctly
  if (read != image->size) {
    fprintf(stderr, "[!] ERROR: Unable to read image file\n");
    exit(1);
  }

  // set open file count
  image->open_files_cnt = 0;

  // set current directory to root and set filename
  image->prompt_depth = 0;
  strcpy(image->filename, image_filename);
}

// read in the boot sector of the image file and save
// it into the Boot sector struct
boot_sector read_boot_sector(const char *image_file_name) {

  // initial variables
  boot_sector boot_sector;
  FILE *image_file;

  // open file
  image_file = fopen(image_file_name, "rb");
  if (!image_file) {
    fprintf(stderr, "[!] ERROR: Unable to open image file");
    exit(1);
  }

  // read each boot sector section into its variable using its
  // predefined size
  fread(boot_sector.jmp_boot, sizeof(unsigned char), jmp_boot_size, image_file);
  fread(boot_sector.OEM_name, sizeof(unsigned char), oem_name_size, image_file);
  fread(boot_sector.byts_per_sec, sizeof(unsigned char), byts_per_sec_size,
        image_file);
  fread(boot_sector.sec_per_clus, sizeof(unsigned char), sec_per_clus_size,
        image_file);
  fread(boot_sector.rsvd_sec_cnt, sizeof(unsigned char), rsvd_sec_cnt_size,
        image_file);
  fread(boot_sector.num_fats, sizeof(unsigned char), num_fats_size, image_file);
  fread(boot_sector.root_ent_cnt, sizeof(unsigned char), root_ent_cnt_size,
        image_file);
  fread(boot_sector.tot_sec_16, sizeof(unsigned char), tot_sec_16_size,
        image_file);
  fread(boot_sector.media, sizeof(unsigned char), media_size, image_file);
  fread(boot_sector.fat_sz_16, sizeof(unsigned char), fat_sz_16_size,
        image_file);
  fread(boot_sector.sec_per_trk, sizeof(unsigned char), sec_per_trk_size,
        image_file);
  fread(boot_sector.num_heads, sizeof(unsigned char), num_heads_size,
        image_file);
  fread(boot_sector.hidd_sec, sizeof(unsigned char), hidd_sec_size, image_file);
  fread(boot_sector.tot_sec_32, sizeof(unsigned char), tot_sec_32_size,
        image_file);
  fread(boot_sector.fat_sz_32, sizeof(unsigned char), fat_sz_32_size,
        image_file);
  fread(boot_sector.ext_flags, sizeof(unsigned char), ext_flags_size,
        image_file);
  fread(boot_sector.fs_ver, sizeof(unsigned char), fs_ver_size, image_file);
  fread(boot_sector.root_clust, sizeof(unsigned char), root_clust_size,
        image_file);
  fread(boot_sector.fs_info, sizeof(unsigned char), fs_info_size, image_file);
  fread(boot_sector.bk_boot_sec, sizeof(unsigned char), bk_boot_sec_size,
        image_file);
  fread(boot_sector.reserved, sizeof(unsigned char), reserved_size, image_file);
  fread(boot_sector.drv_num, sizeof(unsigned char), drv_num_size, image_file);
  fread(boot_sector.reservedl, sizeof(unsigned char), reservedl_size,
        image_file);
  fread(boot_sector.boot_sig, sizeof(unsigned char), boot_sig_size, image_file);
  fread(boot_sector.vol_ID, sizeof(unsigned char), vol_id_size, image_file);
  fread(boot_sector.vol_lab, sizeof(unsigned char), vol_lab_size, image_file);
  fread(boot_sector.file_sys_type, sizeof(unsigned char), fil_sys_type_size,
        image_file);

  return boot_sector;
}

// function for removing trailing whitespace
void remove_trailing_space(char *str) {
  // remove new_line
  char *new_line = strchr(str, '\n');
  if (new_line != NULL)
    *new_line = '\0';
  // trim trailing space
  char *end = str + strlen(str) - 1; // sent end to the end of the string
  while (end > str && isspace(*end))
    end--; /*
            * loop through string and if char is a space, then subtract the
            * pointer
            */
  // end is pointing at character right before ending
  // so, set character after it to be null character
  end[1] = '\0';
}

// function for splitting a string into an array of strings
void split(char *str, char *delim, int *count, char arr[100][100]) {
  // initial variables
  char temp[strlen(str) + 1];
  strcpy(temp, str);
  char *value = strtok(temp, delim);

  // loop through and split string
  *count = 0;
  while (value != NULL) {
    strcpy(arr[*count], value);
    value = strtok(NULL, delim);
    *count = *count + 1;
  }
}

void int_to_u_char(int n, unsigned char *bytes) {
  bytes[3] = (n >> 24) & 0xFF;
  bytes[2] = (n >> 16) & 0xFF;
  bytes[1] = (n >> 8) & 0xFF;
  bytes[0] = n & 0xFF;
}

// function for converting hex string (little endian) to ASCII
void hex_to_ASCII(const unsigned char *buffer, int buff_size, char *str) {
  int i, j;
  char k;
  for (i = 0; i < buff_size; i++) {
    j = (int)buffer[i];
    k = (char)j;
    str[i] = k;
  }
  str[buff_size] = '\0';
}

// function for converting hex string (little endian) to integer
int hex_str_to_dec(const unsigned char *buffer, int buff_size) {

  int i, decimal = 0;
  int values[buff_size];

  for (i = 0; i < buff_size; i++)
    values[i] = (int)buffer[i];
  for (i = 0; i < buff_size; i++)
    decimal += values[i] * power(256, i);

  return decimal;
}

int power(int base, int raise) {
  if (raise == 0)
    return 1;
  else
    return (base * power(base, raise - 1));
}

// function to compare two strings, used by qsort
int compare_dirs(const void *p1, const void *p2) {
  dir_entry *d1 = (dir_entry *)p1;
  dir_entry *d2 = (dir_entry *)p2;
  return strcmp(d1->name, d2->name);
}
