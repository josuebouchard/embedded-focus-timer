#include "pomodoro_uart.h"
#include "driver/uart.h"
#include "esp_err.h"
#include <ctype.h>
#include <string.h>

static const uart_port_t UART_PORT = UART_NUM_0;

void configure_uart(void) {
  // Information:
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html

  // Install driver
  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 2048, 0, 0, NULL, 0));

  // Set communication parameters
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
}

/**
 * @brief Trim leading and trailing ASCII whitespace from a string.
 *
 * Removes whitespace characters (as defined by isspace()) from the beginning
 * and end of the string. The operation is performed in place.
 *
 * @param s Pointer to a mutable, null-terminated string buffer.
 *
 * @return Pointer to the first non-whitespace character within the original
 *         buffer, or NULL if @p s is NULL.
 *
 * @note The returned pointer may differ from the input pointer.
 * @note The input buffer must be writable (do not pass string literals).
 * @note The function does not allocate memory.
 * @note Whitespace detection is ASCII-based (locale-independent).
 *
 * @example
 * char buf[] = "  hello world \r\n";
 * char *trimmed = str_trim(buf);
 * // trimmed -> "hello world"
 */
static char *str_trim(char *s) {
  char *end;

  if (s == NULL)
    return NULL;

  // Trim leading whitespace
  while (*s && isspace((unsigned char)*s))
    s++;

  // All spaces?
  if (*s == '\0')
    return s;

  // Trim trailing whitespace
  end = s + strlen(s) - 1;
  while (end > s && isspace((unsigned char)*end)) {
    end--;
  }

  // Write new null terminator
  end[1] = '\0';

  return s;
}

esp_err_t read_line(char *buf, uint32_t length, TickType_t ticks_to_wait,
                    char **out_trimmed) {
  // Ensure arguments are good
  if (!buf || length == 0 || !out_trimmed) {
    return ESP_ERR_INVALID_ARG;
  }

  if (length == 1) {
    buf[0] = '\0';
    *out_trimmed = buf;
    return ESP_OK;
  }

  // General case

  esp_err_t status = ESP_OK;
  uint8_t character_read; // This is the representation `uart_read_bytes` uses
  uint32_t i = 0;

  // The last byte is reserved for the termination byte
  while (i < length - 1) {
    int bytes_read =
        uart_read_bytes(UART_PORT, &character_read, 1, ticks_to_wait);

    // Handle special cases
    if (bytes_read < 0) {
      status = ESP_FAIL;
      break;
    } else if (bytes_read == 0) {
      status = ESP_ERR_TIMEOUT;
      break;
    }

    if (character_read == '\r' || character_read == '\n')
      break;

    buf[i] = character_read;
    i++;
  }

  // Set sane defaults on error
  if (status != ESP_OK) {
    buf[0] = '\0';
    // `*out_trimmed` will be set later by `str_trim()`
  } else {
    // Flush rest of overly-long line
    if (i == length - 1) {
      do {
        int bytes_read = uart_read_bytes(UART_PORT, &character_read, 1, portMAX_DELAY);
        if (bytes_read < 0)
          return ESP_FAIL;
      } while (character_read != '\n' && character_read != '\r');
    }

    // Never forget termination byte
    buf[i] = '\0';
  }

  *out_trimmed = str_trim(buf);

  return status;
}
