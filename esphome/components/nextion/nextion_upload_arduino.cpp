#include "nextion.h"

#ifdef ARDUINO
#ifdef USE_NEXTION_TFT_UPLOAD

#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

namespace esphome {
namespace nextion {
static const char *const TAG = "nextion.upload.arduino";

// Followed guide
// https://unofficialnextion.com/t/nextion-upload-protocol-v1-2-the-fast-one/1044/2

int Nextion::upload_range(const std::string &url, int range_start) {
  ESP_LOGVV(TAG, "url: %s", url.c_str());
  uint range_size = this->tft_size_ - range_start;
  ESP_LOGVV(TAG, "tft_size_: %i", this->tft_size_);
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  int range_end = (range_start == 0) ? std::min(this->tft_size_, 16383) : this->tft_size_;
  if (range_size <= 0 or range_end <= range_start) {
    ESP_LOGE(TAG, "Invalid range");
    ESP_LOGD(TAG, "Range start: %i", range_start);
    ESP_LOGD(TAG, "Range end: %i", range_end);
    ESP_LOGD(TAG, "Range size: %i", range_size);
    return -1;
  }
  
  HTTPClient client;
  http.setTimeout(15000);  // Yes 15 seconds.... Helps 8266s along
  bool begin_status = false;
#ifdef USE_ESP32
  begin_status = client.begin(this->tft_url_.c_str());
#endif
#ifdef USE_ESP8266
#if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 7, 0)
  client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
#elif USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setFollowRedirects(true);
#endif
#if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setRedirectLimit(3);
#endif
  begin_status = client.begin(*this->get_wifi_client_(), this->tft_url_.c_str());
#endif

  char range_header[64];
  sprintf(range_header, "bytes=%d-%d", range_start, range_end);
  ESP_LOGV(TAG, "Requesting range: %s", range_header);
  client->addHeader("Range", range_header);
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());

  code = client->GET();
  if (code != 200 and code != 206) {
    ESP_LOGW(TAG, "HTTP Request failed; URL: %s; Error: %s, retries(%d/5)", this->tft_url_.c_str(),
            HTTPClient::errorToString(code).c_str(), tries);
    client->end();
    App.feed_wdt();
    delay(500);  // NOLINT


  int total_read_len = 0, read_len;

  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  ESP_LOGV(TAG, "Allocate buffer");
  uint8_t *buffer = new uint8_t[4096];
  std::string recv_string;
  if (buffer == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate memory for buffer");
    ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  } else {
    ESP_LOGV(TAG, "Memory for buffer allocated successfully");

    while (true) {
      App.feed_wdt();
      ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
      size = client->getStreamPtr()->available();
      if (!size) {
        App.feed_wdt();
        delay(2);
        continue;
      }
      int read_len = client->getStreamPtr()->readBytes(&buffer, 4096);
      ESP_LOGVV(TAG, "Read %d bytes from HTTP client, writing to UART", read_len);
      if (read_len > 0) {
        this->write_array(buffer, read_len);
        ESP_LOGVV(TAG, "Write to UART successful");
        this->recv_ret_string_(recv_string, 5000, true);
        this->content_length_ -= read_len;
        ESP_LOGD(TAG, "Uploaded %0.2f %%, remaining %d bytes, heap is %" PRIu32 " bytes",
                 100.0 * (this->tft_size_ - this->content_length_) / this->tft_size_, this->content_length_,
                 ESP.getFreeHeap());

        if (recv_string[0] == 0x08 && recv_string.size() == 5) {  // handle partial upload request
          ESP_LOGD(
              TAG, "recv_string [%s]",
              format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_string.data()), recv_string.size()).c_str());
          uint32_t result = 0;
          for (int j = 0; j < 4; ++j) {
            result += static_cast<uint8_t>(recv_string[j + 1]) << (8 * j);
          }
          if (result > 0) {
            ESP_LOGI(TAG, "Nextion reported new range %" PRIu32, result);
            this->content_length_ = this->tft_size_ - result;
            // Deallocate the buffer when done
            ESP_LOGV(TAG, "Deallocate buffer");
            ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
            delete[] buffer;
            ESP_LOGVV(TAG, "Memory for buffer deallocated");
            ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
            ESP_LOGV(TAG, "Close http client");
            ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
            client->end();
            ESP_LOGVV(TAG, "Client closed");
            ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
            return result;
          }
        } else if (recv_string[0] != 0x05) {  // 0x05 == "ok"
          ESP_LOGE(
              TAG, "Invalid response from Nextion: [%s]",
              format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_string.data()), recv_string.size()).c_str());
          ESP_LOGV(TAG, "Deallocate buffer");
          ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
          delete[] buffer;
          ESP_LOGVV(TAG, "Memory for buffer deallocated");
          ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
          ESP_LOGV(TAG, "Close http client");
          ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
          client->end();
          ESP_LOGVV(TAG, "Client closed");
          ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
          return -1;
        }

