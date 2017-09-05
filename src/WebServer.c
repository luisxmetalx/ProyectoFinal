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


  int reconnect( int domain, int type, int protocol,  const struct sockaddr *addr, socklen_t alen){
    int fd; 
    for (int numsec = 1; numsec <= MAXSLEEP; numsec++) { 
      if (( fd = socket( domain, type, protocol)) < 0)  return(-1); 
      if (connect( fd, addr, alen) == 0) return(fd); 
      close(fd);  
      sleep(1);
    } 
    return(-1); 
  }


char* init_cliente(char *request){ 
    int sockfd,rec; 
    int puerto =8888;
    struct sockaddr_in socketcliente;
    memset(&socketcliente, 0, sizeof(socketcliente)); 
    socketcliente.sin_family = AF_INET;
    socketcliente.sin_port = htons(puerto);
    socketcliente.sin_addr.s_addr = inet_addr("127.0.0.1") ; 
    if (( sockfd = reconnect( socketcliente.sin_family, SOCK_STREAM, 0, (struct sockaddr *)&socketcliente, sizeof(socketcliente))) < 0) { 
      printf("fallo conexion con el proceso daemon .\nPuede que el puerto solicitado este ya ocupado, vuelva a ejecutar el daemon\"\n"); 
      return "\"str_error\":\"ERROR:fallo conexion con el proceso daemon. Puede que el puerto solicitado este ya ocupado, vuelva a ejecutar el daemon\"\n";
    } 
    if(strstr(request, "escribir_archivo")!=NULL){
      send(sockfd,"escribir_archivo",BUFLEN,0);
      sleep(1);
      send(sockfd,request,strlen(request),0);
      printf("Solicitud enviada proceso daemon:\n ");
      printf("Procesando respuesta daemon\n");
      sleep(1);
      char *file = malloc(BUFLEN*sizeof(char *));
      memset(file,0,BUFLEN);
      if((rec=recv(sockfd, file,BUFLEN,0))>0){
        if (strstr(file, "ERROR") != NULL) {
          printf("ERROR: respuesta daemon fallida\n");
          close(sockfd);
          return "\"str_error\":\"ERROR: respuesta deamon fallida\"\n";
        }
        printf("respuesta del daemonUSB:\n %s\n",file);
        close(sockfd);
        return file;
      }
    }else{
      send(sockfd,request,BUFLEN,0);
      printf("Solicitud enviada proceso daemon:\n %s \n",request);
      printf("Procesando respuesta daemon\n");
      char *file = malloc(BUFFERING*sizeof(char *));
      memset(file,0,BUFFERING);
      if((rec=recv(sockfd, file, BUFFERING,0))>0){
        if (strstr(file, "ERROR") != NULL) {
          printf("ERROR: respuesta daemon fallida\n");
          close(sockfd);
          return "\"str_error\":\"ERROR:respuesta daemon fallida\"\n";
        }
        printf("respuesta del daemon:\n %s\n",file);
        close(sockfd);
        return file;
      }  
    }
    close(sockfd);
    return "\"str_error\":\"ERROR:respuesta daemon fallida\"\n";
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