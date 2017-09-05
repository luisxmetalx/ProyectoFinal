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
  


static int jsonequals(const char *json, jsmntok_t *tok, const char *s) {
  if ((int) strlen(s) == tok->end - tok->start-4 && strncmp(json + tok->start+2, s, tok->end - tok->start-4) == 0) {
    return 0;
  }
  return -1;
}

static int jsonlimpio(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

char* jsonombrar(const char *upload_data) {//jasonombrar
  char *elementolista=malloc(sizeof(char)*(POSTBUFFERSIZE));  
  int r,i;
  jsmn_parser p;
  jsmntok_t t[POSTBUFFERSIZE]; 
  jsmn_init(&p);
  char* nodo=malloc(sizeof(char)*(POSTBUFFERSIZE));
  char* nombre=malloc(sizeof(char)*(POSTBUFFERSIZE));
  char* upload_datas=malloc(sizeof(char)*(strlen( upload_data)-2));
  memset(upload_datas,0,strlen( upload_data)-2);
      int j=0;
      for(int i=0;i<strlen(upload_data);i++){
        if(upload_data[i]=='{' && j==0){
          while(upload_data[i]!='}'){
            upload_datas[j]=upload_data[i];
            i++;
            j++;
          }
          upload_datas[j]=upload_data[i];
        }
      }
  char * re=malloc((j-1)*sizeof(char *));
  memset(re,0,j-1);
  re=upload_datas;
  r = jsmn_parse(&p,  re, strlen(re), t, POSTBUFFERSIZE);
  printf("%d,%s",r,re);
  if (r < 0) {
    return NULL;
  }
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    return NULL;
  }
    for (i = 1; i < r; i++) {
      if (jsonequals( upload_datas, &t[i], "nodo") == 0) {
        sprintf(nodo,"%.*s",t[i+1].end-t[i+1].start-4, upload_datas + t[i+1].start+2);      
        printf("\n - %s: %.*s\n", "nodo",t[i+1].end-t[i+1].start-4, upload_datas + t[i+1].start+2);
        i++;
      }
      if (jsonequals( upload_datas, &t[i], "nombre") == 0) {
        sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start-4, upload_datas + t[i+1].start+2);      
        printf("\n - %s: %.*s\n", "nombre",t[i+1].end-t[i+1].start-4, upload_datas + t[i+1].start+2);
        i++;
      }      
    }
  struct NameUSB *usb=malloc(sizeof(struct NameUSB));
  usb->nombre=nombre;
  usb->direccion_fisica=nodo;
  nombrados[elementos]=usb;
  elementos++;     
  sprintf(elementolista,"%s-%s",usb->direccion_fisica, usb->nombre); 
  return elementolista;
}
// faltan funciones de parsing de JSON
char* obtenerDireccion(char* nombre){
  if(elementos==0) {
    printf("No existe algun dispositivo nombrado.\n");
    return"\"str_error\":\"ERROR: No existe algun dispositivo nombrado.\"\n";
  }
  for (int i = 0; i < elementos; i++) {      
    if(strstr(nombrados[i]->nombre,nombre )!=NULL){
      return  nombrados[i]->direccion_logica;
      } 
  } 
  printf("No existe algun dispositivo nombrado con ese nombre.\n");
  return"\"str_error\":\"ERROR:No existe algun dispositivo nombrado con ese nombre. \""; 
}

char* jsonEscribir(const char *upload_data) {
char *elementolista=malloc(sizeof(char)*(BUFFERING)); 
memset(elementolista,0,BUFFERING);
int r,i;
jsmn_parser p;
jsmntok_t t[BUFLEN]; 
jsmn_init(&p);
char* nombre_archivo=malloc(sizeof(char)*(BUFLEN));
char *nombre=malloc(sizeof(char)*(BUFLEN));
char *solicitud=malloc(sizeof(char)*(BUFLEN));
char *tamano_contenido=malloc(sizeof(char)*(BUFLEN));
int tamano;
char* upload_datas=malloc(sizeof(char)*(strlen( upload_data)-2));
memset(upload_datas,0,strlen( upload_data)-2);
int j=0;
for(int i=0;i<strlen(upload_data);i++){
  if(upload_data[i]=='{' && j==0){
    while(upload_data[i]!='}'){
      upload_datas[j]=upload_data[i];
      i++;
      j++;
    }
    upload_datas[j]=upload_data[i];
  }
}
char * re=malloc((j-1)*sizeof(char *));
memset(re,0,j-1);
re=upload_datas;
r = jsmn_parse(&p,  re, strlen(re), t, 128);
printf ("\nprocesando contenido del json.....\n");
printf("json recibido %d:\n %s \n",r,re);
/* Assume the top-level element is an object */
if (r < 0) return NULL;
if (r < 1 || t[0].type != JSMN_OBJECT) return NULL;
for (i = 1; i < r; i++) {
  if (jsonlimpio( upload_datas, &t[i], "nombre_archivo") == 0) {
    sprintf(nombre_archivo,"%.*s",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);      
    printf("\n - %s: %.*s", "nombre_archivo",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);
    i++;
    sprintf(elementolista,"%s",nombre_archivo); 
  }
}
for (i = 1; i < r; i++) {
  if (jsonlimpio( upload_datas, &t[i], "nombre") == 0) {
    sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);      
    printf("\n - %s: %.*s", "nombre",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);
    i++;
  }
  if (jsonlimpio( upload_datas, &t[i], "tamano_contenido") == 0) {
    sprintf(tamano_contenido,"%.*s",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);      
    printf("\n - %s: %.*s", "tamano_contenido",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);
    i++;
    sprintf(elementolista,"%s|%s",elementolista,tamano_contenido); 
  }
  if (jsonlimpio( upload_datas, &t[i], "solicitud") == 0) {
    sprintf(solicitud,"%.*s",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);      
    printf("\n - %s: %.*s", "solicitud",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);
    i++;
    sprintf(elementolista,"%s|%s",elementolista,solicitud); 
  }
}
for (i = 1; i < r; i++) {
  if (jsonlimpio( upload_datas, &t[i], "contenido") == 0) {
    tamano=atoi(tamano_contenido);
    char *contenido=malloc(sizeof(char)*(tamano));
    sprintf(contenido,"%.*s",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);      
    //printf("\n - %s: %.*s", "contenido",t[i+1].end-t[i+1].start, upload_datas + t[i+1].start);
    i++;
    sprintf(elementolista,"%s|%s",elementolista,contenido); 
  }
}
char* direccion=obtenerDireccion(nombre);
if(strstr(direccion, "str_error")!=NULL) return direccion;
printf("\n%d\n",(int)strlen(direccion) );
sprintf(elementolista,"%s|%s|%s|",elementolista,nombre,direccion); 
return elementolista;  
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