        recv_string.clear();
      } else if (read_len == 0) {
        ESP_LOGV(TAG, "End of HTTP response reached");
        break;  // Exit the loop if there is no more data to read
      } else {
        ESP_LOGE(TAG, "Failed to read from HTTP client, error code: %d", read_len);
        break;  // Exit the loop on error
      }
    }

    // Deallocate the buffer when done
    ESP_LOGV(TAG, "Deallocate buffer");
    ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
    delete[] buffer;
    ESP_LOGVV(TAG, "Memory for buffer deallocated");
    ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  }
  ESP_LOGV(TAG, "Close http client");
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  client->end();
  ESP_LOGVV(TAG, "Client closed");
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  return range_end + 1;
}

bool Nextion::upload_tft() {
  ESP_LOGD(TAG, "Nextion TFT upload requested");
  ESP_LOGD(TAG, "URL: %s", this->tft_url_.c_str());

  if (this->is_updating_) {
    ESP_LOGD(TAG, "Currently updating");
    return false;
  }

  if (!network::is_connected()) {
    ESP_LOGD(TAG, "network is not connected");
    return false;
  }

  this->is_updating_ = true;

  HTTPClient http;
  http.setTimeout(15000);  // Yes 15 seconds.... Helps 8266s along
  bool begin_status = false;
#ifdef USE_ESP32
  begin_status = http.begin(this->tft_url_.c_str());
#endif
#ifdef USE_ESP8266
#if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 7, 0)
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
#elif USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  http.setFollowRedirects(true);
#endif
#if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  http.setRedirectLimit(3);
#endif
  begin_status = http.begin(*this->get_wifi_client_(), this->tft_url_.c_str());
#endif

  if (!begin_status) {
    this->is_updating_ = false;
    ESP_LOGD(TAG, "Connection failed");
    ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
    allocator.deallocate(this->transfer_buffer_, this->transfer_buffer_size_);
    return false;
  } else {
    ESP_LOGD(TAG, "Connected");
  }

  http.addHeader("Range", "bytes=0-255");
  const char *header_names[] = {"Content-Range"};
  http.collectHeaders(header_names, 1);
  ESP_LOGD(TAG, "Requesting URL: %s", this->tft_url_.c_str());

  http.setReuse(true);
  // try up to 5 times. DNS sometimes needs a second try or so
  int tries = 1;
  int code = http.GET();
  delay(100);  // NOLINT

  App.feed_wdt();
  while (code != 200 && code != 206 && tries <= 5) {
    ESP_LOGW(TAG, "HTTP Request failed; URL: %s; Error: %s, retrying (%d/5)", this->tft_url_.c_str(),
             HTTPClient::errorToString(code).c_str(), tries);

    delay(250);  // NOLINT
    App.feed_wdt();
    code = http.GET();
    ++tries;
  }

  if ((code != 200 && code != 206) || tries > 5) {
    return this->upload_end_(false);
  }

  String content_range_string = http.header("Content-Range");
  content_range_string.remove(0, 12);
  this->content_length_ = content_range_string.toInt();
  this->tft_size_ = content_length_;
  http.end();

  if (this->content_length_ < 4096) {
    ESP_LOGE(TAG, "Failed to get file size");
    return this->upload_end_(false);
  }

  ESP_LOGD(TAG, "Updating Nextion %s...", this->device_model_.c_str());
  // The Nextion will ignore the update command if it is sleeping

  this->send_command_("sleep=0");
  this->set_backlight_brightness(1.0);
  delay(250);  // NOLINT

  App.feed_wdt();

  char command[128];
  // Tells the Nextion the content length of the tft file and baud rate it will be sent at
  // Once the Nextion accepts the command it will wait until the file is successfully uploaded
  // If it fails for any reason a power cycle of the display will be needed
  sprintf(command, "whmi-wris %d,%d,1", this->content_length_, this->parent_->get_baud_rate());

  // Clear serial receive buffer
  ESP_LOGV(TAG, "Clear serial receive buffer");
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  uint8_t d;
  while (this->available()) {
    this->read_byte(&d);
  };

  ESP_LOGV(TAG, "Send update instruction: %s", command);
  ESP_LOGVV(TAG, "Available heap: %" PRIu32, ESP.getFreeHeap());
  this->send_command_(command);

  std::string response;
  ESP_LOGV(TAG, "Waiting for upgrade response");
  this->recv_ret_string_(response, 5000, true);  // This can take some time to return

  // The Nextion display will, if it's ready to accept data, send a 0x05 byte.
  ESP_LOGD(TAG, "Upgrade response is [%s] - %zu bytes",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(response.data()), response.size()).c_str(),
           response.length());

  if (response.find(0x05) != std::string::npos) {
    ESP_LOGD(TAG, "Preparation for tft update done");
  } else {
    ESP_LOGD(TAG, "Preparation for tft update failed %d \"%s\"", response[0], response.c_str());
    return this->upload_end_(false);
  }

  // Nextion wants 4096 bytes at a time. Make chunk_size a multiple of 4096
