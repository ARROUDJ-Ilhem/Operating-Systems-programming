#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <fcntl.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <dirent.h>



#define SECTOR_SIZE 512



typedef struct {

    unsigned char boot_code[446];

    unsigned char partition_table[64];

    unsigned char signature[2];

} MBR;



typedef struct {

    unsigned char partition_type_guid[16];

    unsigned char unique_partition_guid[16];

    unsigned long long first_lba;

    unsigned long long last_lba;

    unsigned long long attributes;

    unsigned char partition_name[72];

} GPT_Entry;



typedef struct {

    unsigned char jmp_boot[3];

    unsigned char oem_name[8];

    unsigned short bytes_per_sector;

    unsigned char sectors_per_cluster;

    unsigned short reserved_sector_count;

    unsigned char num_fats;

    unsigned short root_entry_count;

    unsigned short total_sectors_16;

    unsigned char media;

    unsigned short fat_size_16;

    unsigned short sectors_per_track;

    unsigned short num_heads;

    unsigned int hidden_sectors;

    unsigned int total_sectors_32;

    unsigned int fat_size_32;

    unsigned short ext_flags;

    unsigned short fs_version;

    unsigned int root_cluster;

    unsigned short fs_info;

    unsigned short backup_boot_sector;

    unsigned char reserved[12];

    unsigned char drive_number;

    unsigned char reserved1;

    unsigned char boot_signature;

    unsigned int volume_id;

    unsigned char volume_label[11];

    unsigned char fs_type[8];

} __attribute__((packed)) FAT32_BootSector;



unsigned char* read_sector(const char* disque, int num_sect) {

    char chemin[20] = "/dev/";

    strcat(chemin, disque);

    FILE* disk = fopen(chemin, "rb");

    if (disk == NULL) {

        perror("Erreur d'ouverture du disque");

        return NULL;

    }



    unsigned char* buffer = malloc(SECTOR_SIZE);

    if (fseek(disk, num_sect * SECTOR_SIZE, SEEK_SET) != 0) {

        perror("Erreur de positionnement");

        fclose(disk);

        free(buffer);

        return NULL;

    }



    int n = fread(buffer, 1, SECTOR_SIZE, disk);

    if (n < SECTOR_SIZE) {

        perror("Erreur de lecture");

        fclose(disk);

        free(buffer);

        return NULL;

    }



    fclose(disk);

    return buffer;

}



void afficher_infos_fat32(const char* disque) {

    unsigned char* sector0 = read_sector(disque, 0);

    if (sector0 == NULL) {

        return;

    }



    // MBR parsing to identify GPT

    MBR* mbr = (MBR*)sector0;

    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xAA) {

        printf("Signature MBR invalide\n");

        free(sector0);

        return;

    }



    // Read GPT header

    unsigned char* gpt_header_sector = read_sector(disque, 1);

    if (gpt_header_sector == NULL) {

        free(sector0);

        return;

    }



    // Read GPT entries (assuming the first entry sector is 2)

    unsigned char* gpt_entries = read_sector(disque, 2);

    if (gpt_entries == NULL) {

        free(sector0);

        free(gpt_header_sector);

        return;

    }



    // Read second GPT entry

    GPT_Entry* second_partition = (GPT_Entry*)(gpt_entries + sizeof(GPT_Entry));



    printf("LBA début de la partition FAT32: %llu\n", second_partition->first_lba);

    printf("Taille en secteurs de la partition FAT32: %llu\n", second_partition->last_lba - second_partition->first_lba + 1);



    // Read the boot sector of the FAT32 partition

    unsigned char* boot_sector = read_sector(disque, second_partition->first_lba);

    if (boot_sector == NULL) {

        free(sector0);

        free(gpt_header_sector);

        free(gpt_entries);

        return;

    }



    FAT32_BootSector* bs = (FAT32_BootSector*)boot_sector;



    printf("Numéro du premier cluster du répertoire racine: %u\n", bs->root_cluster);

    printf("LBA début du répertoire racine: %llu\n", second_partition->first_lba + bs->reserved_sector_count + (bs->num_fats * bs->fat_size_32));

    printf("Taille d'un cluster en secteurs: %u\n", bs->sectors_per_cluster);



    free(sector0);

    free(gpt_header_sector);

    free(gpt_entries);

    free(boot_sector);

}



void list() {

    struct dirent *de;

    DIR *dr = opendir("/dev");

    if (dr == NULL) {

        printf("can't open the disk");

        return;

    }



    while ((de = readdir(dr)) != NULL) {

        if ((strncmp(de->d_name, "sd", 2) == 0) || (strncmp(de->d_name, "hd", 2) == 0)) {

            if (strlen(de->d_name) == 3) {

                printf("%s\n", de->d_name);

            }

        }

    }

    closedir(dr);

}



int main() {

    list();  // list available disks

    afficher_infos_fat32("sda");  // replace "sda" with the actual disk identifier

    return 0;

}
