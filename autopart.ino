#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_flash.h"

#include <FFat.h>

size_t last_flash_used() {
  esp_partition_iterator_t it;
  size_t endpt = 0;
  it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL); 
  for (; it != NULL; it = esp_partition_next(it)) {
    const esp_partition_t *part = esp_partition_get(it);
    endpt = part->address >= endpt ? part->address + part->size : endpt;
  }
  esp_partition_iterator_release(it);
  return endpt;
}

void set_size() {
  uint32_t sz = esp_flash_default_chip->chip_id;
  sz = ((sz & 0xff) << 16) | ((sz >> 16) & 0xff) | (sz & 0xff00);
  sz = (sz >> 16) & 0xFF;
  sz = 2 << (sz - 1);
  esp_flash_default_chip->size = sz;
}

bool register_partition() {
  size_t endpt = last_flash_used();
  set_size();
  if (endpt >= esp_flash_default_chip->size) {
    log_i("No free space on flash");
    return false;
  }
  esp_err_t err;
  err = esp_partition_register_external(esp_flash_default_chip, endpt, esp_flash_default_chip->size - endpt, "ffat",
              ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
  if (err) {
    log_e("Error registering partition: %d", err);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  if (register_partition()) Serial.println("Fat partition created");
  if (!FFat.begin(true)) {
    Serial.println("FFat Mount Failed");
    return;
  }
  Serial.printf("Free space: %10u\n", FFat.freeBytes());  
}

void loop() {}
