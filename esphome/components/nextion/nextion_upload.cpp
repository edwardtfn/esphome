#include "nextion.h"

#ifdef USE_NEXTION_TFT_UPLOAD

#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"
#include <cinttypes>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif
#ifdef USE_ESP_IDF
#include <esp_http_client.h>
#endif

namespace esphome {
namespace nextion {
#ifdef ARDUINO
static const char *const TAG = "nextion.upload.arduino";
#elif defined(USE_ESP_IDF)
static const char *const TAG = "nextion.upload.idf";
#endif  // ARDUINO vs USE_ESP_IDF

// Followed guide
// https://unofficialnextion.com/t/nextion-upload-protocol-v1-2-the-fast-one/1044/2

size_t GetFreeHeap_() {
  #ifdef ARDUINO
  return ESP.getFreeHeap();
  #elif defined(USE_ESP_IDF)
  return GetFreeHeap_();
  #endif  // ARDUINO vs USE_ESP_IDF
}

int Nextion::upload_range(int range_start) {
  ESP_LOGVV(TAG, "url: %s", this->tft_url_.c_str());
  uint range_size = this->tft_size_ - range_start;
  ESP_LOGVV(TAG, "tft_size_: %i", this->tft_size_);
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  int range_end = (range_start == 0) ? std::min(this->tft_size_, 16383) : this->tft_size_;
  if (range_size <= 0 or range_end <= range_start) {
    ESP_LOGE(TAG, "Invalid range");
    ESP_LOGD(TAG, "Range start: %i", range_start);
    ESP_LOGD(TAG, "Range end: %i", range_end);
    ESP_LOGD(TAG, "Range size: %i", range_size);
    return -1;
  }

  #ifdef ARDUINO
  HTTPClient client;
  client.setTimeout(15000);  // Yes 15 seconds.... Helps 8266s along
  #ifdef USE_ESP8266
  #if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 7, 0)
  client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  #elif USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setFollowRedirects(true);
  #endif
  #if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setRedirectLimit(3);
  #endif
  #endif  // USE_ESP8266
  #elif defined(USE_ESP_IDF)
  esp_http_client_config_t config = {
      .url = this->tft_url_.c_str(),
      .cert_pem = nullptr,
      .disable_auto_redirect = false,
      .max_redirection_count = 10,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  #endif  // ARDUINO vs USE_ESP_IDF

  char range_header[64];
  sprintf(range_header, "bytes=%d-%d", range_start, range_end);
  ESP_LOGV(TAG, "Requesting range: %s", range_header);
  #ifdef ARDUINO
  int tries = 1;
  int code = 0;
  bool begin_status = false;
  while (tries <= 5) {
    ++tries;
  #ifdef USE_ESP32
    begin_status = client.begin(this->tft_url_.c_str());
  #endif
  #ifdef USE_ESP8266
    begin_status = client.begin(*this->get_wifi_client_(), this->tft_url_.c_str());
  #endif
    if (!begin_status) {
      ESP_LOGD(TAG, "upload_by_chunks_: connection failed");
      delay(500);  // NOLINT
      continue;
    }

    client.addHeader("Range", range_header);

    code = client.GET();
    if (code == 200 || code == 206) {
      break;
    }
    ESP_LOGW(TAG, "HTTP Request failed; Error: %s, retries(%d/5)",
             HTTPClient::errorToString(code).c_str(), tries);
    client.end();
    App.feed_wdt();
    delay(500);  // NOLINT
  }

  if (tries > 5 or !begin_status) {
    client.end();
    return -1;
  }
  #elif defined(USE_ESP_IDF)
  esp_http_client_set_header(client, "Range", range_header);

  ESP_LOGV(TAG, "Opening http connetion");
  esp_err_t err;
  if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return -1;
  }
  #endif  // ARDUINO vs USE_ESP_IDF

  ESP_LOGV(TAG, "Fetch content length");
  #ifdef ARDUINO
  int content_length = client.getStreamPtr()->available();
  #elif defined(USE_ESP_IDF)
  int content_length = esp_http_client_fetch_headers(client);
  #endif  // ARDUINO vs USE_ESP_IDF
  ESP_LOGV(TAG, "content_length = %d", content_length);
  if (content_length <= 0) {
    ESP_LOGE(TAG, "Failed to get content length: %d", content_length);
    #ifdef ARDUINO
    client.end();
    #elif defined(USE_ESP_IDF)
    esp_http_client_cleanup(client);
    #endif  // ARDUINO vs USE_ESP_IDF
    return -1;
  }

  int total_read_len = 0, read_len;
  std::string recv_string;

  ESP_LOGV(TAG, "Allocate buffer");
  uint8_t *buffer = new uint8_t[4096];
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  if (buffer == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate memory for buffer");
  } else {
    ESP_LOGV(TAG, "Memory for buffer allocated successfully");

    while (true) {
      App.feed_wdt();
      #ifdef ARDUINO
      int read_len = client.getStreamPtr()->readBytes(reinterpret_cast<char *>(buffer), 4096);
      #elif defined(USE_ESP_IDF)
      int read_len = esp_http_client_read(client, reinterpret_cast<char *>(buffer), 4096);
      #endif  // ARDUINO vs USE_ESP_IDF
      ESP_LOGVV(TAG, "Read %d bytes from HTTP client, writing to UART", read_len);
      if (read_len > 0) {
        this->write_array(buffer, read_len);
        this->recv_ret_string_(recv_string, 5000, true);
        this->content_length_ -= read_len;
        ESP_LOGD(TAG, "Uploaded %0.2f %%, remaining %d bytes, heap is %" PRIu32 " bytes",
                 100.0 * (this->tft_size_ - this->content_length_) / this->tft_size_, this->content_length_,
                 GetFreeHeap_());

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
            delete[] buffer;
            ESP_LOGVV(TAG, "Memory for buffer deallocated");
            ESP_LOGV(TAG, "Close http client");
            #ifdef ARDUINO
            client.end();
            #elif defined(USE_ESP_IDF)
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            #endif  // ARDUINO vs USE_ESP_IDF
            ESP_LOGVV(TAG, "Client closed");
            return result;
          }
        } else if (recv_string[0] != 0x05) {  // 0x05 == "ok"
          ESP_LOGE(
              TAG, "Invalid response from Nextion: [%s]",
              format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_string.data()), recv_string.size()).c_str());
          ESP_LOGV(TAG, "Deallocate buffer");
          delete[] buffer;
          ESP_LOGVV(TAG, "Memory for buffer deallocated");
          ESP_LOGV(TAG, "Close http client");
          #ifdef ARDUINO
          client.end();
          #elif defined(USE_ESP_IDF)
          esp_http_client_close(client);
          esp_http_client_cleanup(client);
          #endif  // ARDUINO vs USE_ESP_IDF
          ESP_LOGVV(TAG, "Client closed");
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
    delete[] buffer;
    ESP_LOGVV(TAG, "Memory for buffer deallocated");
  }
  ESP_LOGV(TAG, "Close http client");
  #ifdef ARDUINO
  client.end();
  #elif defined(USE_ESP_IDF)
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  #endif  // ARDUINO vs USE_ESP_IDF
  ESP_LOGVV(TAG, "Client closed");
  return range_end + 1;
}

bool Nextion::upload_tft() {
  ESP_LOGD(TAG, "Nextion TFT upload requested");
  ESP_LOGD(TAG, "url: %s", this->tft_url_.c_str());

  if (this->is_updating_) {
    ESP_LOGW(TAG, "Currently updating");
    return false;
  }

  if (!network::is_connected()) {
    ESP_LOGE(TAG, "Network is not connected");
    return false;
  }

  this->is_updating_ = true;

  // Define the configuration for the HTTP client
  ESP_LOGV(TAG, "Initializing HTTP client");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  #ifdef ARDUINO
  HTTPClient http;
  http.setTimeout(15000);  // Yes 15 seconds.... Helps 8266s along
  bool begin_status = false;
  #ifdef USE_ESP8266
  #if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 7, 0)
  client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  #elif USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setFollowRedirects(true);
  #endif
  #if USE_ARDUINO_VERSION_CODE >= VERSION_CODE(2, 6, 0)
  client.setRedirectLimit(3);
  #endif
  #endif  // USE_ESP8266
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

  //http.setReuse(true);
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
    return this->upload_end(false);
  }

