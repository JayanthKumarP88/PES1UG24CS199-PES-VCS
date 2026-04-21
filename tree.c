// tree.c — Tree object serialization and construction

#include "tree.h"
#include "index.h"
#include "pes.h" // Added to fix the object_write warning
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// ─── Mode Constants ─────────────────────────────────────────────────────────

#define MODE_FILE      0100644
#define MODE_EXEC      0100755
#define MODE_DIR       0040000

// ─── PROVIDED ───────────────────────────────────────────────────────────────

uint32_t get_file_mode(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;

    if (S_ISDIR(st.st_mode))  return MODE_DIR;
    if (st.st_mode & S_IXUSR) return MODE_EXEC;
    return MODE_FILE;
}

int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];

        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1;

        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);

        ptr = space + 1;

        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1;

        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0';

        ptr = null_byte + 1;

        if (ptr + HASH_SIZE > end) return -1;
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }
    return 0;
}

static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    size_t max_size = tree->count * 296;
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;

    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);

    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];

        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1;

        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─── IMPLEMENTED ─────────────────────────────────────────────────────────────

static int build_tree_recursive(IndexEntry *entries, int count, int depth, ObjectID *out_id) {
    Tree current_tree;
    current_tree.count = 0;
    
    int i = 0;
    while (i < count) {
        IndexEntry *entry = &entries[i];
        
        const char *comp_start = entry->path;
        for (int d = 0; d < depth; d++) {
            comp_start = strchr(comp_start, '/');
            if (comp_start) comp_start++;
            else break;
        }
        
        if (!comp_start) {
            i++;
            continue; 
        }
        
        const char *comp_end = strchr(comp_start, '/');
        TreeEntry *tree_entry = &current_tree.entries[current_tree.count];
        
        if (!comp_end) {
            // Using snprintf instead of strncpy to silence GCC warnings
            snprintf(tree_entry->name, sizeof(tree_entry->name), "%s", comp_start);
            tree_entry->mode = entry->mode;
            tree_entry->hash = entry->hash;
            current_tree.count++;
            i++;
        } else {
            size_t dir_len = comp_end - comp_start;
            char dir_name[256];
            snprintf(dir_name, sizeof(dir_name), "%.*s", (int)dir_len, comp_start);
            
            int group_count = 0;
            for (int j = i; j < count; j++) {
                const char *j_comp = entries[j].path;
                for (int d = 0; d < depth; d++) {
                    j_comp = strchr(j_comp, '/');
                    if (j_comp) j_comp++;
                }
                if (j_comp && strncmp(j_comp, dir_name, dir_len) == 0 && j_comp[dir_len] == '/') {
                    group_count++;
                } else {
                    break;
                }
            }
            
            ObjectID sub_tree_id;
            if (build_tree_recursive(&entries[i], group_count, depth + 1, &sub_tree_id) < 0) {
                return -1;
            }
            
            snprintf(tree_entry->name, sizeof(tree_entry->name), "%s", dir_name);
            tree_entry->mode = MODE_DIR;
            tree_entry->hash = sub_tree_id;
            current_tree.count++;
            
            i += group_count;
        }
    }
    
    void *tree_data = NULL;
    size_t tree_len = 0;
    if (tree_serialize(&current_tree, &tree_data, &tree_len) < 0) return -1;
    
    int result = object_write(OBJ_TREE, tree_data, tree_len, out_id);
    free(tree_data);
    
    return result;
}

int tree_from_index(ObjectID *id_out) {
    Index idx;
    if (index_load(&idx) < 0) return -1;
    
    if (idx.count == 0) {
        Tree empty_tree;
        empty_tree.count = 0;
        void *data;
        size_t len;
        if (tree_serialize(&empty_tree, &data, &len) == 0) {
            int ret = object_write(OBJ_TREE, data, len, id_out);
            free(data);
            return ret;
        }
        return -1;
    }
    
    return build_tree_recursive(idx.entries, idx.count, 0, id_out);
}
 
 
