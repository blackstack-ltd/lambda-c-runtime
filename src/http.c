#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "http.h"

static char error[1024];

static size_t write_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  size_t realsize = size*nitems;
  mem_t *mem = (mem_t *) userdata;

  if ((mem->mem = realloc(mem->mem, mem->size + realsize + 1)) == NULL) {
    snprintf(error, 256, "realloc: allocation failed");
    return 0;
  }

  memcpy(&mem->mem[mem->size], buffer, realsize);
  mem->size += realsize;
  mem->mem[mem->size] = '\0';

  return realsize;
}

static size_t read_callback(char *dest, size_t size, size_t nmemb, void *userp) {
  mem_t *mem = (mem_t *)userp;
  size_t buffer_size = size*nmemb, copy_size;

  if (mem->size == 0)
    return 0;

  copy_size = mem->size;

  if(copy_size > buffer_size)
    copy_size = buffer_size;

  memcpy(dest, mem->mem, copy_size);

  mem->mem += copy_size;
  mem->size -= copy_size;

  return copy_size;
}

#define INITHDR() struct curl_slist *headers = NULL

#define INIT() CURL *curl = NULL; \
CURLcode err = CURLE_OK; \
if ((curl = curl_easy_init()) == NULL) { \
  snprintf(error, 256, "curl_easy_init: init failed"); \
  goto clean; \
} \
(void) err

#define SETOPT(X, Y) if ((err = curl_easy_setopt(curl, X, (Y))) != CURLE_OK) { \
  snprintf(error, 256, "curl_easy_setopt: %s: %s", #X, curl_easy_strerror(err)); \
  goto clean; \
} \
(void) err

#define SETHDR(X) if ((headers = curl_slist_append(headers, X)) == NULL) { \
  snprintf(error, 256, "curl_slist_append: cannot append %s header", X); \
  goto clean; \
} \
(void) err

#define PERFORM() if ((err = curl_easy_perform(curl)) != CURLE_OK) { \
  snprintf(error, 256, "curl_easy_perform: %s", curl_easy_strerror(err)); \
  goto clean; \
} \
(void) err

#define ENDHDR() if (headers) curl_slist_free_all(headers)

#define END() if (curl) curl_easy_cleanup(curl)

char *http_get(const char *url, hdr_t *headers, unsigned int headers_len) {
  char *rc = NULL;
  mem_t body = { NULL, 0 };

  INIT();
  SETOPT(CURLOPT_NOPROGRESS, 1L);
  SETOPT(CURLOPT_VERBOSE, 0L);
  SETOPT(CURLOPT_URL, url);
  SETOPT(CURLOPT_FOLLOWLOCATION, 1L);
  SETOPT(CURLOPT_WRITEFUNCTION, write_callback);
  SETOPT(CURLOPT_WRITEDATA, (void *) &body);
  PERFORM();

  for (unsigned int h = 0; h < headers_len; h++) {
    struct curl_header *header;
    if (curl_easy_header(curl, headers[h].name, 0, CURLH_HEADER, -1, &header) == CURLHE_OK) {
      snprintf(headers[h].value, 1024, "%s", header->value);
    } else {
      headers[h].value[0] = '\0';
    }
  }

  rc = body.mem;
clean:
  if (rc == NULL && body.mem != NULL)
    free(body.mem);

  END();

  return rc;
}

char *http_post(const char *url, const char *payload_string) {
  char *rc = NULL;
  mem_t payload = { (char *) payload_string, strlen(payload_string) };
  mem_t body = { NULL, 0 };

  INITHDR();
  INIT();
  SETOPT(CURLOPT_NOPROGRESS, 1L);
  SETOPT(CURLOPT_VERBOSE, 0L);
  SETOPT(CURLOPT_URL, url);
  SETOPT(CURLOPT_POST, 1L);
  SETOPT(CURLOPT_READFUNCTION, read_callback);
  SETOPT(CURLOPT_READDATA, (void *) &payload);
  SETOPT(CURLOPT_WRITEFUNCTION, write_callback);
  SETOPT(CURLOPT_WRITEDATA, (void *) &body);
  SETHDR("Transfer-Encoding: chunked");
  SETHDR("Expect:");
  SETOPT(CURLOPT_HTTPHEADER, headers);
  PERFORM();

  rc = body.mem;
clean:
  if (rc == NULL && body.mem != NULL)
    free(body.mem);

  ENDHDR();
  END();

  return rc;
}

const char *http_strerror() {
  return error;
}
