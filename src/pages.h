#ifndef PAGES_H
#define PAGES_H

#include <Arduino.h>
#include <WiFiNINA.h>

#define DOCTYPE_HTML4 \
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" " \
  "\"http://www.w3.org/TR/html4/strict.dtd\">"

#define FOOTER  "<p><i>IncubatorServer " __DATE__ " " __TIME__ "</i></p>"

extern const char msg404[];
extern const char msgWelcome[];

enum HttpCodes {
  HTTP_200_OK,
  HTTP_404_NOT_FOUND
};

extern const char * HTTP_CODES[];

void sendPage(
  WiFiClient & client, 
  int code, 
  const char * type, 
  const char * message
);

#endif


