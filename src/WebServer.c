#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jsmn.h"
#include <sys/stat.h>
#include <stddef.h>        
#include <unistd.h>             
#include <netdb.h> 
#include <errno.h> 
#include <syslog.h> 
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#define MAXSLEEP        1
#define TAMANO 25
#define POSTBUFFERSIZE  512
#define BUFLEN 1024
#define BUFFERING 100000

struct USBlista{
  char* nombre;
  char* nodo;
  char* montaje;
  char* sci;
  char* VendoridProduct;
};
struct manejoColaJson{
  char* info;
  char* stringjson;
};
struct NameUSB{
    char* nombre;
    char* direccion_fisica;
    char* direccion_logica;
};
  
  struct NameUSB* nombrados[TAMANO];
  int elementos=0;

  struct USBlista* usblista[TAMANO];  
  int usbelementos=0;


static int send_page (struct MHD_Connection *connection, const char *page)
{
int ret;
struct MHD_Response *response;


response = MHD_create_response_from_buffer (strlen (page), (void *) page,MHD_RESPMEM_PERSISTENT);
if (!response)
  return MHD_NO;

ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
MHD_destroy_response (response);

return ret;
}


static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
      const char *filename, const char *content_type,
      const char *transfer_encoding, const char *data, uint64_t off,
      size_t size)
{
struct connection_info_struct *con_info = coninfo_cls;

if (0 == strcmp (key, "name"))
  {
  if ((size > 0) && (size <= MAXNAMESIZE))
    {
    char *answerstring;
    answerstring = malloc (MAXANSWERSIZE);
    if (!answerstring)
      return MHD_NO;

    snprintf (answerstring, MAXANSWERSIZE, greetingpage, data);
    con_info->answerstring = answerstring;
    }
  else
    con_info->answerstring = NULL;

  return MHD_NO;
  }

return MHD_YES;
}

static void request_completed (void *cls, struct MHD_Connection *connection,
         void **con_cls, enum MHD_RequestTerminationCode toe)
{
struct connection_info_struct *con_info = *con_cls;

if (NULL == con_info)
  return;

if (con_info->connectiontype == POST)
  {
  MHD_destroy_post_processor (con_info->postprocessor);
  if (con_info->answerstring)
    free (con_info->answerstring);
  }

free (con_info);
*con_cls = NULL;
}


static int answer_to_connection (void *cls, struct MHD_Connection *connection,
          const char *url, const char *method,
          const char *version, const char *upload_data,
          size_t *upload_data_size, void **con_cls)
{
if (NULL == *con_cls)
  {
  struct connection_info_struct *con_info;

  con_info = malloc (sizeof (struct connection_info_struct));
  if (NULL == con_info)
    return MHD_NO;
  con_info->answerstring = NULL;

  if (0 == strcmp (method, "POST"))
    {
    con_info->postprocessor =
      MHD_create_post_processor (connection, POSTBUFFERSIZE,
                   iterate_post, (void *) con_info);

    if (NULL == con_info->postprocessor)
      {
      free (con_info);
      return MHD_NO;
      }

    con_info->connectiontype = POST;
    }
  else
    con_info->connectiontype = GET;

  *con_cls = (void *) con_info;

  return MHD_YES;
  }

if (0 == strcmp (method, "GET"))
  {
  return send_page (connection, askpage);
  }

if (0 == strcmp (method, "POST"))
  {
  struct connection_info_struct *con_info = *con_cls;

  if (*upload_data_size != 0)
    {
    MHD_post_process (con_info->postprocessor, upload_data,
              *upload_data_size);
    *upload_data_size = 0;

    return MHD_YES;
    }
  else if (NULL != con_info->answerstring)
    return send_page (connection, con_info->answerstring);
  }

return send_page (connection, errorpage);
}

int main (int argc, char *argv[])
{
 if(argc != 2){
     printf("Uso: ./bin/WebServer <puerto>\n");
     exit(-1);
   }
 int puerto = atoi(argv[1]);
 struct MHD_Daemon *daemon;

 daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, puerto, NULL, NULL,
                          &answer_to_connection, NULL, MHD_OPTION_END);
 if (NULL == daemon)
     return 1;

 getchar ();

 MHD_stop_daemon (daemon);

 return 0;
}