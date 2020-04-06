#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HEADER_LUMPS 64
#define LUMP_ENTITIES 0
#define TMP_OUT "tmp.bsp"

#ifdef __linux__
#define BZIP2_BIN "bzip2"
#else
#define BZIP2_BIN "bzip2.exe"
#endif

#ifdef MSVC
#pragma pack(0)
#endif
struct __attribute__((packed)) lump_t
{
    uint32_t fileofs;
    uint32_t filelen;
    uint32_t version;
    uint8_t cc[4];
};

#ifdef MSVC
#pragma pack(0)
#endif
struct __attribute__((packed)) bsp_hdr_t
{
    uint32_t ident;
    uint32_t version;
    lump_t lumps[HEADER_LUMPS];
    uint32_t map_revision;
};

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <in_file>\n", argv[0]);
        return 1;
    }

    FILE* in_fp = fopen(argv[1], "rb");

    if(!in_fp)
    {
        fprintf(stderr, "could not open %s\n", argv[1]);
        return 1;
    }

    fseek(in_fp, 0, SEEK_END);
    size_t sz = ftell(in_fp);
    fseek(in_fp, 0, SEEK_SET);

    uint8_t* map = new uint8_t[sz];

    if(!map)
    {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    printf("read %ld/%ld bytes <-- %s\n", fread(map, sizeof(uint8_t), sz, in_fp), sz, argv[1]);
    fclose(in_fp);

    bsp_hdr_t* hdr = (bsp_hdr_t*)map;

    printf("ident: %08X, version: %08X\n", hdr->ident, hdr->version);
    printf("entity lump: off: %08X, len: %08X\n", hdr->lumps[LUMP_ENTITIES].fileofs,
        hdr->lumps[LUMP_ENTITIES].filelen
    );

    uint8_t* entity_lump = &map[hdr->lumps[LUMP_ENTITIES].fileofs];
    size_t entity_lump_len = hdr->lumps[LUMP_ENTITIES].filelen;

    if(entity_lump + entity_lump_len > map + sz)
    {
        fprintf(stderr, "entity lump overflow abort...\n");
        return 1;
    }

    printf("nulling entity lump...\n");
    for(size_t i = 0; i < entity_lump_len; i++)
        entity_lump[i] = 0;

    FILE* out_fp = fopen(TMP_OUT, "wb");

    if(!out_fp)
    {
        fprintf(stderr, "could not open %s\n", TMP_OUT);
        return 1;
    }

    printf("writing %ld/%ld bytes --> %s\n", fwrite(map, sizeof(uint8_t), sz, out_fp), sz, TMP_OUT);
    fclose(out_fp);

    char in_name[256];
    char cmd[256];
    snprintf(in_name, sizeof(in_name), "%s1", argv[1]);
    rename(argv[1], in_name);
    rename(TMP_OUT, argv[1]);
    snprintf(cmd, sizeof(cmd), BZIP2_BIN " -z \"%s\"", argv[1]);
    system(cmd);
    rename(in_name, argv[1]);
    printf("%s.bz2 goes to fastdl/game/maps/%s.bz2\n", argv[1], argv[1]);
    printf("%s goes to server/game/maps/%s\n", argv[1], argv[1]);
    return 0;
}
