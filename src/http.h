#ifndef _HTTP_H_
#define _HTTP_H_

typedef struct {
  char *mem;
  unsigned int size;
} mem_t;

typedef struct {
  char name[1024];
  char value[1024];
} hdr_t;

char *http_get(const char *url, hdr_t *headers, unsigned int headers_len);
char *http_post(const char *url, const char *payload);
const char *http_strerror(void);

#endif // _HTTP_H_
