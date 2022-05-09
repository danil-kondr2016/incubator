#include "pages.h"
#include <stdio.h>
#include <string.h>

const char * HTTP_CODES[] = {
  "HTTP/1.1 200 OK",
  "HTTP/1.1 404 Not Found"
};

const char msg404[] =
  DOCTYPE_HTML4 "\r\n"
  "<html>\r\n"
  "<head><title>Error</title></head>\r\n"
  "<body>\r\n"
  "<h1 align=\"center\">Error 404: page not found</h1>\r\n"
  "<hr>\r\n" FOOTER "\r\n"
  "</body>\r\n"
  "</html>\r\n";

const char msgWelcome[] = 
  DOCTYPE_HTML4 "\r\n"
  "<html>\r\n"
  "<head><title>Incubator</title></head>\r\n"
  "<body>\r\n"
  "<h1 align=\"center\">The IoT-incubator</h1>\r\n"
  "<p align=\"center\">Created by Kondratenko Daniel in 2021</p>\r\n"
  "<hr>\r\n" FOOTER "\r\n"
  "</body>\r\n"
  "</html>\r\n";

void sendPage(
  WiFiClient & client,
  int code,
  const char * type,
  const char * message
)
{
  char content_type_buf[256];
  char content_length_buf[256];

  size_t content_len = strlen(message);

  sprintf(content_type_buf, "Content-Type: %s", type);
  sprintf(content_length_buf, "Content-Length: %ld", content_len + 2);

  client.println(HTTP_CODES[code]);
  client.println(content_type_buf);
  client.println(content_length_buf);
  client.println();
  client.println(message);
}
