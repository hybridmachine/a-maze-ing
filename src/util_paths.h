#pragma once

#include <stdbool.h>
#include <stddef.h>

bool util_paths_userdata_dir(char *out, size_t out_size);
bool util_paths_save_db(char *out, size_t out_size);
bool util_paths_resource_dir(char *out, size_t out_size);
bool util_paths_content_path(const char *relative_path, char *out, size_t out_size);