  String content_range_string = http.header("Content-Range");
  content_range_string.remove(0, 12);
  this->tft_size_ = content_range_string.toInt();
  http.end();

  #elif defined(USE_ESP_IDF)
  esp_http_client_config_t config = {
      .url = this->tft_url_.c_str(),
      .cert_pem = nullptr,
      .method = HTTP_METHOD_HEAD,
      .timeout_ms = 15000,
      .disable_auto_redirect = false,
      .max_redirection_count = 10,
  };
  // Initialize the HTTP client with the configuration
  esp_http_client_handle_t http = esp_http_client_init(&config);
  if (!http) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client.");
    return this->upload_end(false);
  }

  // Perform the HTTP request
  ESP_LOGV(TAG, "Check if the client could connect");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  esp_err_t err = esp_http_client_perform(http);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(http);
    return this->upload_end(false);
  }

  // Check the HTTP Status Code
  ESP_LOGV(TAG, "Check the HTTP Status Code");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  int status_code = esp_http_client_get_status_code(http);
  ESP_LOGV(TAG, "HTTP Status Code: %d", status_code);
  this->tft_size_ = esp_http_client_get_content_length(http);
  ESP_LOGD(TAG, "TFT file size: %zu", this->tft_size_);

  ESP_LOGD(TAG, "Close HTTP connection");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  esp_http_client_close(http);
  esp_http_client_cleanup(http);
  ESP_LOGVV(TAG, "Connection closed");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  #endif  // ARDUINO vs USE_ESP_IDF

  if (this->tft_size_ < 4096) {
    ESP_LOGE(TAG, "File size check failed. Size: %zu", this->tft_size_);
    return this->upload_end(false);
  } else {
    ESP_LOGV(TAG, "File size check passed. Proceeding...");
  }
  this->content_length_ = this->tft_size_;

  ESP_LOGD(TAG, "Updating Nextion");

  // The Nextion will ignore the update command if it is sleeping
  ESP_LOGV(TAG, "Wake-up Nextion");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  this->send_command_("sleep=0");
  this->set_backlight_brightness(1.0);
  vTaskDelay(pdMS_TO_TICKS(250));  // NOLINT

  App.feed_wdt();
  char command[128];
  // Tells the Nextion the content length of the tft file and baud rate it will be sent at
  // Once the Nextion accepts the command it will wait until the file is successfully uploaded
  // If it fails for any reason a power cycle of the display will be needed
  sprintf(command, "whmi-wris %d,%" PRIu32 ",1", this->content_length_, this->parent_->get_baud_rate());

  // Clear serial receive buffer
  ESP_LOGV(TAG, "Clear serial receive buffer");
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  uint8_t d;
  while (this->available()) {
    this->read_byte(&d);
  };

  ESP_LOGV(TAG, "Send update instruction: %s", command);
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  this->send_command_(command);

  std::string response;
  ESP_LOGV(TAG, "Waiting for upgrade response");
  this->recv_ret_string_(response, 2000, true);  // This can take some time to return

  // The Nextion display will, if it's ready to accept data, send a 0x05 byte.
  ESP_LOGD(TAG, "Upgrade response is [%s] - %zu bytes",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(response.data()), response.size()).c_str(),
           response.length());
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());

  if (response.find(0x05) != std::string::npos) {
    ESP_LOGV(TAG, "Preparation for tft update done");
  } else {
    ESP_LOGE(TAG, "Preparation for tft update failed %d \"%s\"", response[0], response.c_str());
    return this->upload_end(false);
  }

  delay(500);
  ESP_LOGD(TAG, "Updating TFT to Nextion:");
  ESP_LOGD(TAG, "  URL: %s", this->tft_url_.c_str());
  ESP_LOGD(TAG, "  File size: %d", this->content_length_);
  ESP_LOGD(TAG, "  Free heap: %" PRIu32, GetFreeHeap_());

  delay(500);

  ESP_LOGV(TAG, "Starting transfer by chunks loop");
  int result = 0;
  while (this->content_length_ > 0) {
    delay(500);
    result = upload_range(result);
    if (result < 0) {
      ESP_LOGE(TAG, "Error updating Nextion!");
      return this->upload_end(false);
    }
    App.feed_wdt();
    ESP_LOGV(TAG, "Free heap: %" PRIu32 ", Bytes left: %d", GetFreeHeap_(), this->content_length_);
  }

  ESP_LOGD(TAG, "Successfully updated Nextion!");

  return upload_end(true);
}

bool Nextion::upload_end(bool successful) {
  ESP_LOGVV(TAG, "Free heap: %" PRIu32, GetFreeHeap_());
  this->is_updating_ = false;
  ESP_LOGD(TAG, "Restarting Nextion");
  this->soft_reset();
  vTaskDelay(pdMS_TO_TICKS(1500));  // NOLINT
  if (successful) {
    ESP_LOGD(TAG, "Restarting ESPHome");
    esp_restart();  // NOLINT(readability-static-accessed-through-instance)
  }
  return successful;
}

}  // namespace nextion
}  // namespace esphome

#endif  // USE_NEXTION_TFT_UPLOAD
