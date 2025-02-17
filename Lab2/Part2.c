#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <fcntl.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <dirent.h>

#include <stdint.h>



#define TAILLE_SECTEUR 512



typedef struct {

    unsigned char code_boot[446];

    unsigned char table_partition[64];

    unsigned char signature[2];

} MBR;



typedef struct {

    unsigned char type_guid_partition[16];

    unsigned char guid_unique_partition[16];

    unsigned long long premier_lba;

    unsigned long long dernier_lba;

    unsigned long long attributs;

    unsigned char nom_partition[72];

} Entree_GPT;



typedef struct {

    unsigned char saut_boot[3];

    unsigned char nom_oem[8];

    unsigned short octets_par_secteur;

    unsigned char secteurs_par_cluster;

    unsigned short secteurs_reserves;

    unsigned char nombre_fats;

    unsigned short entree_racine_count;

    unsigned short total_secteurs_16;

    unsigned char media;

    unsigned short taille_fat_16;

    unsigned short secteurs_par_piste;

    unsigned short nombre_heads;

    unsigned int secteurs_caches;

    unsigned int total_secteurs_32;

    unsigned int taille_fat_32;

    unsigned short drapeaux;

    unsigned short version_fs;

    unsigned int cluster_racine;

    unsigned short info_fs;

    unsigned short secteur_backup_boot;

    unsigned char reserve[12];

    unsigned char numero_lecteur;

    unsigned char reserve1;

    unsigned char signature_boot;

    unsigned int id_volume;

    unsigned char etiquette_volume[11];

    unsigned char type_fs[8];

} __attribute__((packed)) BootSector_FAT32;



unsigned char* lire_secteur(const char* disque, int num_sect) {

    char chemin[20] = "/dev/";

    strcat(chemin, disque);



    FILE* disk = fopen(chemin, "rb");

    if (disk == NULL) {

        perror("Erreur d'ouverture du disque");

        return NULL;

    }



    unsigned char* buffer = malloc(TAILLE_SECTEUR);

    if (fseek(disk, num_sect * TAILLE_SECTEUR, SEEK_SET) != 0) {

        perror("Erreur de positionnement");

        fclose(disk);

        free(buffer);

        return NULL;

    }



    int n = fread(buffer, 1, TAILLE_SECTEUR, disk);

    if (n < TAILLE_SECTEUR) {

        perror("Erreur de lecture");

        fclose(disk);

        free(buffer);

        return NULL;

    }



    fclose(disk);

    return buffer;

}



void afficher_infos_fat32(const char* disque) {

    unsigned char* sector0 = lire_secteur(disque, 0);

    if (sector0 == NULL) {

        return;

    }



    MBR* mbr = (MBR*)sector0;

    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xAA) {

        printf("Signature MBR invalide\n");

        free(sector0);

        return;

    }



    unsigned char* gpt_header_sector = lire_secteur(disque, 1);

    if (gpt_header_sector == NULL) {

        free(sector0);

        return;

    }



    unsigned char* gpt_entries = lire_secteur(disque, 2);

    if (gpt_entries == NULL) {

        free(sector0);

        free(gpt_header_sector);

        return;

    }



    Entree_GPT* second_partition = (Entree_GPT*)(gpt_entries + sizeof(Entree_GPT));



    printf("LBA début de la partition FAT32: %llu\n", second_partition->premier_lba);

    printf("Taille en secteurs de la partition FAT32: %llu\n", second_partition->dernier_lba - second_partition->premier_lba + 1);



    unsigned char* boot_sector = lire_secteur(disque, second_partition->premier_lba);

    if (boot_sector == NULL) {

        free(sector0);

        free(gpt_header_sector);

        free(gpt_entries);

        return;

    }



    BootSector_FAT32* bs = (BootSector_FAT32*)boot_sector;



    uint32_t cluster_racine = bs->cluster_racine;

    uint32_t lba_root_dir = second_partition->premier_lba + bs->secteurs_reserves + (bs->nombre_fats * bs->taille_fat_32);

    uint32_t taille_cluster = bs->secteurs_par_cluster * TAILLE_SECTEUR;



    printf("Numéro du premier cluster du répertoire racine: %u\n", cluster_racine);

    printf("LBA début du répertoire racine: %u\n", lba_root_dir);

    printf("Taille d'un cluster en secteurs: %u\n", bs->secteurs_par_cluster);



    free(sector0);

    free(gpt_header_sector);

    free(gpt_entries);

    free(boot_sector);

}



void afficher_contenu_repertoire_racine(const char* disque, uint32_t lba_root_dir, uint32_t taille_cluster) {

    unsigned char* root_dir = lire_secteur(disque, lba_root_dir);

    if (root_dir == NULL) {

        return;

    }



    printf("\n\n\t\t******************* Contenu du répertoire racine ******************* \n");

    printf("\n|-----------|-----------------------|------------------|-----------------|---------------------|");

    printf("\n| Nom       |    Taille(secteurs)   |  Taille Ko       | Premier cluster |  LBA premier Cluster|");

    printf("\n|-----------|-----------------------|------------------|-----------------|---------------------|");



    for (int i = 0; i < taille_cluster; i += 32) {

        if (root_dir[i] == 0x00) break;  // End of directory



        if (root_dir[i + 11] != 0x0F) {

            char nom[12];

            memcpy(nom, root_dir + i, 11);

            nom[11] = '\0';



            uint32_t premier_cluster = *(uint16_t*)&root_dir[i + 20] << 16 | *(uint16_t*)&root_dir[i + 26];

            uint32_t taille_fichier = *(uint32_t*)&root_dir[i + 28];

            uint32_t taille_secteurs = (taille_fichier + TAILLE_SECTEUR - 1) / TAILLE_SECTEUR;

            uint32_t lba_premier_cluster = lba_root_dir + (premier_cluster - 2) * taille_cluster;



            printf("\n| %-10s | %21u | %16.2f | %15u | %19u |", nom, taille_secteurs, taille_secteurs / 2.0, premier_cluster, lba_premier_cluster);

        }

    }



    free(root_dir);

    printf("\n|-----------|-----------------------|------------------|-----------------|---------------------|\n");

}



void lister_disques() {

    struct dirent *de;

    DIR *dr = opendir("/dev");

    if (dr == NULL) {

        printf("Erreur d'ouverture du répertoire /dev\n");

        return;

    }



    printf("Disques disponibles:\n");

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

    lister_disques();



    const char* disque = "sdb2";  // Remplacer par l'identifiant réel du disque

    afficher_infos_fat32(disque);



    // Appel de la fonction afficher_contenu_repertoire_racine

    uint32_t lba_root_dir = 8284;  // Exemple, remplacer par la valeur réelle obtenue

    uint32_t taille_cluster = 4096;  // Exemple, remplacer par la valeur réelle obtenue

    afficher_contenu_repertoire_racine(disque, lba_root_dir, taille_cluster);



    return 0;

}
