// object.c — Content-addressable object store
//
// Every piece of data (file contents, directory listings, commits) is stored
// as an "object" named by its SHA-256 hash. Objects are stored under
// .pes/objects/XX/YYYYYY... where XX is the first two hex characters of the
// hash (directory sharding).

#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/evp.h>

// ─── PROVIDED ────────────────────────────────────────────────────────────────

void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sprintf(hex_out + i * 2, "%02x", id->hash[i]);
    }
    hex_out[HASH_HEX_SIZE] = '\0';
}

int hex_to_hash(const char *hex, ObjectID *id_out) {
    if (strlen(hex) < HASH_HEX_SIZE) return -1;
    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + i * 2, "%2x", &byte) != 1) return -1;
        id_out->hash[i] = (uint8_t)byte;
    }
    return 0;
}

void compute_hash(const void *data, size_t len, ObjectID *id_out) {
    unsigned int hash_len;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, id_out->hash, &hash_len);
    EVP_MD_CTX_free(ctx);
}

// Get the filesystem path where an object should be stored.
void object_path(const ObjectID *id, char *path_out, size_t path_size) {
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    snprintf(path_out, path_size, "%s/%.2s/%s", OBJECTS_DIR, hex, hex + 2);
}

int object_exists(const ObjectID *id) {
    char path[512];
    object_path(id, path, sizeof(path));
    return access(path, F_OK) == 0;
}

// ─── IMPLEMENTED ─────────────────────────────────────────────────────────────

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
    const char *type_str;
    switch (type) {
        case OBJ_BLOB:   type_str = "blob";   break;
        case OBJ_TREE:   type_str = "tree";   break;
        case OBJ_COMMIT: type_str = "commit"; break;
        default: return -1;
    }

    // 1. Build the full object: header + data
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, len) + 1;
    
    size_t full_len = header_len + len;
    uint8_t *full_data = malloc(full_len);
    if (!full_data) return -1;

    memcpy(full_data, header, header_len);
    memcpy(full_data + header_len, data, len);

    // 2. Compute SHA-256 hash of the FULL object
    compute_hash(full_data, full_len, id_out);

    // 3. Deduplication check
    if (object_exists(id_out)) {
        free(full_data);
        return 0;
    }

    // Get the final path
    char final_path[512];
    object_path(id_out, final_path, sizeof(final_path));

    // 4. Extract shard directory and create it
    char shard_dir[512];
    strncpy(shard_dir, final_path, sizeof(shard_dir));
    char *last_slash = strrchr(shard_dir, '/');
    if (last_slash) *last_slash = '\0';
    
    mkdir(shard_dir, 0755); 

    // 5. Write to a temporary file
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s/.tmp_%d", shard_dir, getpid());
    
    int fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        free(full_data);
        return -1;
    }

    ssize_t written = write(fd, full_data, full_len);
    if (written != (ssize_t)full_len) {
        close(fd);
        unlink(tmp_path);
        free(full_data);
        return -1;
    }

    // 6. fsync to ensure data reaches disk before renaming
    fsync(fd);
    close(fd);

    // 7. Atomic rename
    if (rename(tmp_path, final_path) < 0) {
        unlink(tmp_path);
        free(full_data);
        return -1;
    }

    // 8. Open and fsync the shard directory
    int dir_fd = open(shard_dir, O_RDONLY);
    if (dir_fd >= 0) {
        fsync(dir_fd);
        close(dir_fd);
    }

    // 9. Cleanup
    free(full_data);
    return 0;
}

int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {
    // 1. Build the file path
    char path[512];
    object_path(id, path, sizeof(path));

    // 2. Open and read the entire file
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(f);
        return -1;
    }

    uint8_t *full_data = malloc(file_size);
    if (!full_data) {
        fclose(f);
        return -1;
    }

    if (fread(full_data, 1, file_size, f) != (size_t)file_size) {
        free(full_data);
        fclose(f);
        return -1;
    }
    fclose(f);

    // 3. Verify integrity
    ObjectID computed_id;
    compute_hash(full_data, file_size, &computed_id);
    if (memcmp(id->hash, computed_id.hash, HASH_SIZE) != 0) {
        free(full_data);
        return -1; 
    }

    // 4. Parse the header
    uint8_t *null_byte = memchr(full_data, '\0', file_size);
    if (!null_byte) {
        free(full_data);
        return -1;
    }

    char type_str[16];
    size_t parsed_len;
    if (sscanf((char *)full_data, "%15s %zu", type_str, &parsed_len) != 2) {
        free(full_data);
        return -1;
    }

    // 5. Set *type_out
    if (strcmp(type_str, "blob") == 0) *type_out = OBJ_BLOB;
    else if (strcmp(type_str, "tree") == 0) *type_out = OBJ_TREE;
    else if (strcmp(type_str, "commit") == 0) *type_out = OBJ_COMMIT;
    else {
        free(full_data);
        return -1;
    }

    size_t header_len = (null_byte - full_data) + 1; 
    
    if ((file_size - header_len) != parsed_len) {
        free(full_data);
        return -1;
    }

    // 6. Allocate buffer and copy the data portion
    *len_out = parsed_len;
    *data_out = malloc(parsed_len);
    if (!*data_out) {
        free(full_data);
        return -1;
    }

    memcpy(*data_out, full_data + header_len, parsed_len);
    free(full_data);

    return 0;
}
