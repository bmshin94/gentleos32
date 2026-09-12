/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: file.c - Operations on files
 */

#include <kernel.h>

extern file_st builtin_files[];
extern size_t builtin_files_count;

global const char *file_type_names[FILE_TYPE_COUNT] = {
    "unknown",
    "bitmap",
    "song",
};

global size_t
file_count(void)
{
    system_info_st *si = &krn_system_info;

    return builtin_files_count + si->initrd_files_count;
}

global file_st *
file_get(size_t index)
{
    system_info_st *si = &krn_system_info;

    if (index < si->initrd_files_count) {
        return &si->initrd_files[index];
    }

    index -= si->initrd_files_count;

    if (index < builtin_files_count) {
        return &builtin_files[index];
    }

    return NULL;
}

global file_st *
file_lookup(const char *name)
{
    size_t i;
    file_st *file;

    for (i = 0; i < file_count(); ++i) {
        file = file_get(i);

        if (strncmp(name, file->name, sizeof(file->name)) == 0) {
            return file;
        }
    }

    return NULL;
}

static void
file_init(file_st *file)
{
    if (file->type == FILE_TYPE_SONG) {
        song_init_notes(file);
    }
}

static void
file_init_group(const char *label, file_st *files, size_t count)
{
    size_t i;
    size_t total_size = 0;
    int show_kb;

    for (i = 0; i < count; ++i) {
        file_init(&files[i]);
        total_size += files[i].size;
    }

    show_kb = (total_size >> 10) > 4;

    krn_debug_printf("%s files: %u (%u %s)\n",
        label,
        count,
        show_kb ? (total_size >> 10) : total_size,
        show_kb ? "KB" : "B"
    );
}

global void
file_init_all(void)
{
    system_info_st *si = &krn_system_info;

    file_init_group("Initrd", si->initrd_files, si->initrd_files_count);
    file_init_group("Built-in", builtin_files, builtin_files_count);
}
