#ifdef USE_NEXTION_TFT_UPLOAD

#pragma once

#ifdef ARDUINO
#ifdef USE_ESP32
#include <HTTPClient.h>
#endif  // USE_ESP32
#ifdef USE_ESP8266
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif  // USE_ESP8266
#elif defined(USE_ESP_IDF)
#include <esp_http_client.h>
#endif  // ARDUINO vs ESP-IDF

namespace esphome {
namespace nextion {

public:
  /**
   * @enum TFTUploadResult
   * @brief Enumeration for the results of a TFT upload operation.
   *
   * This enum is used to represent the various outcomes (both success and failure)
   * of an attempt to upload a TFT file to a Nextion display.
   */
  enum class TFTUploadResult {
    /**
     * @brief The upload state is unknown.
     * This may occur if the upload process is disrupted or the status cannot be determined.
     */
    UNKNOWN,

    /**
     * @brief The upload operation completed successfully.
     * The TFT file has been uploaded and applied to the Nextion display.
     */
    OK,

    /**
     * @brief Another upload is already in progress.
     * Wait for the current operation to complete before initiating a new upload.
     */
    UPLOAD_IN_PROGRESS,

    /**
     * @brief Network is not connected.
     * Ensure the device is connected to a network before attempting to upload.
     */
    NETWORK_ERROR_NOT_CONNECTED,

    /**
     * @brief Connection to the HTTP server failed.
     * Check the server URL and network connection.
     */
    HTTP_ERROR_CONNECTION_FAILED,

    /**
     * @brief Received a server error response from the HTTP server.
     * Indicates a problem on the server side.
     */
    HTTP_ERROR_RESPONSE_SERVER,

    /**
     * @brief Indicates that the requested resource was not found on the server.
     * This error is triggered when the HTTP server responds with a 404 status code, which typically occurs if the URL
     * to the TFT file is incorrect, the file has been removed, or the server configuration does not route the request
     * to the intended resource.
     *
     * This specific error points to a situation where the request was sent correctly but the server could not find the
     * requested resource. To address this error, ensure that the URL for the TFT file is accurate and that the file
     * indeed exists at the specified location on the server. Additionally, check the server's routing and
     * configuration settings to ensure they correctly handle requests for the resource. Correcting the URL, verifying
     * the presence of the file on the server, and confirming server configurations are primary steps for resolving
     * this issue.
     */
    HTTP_ERROR_RESPONSE_NOT_FOUND,

    /**
     * @brief Received a client error response from the HTTP server.
     * Check the request for errors.
     */
    HTTP_ERROR_RESPONSE_CLIENT,

    /**
     * @brief Received a redirection response from the HTTP server that could not be followed.
     */
    HTTP_ERROR_RESPONSE_REDIRECTION,

    /**
     * @brief Received an unexpected response from the HTTP server.
     * Verify the server and request.
     */
    HTTP_ERROR_RESPONSE_OTHER,

    /**
     * @brief The HTTP server provided an invalid header.
     * Validate the server's response format.
     */
    HTTP_ERROR_INVALID_SERVER_HEADER,

    /**
     * @brief Failed to initialize the HTTP client.
     * Ensure the client setup is correct.
     */
    HTTP_ERROR_CLIENT_INITIALIZATION,

    /**
     * @brief Failed to setup a persistent connection with the HTTP server.
     * Check the server's keep-alive settings.
     */
    HTTP_ERROR_KEEP_ALIVE,

    /**
     * @brief The HTTP request failed. Review the request configuration and server availability.
     */
    HTTP_ERROR_REQUEST_FAILED,

    /**
     * @brief The downloaded file size did not match the expected size.
     * Verify the file source and size specified.
     */
    HTTP_ERROR_INVALID_FILE_SIZE,

    /**
     * @brief Failed to fetch the full package from the HTTP server.
     * Ensure the server is serving the complete file.
     */
    HTTP_ERROR_FAILED_TO_FETCH_FULL_PACKAGE,