#ifdef USE_ESP32
  uint32_t chunk_size = 8192;
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0) {
    chunk_size = this->content_length_;
  } else {
    if (ESP.getFreeHeap() > 81920) {  // Ensure some FreeHeap to other things and limit chunk size
      chunk_size = ESP.getFreeHeap() - 65536;
      chunk_size = int(chunk_size / 4096) * 4096;
      chunk_size = chunk_size > 32768 ? 32768 : chunk_size;
    } else if (ESP.getFreeHeap() < 32768) {
      chunk_size = 4096;
    }
  }
#else
  // NOLINTNEXTLINE(readability-static-accessed-through-instance)
  uint32_t chunk_size = ESP.getFreeHeap() < 16384 ? 4096 : 8192;
#endif

  if (this->transfer_buffer_ == nullptr) {
    ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    ESP_LOGD(TAG, "Allocating buffer size %d, Heap size is %u", chunk_size, ESP.getFreeHeap());
    this->transfer_buffer_ = allocator.allocate(chunk_size);
    if (this->transfer_buffer_ == nullptr) {  // Try a smaller size
      ESP_LOGD(TAG, "Could not allocate buffer size: %d trying 4096 instead", chunk_size);
      chunk_size = 4096;
      ESP_LOGD(TAG, "Allocating %d buffer", chunk_size);
      this->transfer_buffer_ = allocator.allocate(chunk_size);

      if (!this->transfer_buffer_)
        return this->upload_end_(false);
    }

    this->transfer_buffer_size_ = chunk_size;
  }

  // NOLINTNEXTLINE(readability-static-accessed-through-instance)
  ESP_LOGD(TAG, "Updating tft from \"%s\" with a file size of %d using %zu chunksize, Heap Size %d",
           this->tft_url_.c_str(), this->content_length_, this->transfer_buffer_size_, ESP.getFreeHeap());

  int result = 0;
  while (this->content_length_ > 0) {
    result = this->upload_range(this->tft_url_.c_str(), result);
    //result = this->upload_by_chunks_(&http, result);
    if (result < 0) {
      ESP_LOGD(TAG, "Error updating Nextion!");
      return this->upload_end_(false);
    }
    App.feed_wdt();
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    ESP_LOGD(TAG, "Heap Size %d, Bytes left %d", ESP.getFreeHeap(), this->content_length_);
  }
  ESP_LOGD(TAG, "Successfully updated Nextion!");

  return this->upload_end_(true);
}

bool Nextion::upload_end_(bool successful) {
  this->is_updating_ = false;
  ESP_LOGD(TAG, "Restarting Nextion");
  this->soft_reset();
  if (successful) {
    delay(1500);  // NOLINT
    ESP_LOGD(TAG, "Restarting esphome");
    ESP.restart();  // NOLINT(readability-static-accessed-through-instance)
  }
  return successful;
}

#ifdef USE_ESP8266
WiFiClient *Nextion::get_wifi_client_() {
  if (this->tft_url_.compare(0, 6, "https:") == 0) {
    if (this->wifi_client_secure_ == nullptr) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      this->wifi_client_secure_ = new BearSSL::WiFiClientSecure();
      this->wifi_client_secure_->setInsecure();
      this->wifi_client_secure_->setBufferSizes(512, 512);
    }
    return this->wifi_client_secure_;
  }

  if (this->wifi_client_ == nullptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    this->wifi_client_ = new WiFiClient();
  }
  return this->wifi_client_;
}
#endif
}  // namespace nextion
}  // namespace esphome

#endif  // USE_NEXTION_TFT_UPLOAD
#endif  // ARDUINO
