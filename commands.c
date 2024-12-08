#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
#include "fat32.h"  // Alterado para incluir o cabeçalho FAT32
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>

/*
 * Função de busca na pasta raíz.
 */
struct far_dir_searchres find_in_root(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = { .found = false };

    for (size_t i = 0; i < bpb->possible_rentries; i++)
    {
        if (dirs[i].name[0] == '\0') continue;

        if (memcmp((char *) dirs[i].name, filename, FAT32STR_SIZE) == 0)
        {
            res.found = true;
            res.fdir  = dirs[i];
            res.idx   = i;
            break;
        }
    }

    return res;
}

/*
 * Função de ls
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    struct fat_dir *dirs = malloc(sizeof(struct fat_dir) * bpb->possible_rentries);

    for (int i = 0; i < bpb->possible_rentries; i++)
    {
        uint32_t offset = bpb_froot_addr(bpb) + i * sizeof(struct fat_dir);
        read_bytes(fp, offset, &dirs[i], sizeof(dirs[i]));
    }

    return dirs;
}

void mv(FILE *fp, char *source, char* dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    bool badname = cstr_to_fat32wnull(source, source_rname)
                || cstr_to_fat32wnull(dest,   dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_froot_addr(bpb);
    uint32_t root_size    = sizeof(struct fat_dir) * bpb->possible_rentries;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler struct fat_dir");

    struct far_dir_searchres dir1 = find_in_root(&root[0], source_rname, bpb);
    struct far_dir_searchres dir2 = find_in_root(&root[0], dest_rname,   bpb);

    if (dir2.found == true)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via mv.", dest);

    if (dir1.found == false)
        error(EXIT_FAILURE, 0, "Não foi possivel encontrar o arquivo %s.", source);

    memcpy(dir1.fdir.name, dest_rname, sizeof(char) * FAT32STR_SIZE);

    uint32_t source_address = sizeof(struct fat_dir) * dir1.idx + root_address;

    (void) fseek(fp, source_address, SEEK_SET);
    (void) fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);

    printf("mv %s → %s.\n", source, dest);
}

void rm(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    char fat32_rname[FAT32STR_SIZE_WNULL];

    if (cstr_to_fat32wnull(filename, fat32_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_froot_addr(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->possible_rentries;

    struct fat_dir root[root_size];
    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
    {
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler struct fat_dir");
    }

    struct far_dir_searchres dir = find_in_root(&root[0], fat32_rname, bpb);

    if (dir.found == false)
    {
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", filename);
    }

    dir.fdir.name[0] = DIR_FREE_ENTRY;

    uint32_t file_address = sizeof(struct fat_dir) * dir.idx + root_address;

    (void) fseek(fp, file_address, SEEK_SET);
    (void) fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

    uint32_t fat_address = bpb_faddress(bpb);
    uint16_t cluster_number = dir.fdir.starting_cluster;
    uint16_t null = 0x0;
    size_t count = 0;

    while (cluster_number < FAT32_EOF)
    {
        uint32_t infat_cluster_address = fat_address + cluster_number * sizeof(uint32_t);
        read_bytes(fp, infat_cluster_address, &cluster_number, sizeof(uint32_t));

        (void) fseek(fp, infat_cluster_address, SEEK_SET);
        (void) fwrite(&null, sizeof(uint32_t), 1, fp);

        count++;
    }

    printf("rm %s, %li clusters apagados.\n", filename, count);
}

struct fat32_newcluster_info fat32_find_free_cluster(FILE* fp, struct fat_bpb* bpb)
{
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t total_clusters = bpb_fdata_cluster_count(bpb);

    for (uint32_t cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t entry;
        uint32_t entry_address = fat_address + cluster * sizeof(uint32_t);

        (void) read_bytes(fp, entry_address, &entry, sizeof(uint32_t));

        if (entry == 0x0)
            return (struct fat32_newcluster_info) { .cluster = cluster, .address = entry_address };
    }

    return (struct fat32_newcluster_info) {0};
}

void cp(FILE *fp, char* source, char* dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    bool badname = cstr_to_fat32wnull(source, source_rname)
                || cstr_to_fat32wnull(dest,   dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_froot_addr(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->possible_rentries;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler struct fat_dir");

    struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);

    if (!dir1.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", source);

    if (find_in_root(root, dest_rname, bpb).found)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via cp.", dest);

    struct fat_dir new_dir = dir1.fdir;
    memcpy(new_dir.name, dest_rname, FAT32STR_SIZE);

    bool dentry_failure = true;

    for (int i = 0; i < bpb->possible_rentries; i++) if (root[i].name[0] == DIR_FREE_ENTRY || root[i].name[0] == '\0')
    {
        uint32_t dest_address = sizeof(struct fat_dir) * i + root_address;

        (void) fseek(fp, dest_address, SEEK_SET);
                (void) fwrite(&new_dir, sizeof(struct fat_dir), 1, fp);

        dentry_failure = false;

        break;
    }

    if (dentry_failure)
        error_at_line(EXIT_FAILURE, ENOSPC, __FILE__, __LINE__, "Não foi possivel alocar uma entrada no diretório raiz.");

    // Allocate clusters for the new file
    int count = 0;
    struct fat32_newcluster_info next_cluster, prev_cluster = { .cluster = FAT32_EOF_HI };
    uint32_t cluster_count = dir1.fdir.file_size / bpb->bytes_p_sect / bpb->sector_p_clust + 1;

    while (cluster_count--)
    {
        prev_cluster = next_cluster;
        next_cluster = fat32_find_free_cluster(fp, bpb);

        if (next_cluster.cluster == 0x0)
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Disco cheio (imagem foi corrompida)");

        (void) fseek(fp, next_cluster.address, SEEK_SET);
        (void) fwrite(&prev_cluster.cluster, sizeof(uint32_t), 1, fp);

        count++;
    }

    new_dir.starting_cluster = next_cluster.cluster;

    // Copy file data
    const uint32_t fat_address = bpb_faddress(bpb);
    const uint32_t data_region_start = bpb_fdata_addr(bpb);
    const uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

    size_t bytes_to_copy = new_dir.file_size;
    uint32_t source_cluster_number = dir1.fdir.starting_cluster;
    uint32_t destin_cluster_number = new_dir.starting_cluster;

    while (bytes_to_copy != 0)
    {
        uint32_t source_cluster_address = (source_cluster_number - 2) * cluster_width + data_region_start;
        uint32_t destin_cluster_address = (destin_cluster_number - 2) * cluster_width + data_region_start;

        size_t copied_in_this_sector = MIN(bytes_to_copy, cluster_width);
        char filedata[cluster_width];

        (void) read_bytes(fp, source_cluster_address, filedata, copied_in_this_sector);
        (void) fseek(fp, destin_cluster_address, SEEK_SET);
        (void) fwrite(filedata, sizeof(char), copied_in_this_sector, fp);

        bytes_to_copy -= copied_in_this_sector;

        uint32_t source_next_cluster_address = fat_address + source_cluster_number * sizeof(uint32_t);
        uint32_t destin_next_cluster_address = fat_address + destin_cluster_number * sizeof(uint32_t);

        (void) read_bytes(fp, source_next_cluster_address, &source_cluster_number, sizeof(uint32_t));
        (void) read_bytes(fp, destin_next_cluster_address, &destin_cluster_number, sizeof(uint32_t));
    }

    printf("cp %s → %s, %i clusters copiados.\n", source, dest, count);
}