    /**
     * @brief Failed to open a connection to the HTTP server.
     * Check the network connection and server URL.
     */
    HTTP_ERROR_FAILED_TO_OPEN_CONNECTION,

    /**
     * @brief Failed to get content length from the HTTP server.
     * Verify the server's response headers.
     */
    HTTP_ERROR_FAILED_TO_GET_CONTENT_LENGTH,

    /**
     * @brief Failed to set the HTTP method for the request.
     * Ensure the HTTP client supports the intended method.
     */
    HTTP_ERROR_SET_METHOD_FAILED,

    // Nextion Errors
    /**
     * @brief Preparation for TFT upload failed.
     * Check the device's readiness and connection status.
     */
    NEXTION_ERROR_PREPARATION_FAILED,

    /**
     * @brief Received an invalid response from Nextion.
     * Verify the command sent and the device's state.
     */
    NEXTION_ERROR_INVALID_RESPONSE,

    /**
     * @brief Failed to send an exit reparse command to Nextion.
     * Ensure the device is connected and ready to receive commands.
     */
    NEXTION_ERROR_EXIT_REPARSE_NOT_SENT,

    // Process Errors
    /**
     * @brief Invalid range requested.
     * Verify the range values are within acceptable limits.
     */
    PROCESS_ERROR_INVALID_RANGE,

    // Memory Errors
    /**
     * @brief Failed to allocate the necessary memory.
     * Ensure there is sufficient memory available for the operation.
     */
    MEMORY_ERROR_FAILED_TO_ALLOCATE,
  };

  /**
   * @brief Converts TFTUploadResult enum values to their corresponding string representations.
   *
   * This function is used to obtain human-readable strings for logging or displaying
   * the outcomes of TFT upload operations to a Nextion display. It maps each enum value
   * defined in TFTUploadResult to a descriptive text message.
   *
   * @param result The TFTUploadResult enum value to be converted to a string.
   * @return A const char* pointing to the string representation of the given result.
   */
  static const char *tft_upload_result_to_string(TFTUploadResult result);

  /**
   * Set the tft file URL. https seems problematic with arduino..
   */
  void set_tft_url(const std::string &tft_url) { this->tft_url_ = tft_url; }

  /**
   * @brief Uploads the TFT file to the Nextion display.
   *
   * This function initiates the upload of a TFT file to the Nextion display. Users can specify a target baud rate for
   * the transfer. If the provided baud rate is not supported by Nextion, the function defaults to using the current
   * baud rate set for the display. If no baud rate is specified (or if 0 is passed), the current baud rate is used.
   *
   * Supported baud rates are: 2400, 4800, 9600, 19200, 31250, 38400, 57600, 115200, 230400, 250000, 256000, 512000
   * and 921600. Selecting a baud rate supported by both the Nextion display and the host hardware is essential for
   * ensuring a successful upload process.
   *
   * @param baud_rate The desired baud rate for the TFT file transfer, specified as an unsigned 32-bit integer.
   * If the specified baud rate is not supported, or if 0 is passed, the function will use the current baud rate.
   * The default value is 0, which implies using the current baud rate.
   * @param exit_reparse If true, the function exits reparse mode before uploading the TFT file. This parameter
   * defaults to true, ensuring that the display is ready to receive and apply the new TFT file without needing
   * to manually reset or reconfigure. Exiting reparse mode is recommended for most upload scenarios to ensure
   * the display properly processes the uploaded file command.
   * @return Nextion::TFTUploadResult Indicates the outcome of the transfer. Returns Nextion::TFTUploadResult::OK for
   * success, otherwise, indicates failure with additional details on the operation status. These additional details
   * can include specific error codes or messages that help identify the cause of failure, making troubleshooting more
   * straightforward.
   */
  Nextion::TFTUploadResult upload_tft(uint32_t baud_rate = 0, bool exit_reparse = true);

protected:
#ifdef USE_ESP8266
  WiFiClient *wifi_client_{nullptr};
  BearSSL::WiFiClientSecure *wifi_client_secure_{nullptr};
  WiFiClient *get_wifi_client_();
#endif
  int content_length_ = 0;
  int tft_size_ = 0;
  uint32_t original_baud_rate_ = 0;

