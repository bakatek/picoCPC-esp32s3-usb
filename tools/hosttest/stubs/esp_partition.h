#pragma once
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_DATA_FAT 2
struct esp_partition_t { uint32_t size; };
const esp_partition_t* esp_partition_find_first(int,int,const char*);
esp_err_t esp_partition_read(const esp_partition_t*, size_t off, void* dst, size_t n);
esp_err_t esp_partition_write(const esp_partition_t*, size_t off, const void* src, size_t n);
esp_err_t esp_partition_erase_range(const esp_partition_t*, size_t off, size_t n);