  /**
   * @brief Evaluates the HTTP response code received and categorizes it into specific TFTUploadResult errors.
   *
   * This function is designed to interpret the HTTP response codes and convert them into corresponding TFTUploadResult
   * enum values that indicate the outcome of the TFT file upload attempt.
   *
   * @param code The HTTP status code received from the server as part of the TFT file upload process.
   *
   * @return TFTUploadResult The result of evaluating the HTTP status code:
   *         - HTTP_ERROR_RESPONSE_SERVER for server-side errors (HTTP status codes 500 and above).
   *         - HTTP_ERROR_RESPONSE_NOT_FOUND for a 404 Not Found error, highlighting the frequent scenario of missing
   * resources, especially relevant in the context of local servers.
   *         - HTTP_ERROR_RESPONSE_CLIENT for other client-side errors (HTTP status codes in the 400 range
   * excluding 404).
   *         - HTTP_ERROR_RESPONSE_REDIRECTION for redirection errors (HTTP status codes in the 300 range).
   *         - HTTP_ERROR_RESPONSE_OTHER for any other response codes that don't indicate a clear success (anything
   * other than 200 and 206) or when the number of tries exceeds a certain limit, indicating potential issues in the
   * upload process.
   *         - OK for successful uploads where the server's response indicates that the file has been accepted and
   * processed (HTTP status codes 200 and 206).
   *
   * Note: This function is an internal utility used to streamline the error handling process for TFT uploads to
   * Nextion displays by mapping various HTTP response scenarios to more specific error outcomes.
   */
  TFTUploadResult handle_http_response_code_(int code);

  /**
   * This function requests a specific range of data from an HTTP server using a persistent connection
   * and sends it to the Nextion display in predefined chunks.
   * It is compatible with both Arduino and ESP-IDF environments,
   * leveraging a pre-initialized HTTP client to maintain the connection for consecutive range requests.
   * The function determines the range of data to request based on the current position, indicated by `range_start`,
   * and internally uses a fixed chunk size for data transmission.
   * The `range_start` parameter is updated after each successful chunk transfer, facilitating sequential data fetching.
   * The data is sent to the Nextion display in chunks, ensuring efficient and manageable data transfer.
   *
   * For Arduino environments, the function requires an HTTPClient object passed by reference,
   * which is used to manage the connection and request headers.
   * In ESP-IDF environments, an esp_http_client_handle_t is required, representing the handle to the initialized HTTP
   * client.
   *
   * @param http_client The HTTP client instance for making the range request. For Arduino, this should be an HTTPClient
   * object; for ESP-IDF, an esp_http_client_handle_t.
   * @param range_start A reference to an integer that specifies the starting position of the current data transfer.
   * This value is updated to mark the beginning of the next chunk after a successful transfer.
   *
   * @return A Nextion::TFTUploadResult indicating the outcome of the transfer. It can be an OK status for successful
   * transfers, or various error codes that detail the nature of any failure encountered during the operation.
   */
#ifdef ARDUINO
  TFTUploadResult upload_by_chunks_(HTTPClient &http_client, int &range_start);
#elif defined(USE_ESP_IDF)
  TFTUploadResult upload_by_chunks_(esp_http_client_handle_t http_client, int &range_start);
#endif  // ARDUINO vs USE_ESP_IDF

  /**
   * Ends the upload process, restart Nextion and, if successful,
   * restarts ESP
   * @param Nextion::TFTUploadResult result of the transfer.
   * @return Nextion::TFTUploadResult result of the transfer.
   */
  TFTUploadResult upload_end_(TFTUploadResult upload_results);

  /**
   * Returns the ESP Free Heap memory. This is framework independent.
   * @return Free Heap in bytes.
   */
  uint32_t get_free_heap_();

  std::string tft_url_;
  bool upload_first_chunk_sent_ = false;

}  // namespace nextion
}  // namespace esphome

#endif  // USE_NEXTION_TFT_UPLOAD
