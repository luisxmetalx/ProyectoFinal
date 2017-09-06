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
#define MAXSLEEP 10
#define TAMANO 25
#define POSTLEN  512
#define BUFLEN 1024
#define BUFF_DISP 100000

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
      char *file = malloc(BUFF_DISP*sizeof(char *));
      memset(file,0,BUFF_DISP);
      if((rec=recv(sockfd, file, BUFF_DISP,0))>0){
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

  struct NameUSB{
    char* nombre;
    char* direccion_fisica;
    char* direccion_logica;
};
  
  struct NameUSB* nombrados[TAMANO];
  int elementos=0;


  static int JsonEmpty(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
      return 0;
    }
    return -1;
  }

  struct InfoUSB{
    char* nombre;
    char* nodo;
    char* montaje;
    char* sci;
    char* VendoridProduct;
  };
  
  struct InfoUSB* usblista[TAMANO];
  int usbelementos=0;

  char* jsonombrar(const char *loadate) {//jasonombrar
    char *elementolista=malloc(sizeof(char)*(POSTLEN));  
    int r,i;
    jsmn_parser p;
    jsmntok_t t[POSTLEN]; 
    jsmn_init(&p);
    char* nodo=malloc(sizeof(char)*(POSTLEN));
    char* nombre=malloc(sizeof(char)*(POSTLEN));
    char* new_data=malloc(sizeof(char)*(strlen( loadate)-2));
    memset(new_data,0,strlen( loadate)-2);
        int j=0;
        for(int i=0;i<strlen(loadate);i++){
          if(loadate[i]=='{' && j==0){
            while(loadate[i]!='}'){
              new_data[j]=loadate[i];
              i++;
              j++;
            }
            new_data[j]=loadate[i];
          }
        }
    char * re=malloc((j-1)*sizeof(char *));
    memset(re,0,j-1);
    re=new_data;
    r = jsmn_parse(&p,  re, strlen(re), t, POSTLEN);
    printf ("\nprocesando contenido del json.....\n");
    printf("%d,%s",r,re);
    if (r < 0) {
      return NULL;
    }
    if (r < 1 || t[0].type != JSMN_OBJECT) {
      return NULL;
    }
      for (i = 1; i < r; i++) {
        if (jsonequals( new_data, &t[i], "nodo") == 0) {
          sprintf(nodo,"%.*s",t[i+1].end-t[i+1].start-4, new_data + t[i+1].start+2);      
          printf("\n - %s: %.*s\n", "nodo",t[i+1].end-t[i+1].start-4, new_data + t[i+1].start+2);
          i++;
        }
        if (jsonequals( new_data, &t[i], "nombre") == 0) {
          sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start-4, new_data + t[i+1].start+2);      
          printf("\n - %s: %.*s\n", "nombre",t[i+1].end-t[i+1].start-4, new_data + t[i+1].start+2);
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

char* DireccionUSB(char* nombre){
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
char* jsonEscribir(const char *loadate) {
  char *elementolista=malloc(sizeof(char)*(BUFF_DISP)); 
  memset(elementolista,0,BUFF_DISP);
  int r,i;
  jsmn_parser p;
  jsmntok_t t[BUFLEN]; 
  jsmn_init(&p);
  char* nombre_archivo=malloc(sizeof(char)*(BUFLEN));
  char *nombre=malloc(sizeof(char)*(BUFLEN));
  char *solicitud=malloc(sizeof(char)*(BUFLEN));
  char *tamano_contenido=malloc(sizeof(char)*(BUFLEN));
  int tamano;
  char* new_data=malloc(sizeof(char)*(strlen( loadate)-2));
  memset(new_data,0,strlen( loadate)-2);
  int j=0;
  for(int i=0;i<strlen(loadate);i++){
    if(loadate[i]=='{' && j==0){
      while(loadate[i]!='}'){
        new_data[j]=loadate[i];
        i++;
        j++;
      }
      new_data[j]=loadate[i];
    }
  }
  char * re=malloc((j-1)*sizeof(char *));
  memset(re,0,j-1);
  re=new_data;
  r = jsmn_parse(&p,  re, strlen(re), t, 128);
  printf ("\nprocesando contenido del json.....\n");
  printf("json recibido %d:\n %s \n",r,re);
  /* Assume the top-level element is an object */
  if (r < 0) return NULL;
  if (r < 1 || t[0].type != JSMN_OBJECT) return NULL;
  for (i = 1; i < r; i++) {
    if (JsonEmpty( new_data, &t[i], "nombre_archivo") == 0) {
      sprintf(nombre_archivo,"%.*s",t[i+1].end-t[i+1].start,new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "nombre_archivo",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
      sprintf(elementolista,"%s",nombre_archivo); 
    }
  }
  for (i = 1; i < r; i++) {
    if (JsonEmpty( new_data, &t[i], "nombre") == 0) {
      sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "nombre",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
    }
    if (JsonEmpty( new_data, &t[i], "tamano_contenido") == 0) {
      sprintf(tamano_contenido,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "tamano_contenido",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
      sprintf(elementolista,"%s|%s",elementolista,tamano_contenido); 
    }
    if (JsonEmpty( new_data, &t[i], "solicitud") == 0) {
      sprintf(solicitud,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "solicitud",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
      sprintf(elementolista,"%s|%s",elementolista,solicitud); 
    }
  }
  for (i = 1; i < r; i++) {
    if (JsonEmpty( new_data, &t[i], "contenido") == 0) {
      tamano=atoi(tamano_contenido);
      char *contenido=malloc(sizeof(char)*(tamano));
      sprintf(contenido,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "contenido",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
      sprintf(elementolista,"%s|%s",elementolista,contenido); 
    }
  }
  char* direccion=DireccionUSB(nombre);
  if(strstr(direccion, "str_error")!=NULL) return direccion;
  printf("\n%d\n",(int)strlen(direccion) );
  sprintf(elementolista,"%s|%s|%s|",elementolista,nombre,direccion); 
  return elementolista;  
}

char* jsonLeer(const char *loadate) {
  char *elementolista=malloc(sizeof(char)*(BUFF_DISP)); 
  memset(elementolista,0,BUFF_DISP);
  int r,i;
  jsmn_parser p;
  jsmntok_t t[BUFLEN]; 
  jsmn_init(&p);
  char* nombre_archivo=malloc(sizeof(char)*(BUFLEN));
  char* nombre=malloc(sizeof(char)*(BUFLEN));
  char* solicitud=malloc(sizeof(char)*(BUFLEN));
  char* new_data=malloc(sizeof(char)*(strlen( loadate)-2));
  memset(new_data,0,strlen( loadate)-2);
  int j=0;
  for(int i=0;i<strlen(loadate);i++){
    if(loadate[i]=='{' && j==0){
      while(loadate[i]!='}'){
        new_data[j]=loadate[i];
        i++;
        j++;
      }
      new_data[j]=loadate[i];
    }
  }
  char * re=malloc((j-1)*sizeof(char *));
  memset(re,0,j-1);
  re=new_data;
  r = jsmn_parse(&p,  re, strlen(re), t, 128);
  printf ("\nprocesando contenido del json.....\n");
  printf("json recibido %d:\n %s \n",r,re);
  /* Assume the top-level element is an object */
  if (r < 0) return NULL;
  if (r < 1 || t[0].type != JSMN_OBJECT) return NULL;
  for (i = 1; i < r; i++) {
    if (JsonEmpty(new_data, &t[i], "nombre_archivo") == 0) {
      sprintf(nombre_archivo,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "nombre_archivo",t[i+1].end-t[i+1].start, new_data + t[i+1].start);
      i++;
      sprintf(elementolista,"%s",nombre_archivo); 
    }
  }
  for (i = 1; i < r; i++) {
    if (JsonEmpty(new_data, &t[i], "nombre") == 0) {
      sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start,new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "nombre",t[i+1].end-t[i+1].start,new_data + t[i+1].start);
      i++;
    }
    if (JsonEmpty( new_data, &t[i], "solicitud") == 0) {
      sprintf(solicitud,"%.*s",t[i+1].end-t[i+1].start,new_data + t[i+1].start);      
      printf("\n - %s: %.*s", "solicitud",t[i+1].end-t[i+1].start,new_data+ t[i+1].start);
      i++;
      sprintf(elementolista,"%s|%s",elementolista,solicitud); 
    }
  }
  char* direccion=DireccionUSB(nombre);
  if(strstr(direccion, "str_error")!=NULL) return direccion;
  printf("\n%d\n",(int)strlen(direccion) );
  sprintf(elementolista,"%s|%s|%s|",elementolista,nombre,direccion); 
  return elementolista;  
}


int jsonlistar(const char *loadate, int cantparametros, const char *s[]) {//precjson
  int r,i;
  jsmn_parser p;
  jsmntok_t t[POSTLEN]; 
  jsmn_init(&p);
  char* new_data=malloc(sizeof(char)*(strlen( loadate)));
  int j=0;
  for(int i=0;i<strlen(loadate);i++){
    if(loadate[i]=='{' && j==0){
      i++;
      while(loadate[i]!='}'){
        new_data[j]=loadate[i];
        i++;
        j++;
      }
    }
  }
  char * re=malloc((j-1)*sizeof(char *));
  memset(re,0,j-1);
  re=new_data;
  r = jsmn_parse(&p,  re, strlen(re), t, POSTLEN);
  printf ("\nProcesando contenido del json.....\n");
  if (r < 0) {
    printf("Fallido parsing JSON: %d\n", r);
    return 0;
  }
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    printf("Objecto no esperado\n");
    return 0;
  }
  for(int token=0;token < cantparametros; token++){
    for (i = 1; i < r; i++) {
      if (jsonequals(new_data, &t[i], s[token]) == 0) {
        printf("- %s: %.*s\n", s[token],t[i+1].end-t[i+1].start-4,new_data + t[i+1].start+2);
        i++;
      }
    }
  }
  return 1;
}
      
static int Headers_Interator (void *cls, enum MHD_ValueKind kind, const char *key, const char *value){
  printf ("Encabezado %s: %s\n", key, value);
  return MHD_YES;
}

void iterar(struct NameUSB *lista[]){
  printf("\nDISPOSITIVO NOMBRADOS\n");
  if(elementos==0) {
    printf("Lista vacia.No hay dispositivo USB nombrados.\n");
    return;
  }
  for (int i = 0; i < elementos; i++) {     
      printf("dispositivo N %d | nombre:%s,direccion fisica:%s,direccion logica:%s \n", i+1,lista[i]->nombre,lista[i]->direccion_fisica,lista[i]->direccion_logica);
      }
}
      
void iterarlistado(struct InfoUSB *lista[]){
  printf("\nHISTORIAL DE DISPOSITIVOS\n");
  if(usbelementos==0) {
    printf("Lista vacia.No hay dispositivo USB.\n");
    return;
  }
  for (int i = 0; i < usbelementos; i++) {     
    printf("USB N %d | nombre:%s,nodo:%s,montaje:%s,sci:%s,VendoridProduct :%s \n", i+1,lista[i]->nombre,lista[i]->nodo,lista[i]->montaje,lista[i]->sci,lista[i]->VendoridProduct);
  }
}

void ListaUSB(const char *loadate) {
  int r,i;
  jsmn_parser p;
  jsmntok_t t[128]; 
  jsmn_init(&p);
  char* nodo=malloc(sizeof(char)*(10));
  char *nombre=malloc(sizeof(char)*(50));
  char *montaje=malloc(sizeof(char)*(50));
  char *sci=malloc(sizeof(char)*(50));
  char *VendoridProduct=malloc(sizeof(char)*(50));
  char* new_data=malloc(sizeof(char)*(strlen( loadate)-2));
  memset(new_data,0,strlen( loadate)-2);
  int j=0;
  for(int i=0;i<strlen(loadate);i++){
    if(loadate[i]=='{' && j==0){
      while(loadate[i]!='}'){
        new_data[j]=loadate[i];
        i++;
        j++;
      }
      new_data[j]=loadate[i];
    }
  }
  char * re=malloc((j-1)*sizeof(char *));
  memset(re,0,j-1);
  re=new_data;
  r = jsmn_parse(&p,  loadate, strlen(loadate), t, 128);
  if (r < 0) {
    return ;
  }
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    return ;
  }
  for (i = 1; i < r; i++) {
    if (JsonEmpty( loadate, &t[i], "nombre") == 0) {
      sprintf(nombre,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      i++;
    }
    if (JsonEmpty( loadate, &t[i], "nodo") == 0) {
      sprintf(nodo,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      i++;
    }
    if (JsonEmpty( loadate, &t[i], "montaje") == 0) {
      sprintf(montaje,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      i++;
    }  
    if (JsonEmpty( loadate, &t[i], "scsi") == 0) {
      sprintf(sci,"%.*s",t[i+1].end-t[i+1].start,new_data+ t[i+1].start);      
      i++;
    }  
    if (JsonEmpty( loadate, &t[i], "Vendor:idProduct") == 0) {
      sprintf(VendoridProduct,"%.*s",t[i+1].end-t[i+1].start, new_data + t[i+1].start);      
      i++;
    }         
  }
 struct InfoUSB *usb2=malloc(sizeof(struct InfoUSB));
  usb2->nombre=nombre;
  usb2->nodo=nodo;
  usb2->montaje=montaje;
  usb2->sci=sci;
  usb2->VendoridProduct=VendoridProduct;
  usblista[usbelementos]=usb2;
  usbelementos++;     
  return;
}

struct ColaJson{
  char* info;
  char* stringjson;
};

void Split_Request(char* listausb){
  char* usb1=malloc(BUFLEN*sizeof(char*));
  if(strstr(listausb,"},")>0){
    const char delimitadores[2] = "}";
    char *token;
    token = strtok(listausb, delimitadores);
    int i=0;
    while( token != NULL ) {
      sprintf(usb1,"%s}",token);
      if(i>0){
      ListaUSB(usb1+1);
      printf("%s\n", usb1+1);
      }else{
      ListaUSB(usb1);
      printf("%s\n", usb1);
      i++;
    }
      token = strtok(NULL, delimitadores);
      
   }
  }else{
    ListaUSB(listausb);
  }
}

struct InfoUSB* ApodoUSB(char * solicitud){
  char * respx=malloc(8*sizeof(char *));
  char * resp2=malloc(20*sizeof(char *));
    int j=0,z=0,w=0;
    for(int i=0;i<strlen(solicitud);i++){
      if(solicitud[i]=='-' && j==0){
        i++;
        while(solicitud[i]!='-'){
          respx[j]=solicitud[i];
          i++;
          j++;
        }
        z++;
        i++;
      }
      if(z==1){
        resp2[w]=solicitud[i];
        w++;
      }
    }
  char * r=malloc((j-2)*sizeof(char *));
  memset(r,0,j-2);
  char * nombre=malloc((w)*sizeof(char *));
  memset(nombre,0,w-1);
  r=respx;
  nombre=resp2;
  for (int i = 0; i < usbelementos; i++) {   
    if(strstr(r,usblista[i]->nodo )!=NULL){
      usblista[i]->nombre=nombre;
      return  usblista[i];
    } 
  }
  return NULL;
}
      
static int send_response(struct MHD_Connection *connection, const char *page, int statuscod){
    int tmp;
    struct MHD_Response *response;
    response =MHD_create_response_from_buffer (strlen (page), (void *) page,MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;
    MHD_add_response_header(response,"Content-Type","application/json");
    tmp = MHD_queue_response (connection, statuscod, response);
    MHD_destroy_response (response);
    return tmp;
  }
      
int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version,
                                const char *loadate, size_t *loadate_size, void **con_cls){
    char* jsonresp=malloc(BUFF_DISP*sizeof(char *));
    memset(jsonresp,0,BUFF_DISP);
    char* solicitud=malloc(BUFF_DISP*sizeof(char *));
    memset(solicitud,0,BUFF_DISP);
    if (0 == strcmp (method,MHD_HTTP_METHOD_GET)  && !strncasecmp(url, "/listar_dispositivos", 19)){
        sprintf(solicitud, "%s-%s",method,"listar_dispositivos");
        printf ("\nNueva  %s solicitud en  %s con  version %s \n", method, url, version);
        MHD_get_connection_values (connection, MHD_HEADER_KIND, Headers_Interator,NULL);
        char *resp=init_cliente(solicitud);
        if(strstr(resp, "str_error")!=NULL ){
          sprintf(jsonresp,"{\"solicitud\": \"listar_dispositivos\", \n"
                            "\"status\": \"-1 \", %s}",resp);
          return send_response (connection, jsonresp,400); 
        }else{
          if(strlen(resp)<=1){
            sprintf(jsonresp,"{\"solicitud\": \"listar_dispositivos\", \"dispositivos\": [%s ], \n"
                            "\"status\": \"0\", \"str_error\" :\"no existen dispositivos actualmente conectados\" }",resp);
            return send_response (connection, jsonresp, MHD_HTTP_OK); 
          }else{
            sprintf(jsonresp,"{\"solicitud\": \"listar_dispositivos\", \"dispositivos\": [%s ], \n"
                            "\"status\": \"0\", \"str_error\" : 0}",resp);
            Split_Request(resp);
            iterarlistado(usblista);
            return send_response (connection, jsonresp, MHD_HTTP_OK);
          }
        }
    }
    else if (0 == strcmp (method, "POST") && !strncasecmp(url, "/nombrar_dispositivo", 17)){
    struct ColaJson *cola = NULL;
    cola = (struct ColaJson*)*con_cls;
    if(cola == NULL) {
      cola = malloc(sizeof(struct ColaJson));
      cola->info = 0;
      *con_cls = cola;
      return MHD_YES;
    }
    if(*loadate_size != 0) {
      cola->stringjson= malloc(*loadate_size + 1);
      strncpy(cola->stringjson, loadate, *loadate_size);
      *loadate_size = 0;
      return MHD_YES;
    }else {
      printf ("\nNueva  %s solicitud en  %s con  version %s \n", method, url, version);
      iterar(nombrados);
      MHD_get_connection_values (connection, MHD_HEADER_KIND, Headers_Interator,NULL);
      printf ("obteniendo json con información............\n");
      sprintf(solicitud, "%s-%s","GET","listar_dispositivos");
      char *respuestadaemon=init_cliente(solicitud);
      if(strstr(respuestadaemon, "str_error")==NULL && strlen(respuestadaemon)>=2){
        Split_Request(respuestadaemon);
        char * resp=jsonombrar(cola->stringjson);
        if(resp!=NULL){
          char *nuevo=malloc(BUFF_DISP*sizeof(char *));
          sprintf(nuevo, "%s-%s","obtenerdireccion",resp);
          struct InfoUSB *usb2=malloc(sizeof(struct InfoUSB));
          usb2=ApodoUSB(nuevo);
          nombrados[elementos-1]->direccion_logica=usb2->montaje;
          iterar(nombrados);
          sprintf(jsonresp,"{\"solicitud\": \"nombrar_dispositivo\", \"nombre\":\"%s\" , \n"
                        "\"nodo\":\"%s\" ,\"status\": \"0\", \"str_error\" : 0}",nombrados[elementos-1]->nombre,nombrados[elementos-1]->direccion_fisica);
          return send_response (connection, jsonresp, MHD_HTTP_OK);
        }else{
          sprintf(jsonresp,"{\"solicitud\": \"nombrar_dispositivo\", \n"
                      "\"status\": \"-1 \",\"str_error\" :\"error : formato json erroneo \" }");
          return send_response (connection, jsonresp,400); 
        }
      }else{
        if(strlen(respuestadaemon)>=2){
          sprintf(jsonresp,"{\"solicitud\": \"nombrar_dispositivo\", \n"
                      "\"status\": \"-1 \", %s}",respuestadaemon);
          return send_response (connection, jsonresp,400); 
        }else{
          sprintf(jsonresp,"{\"solicitud\": \"nombrar_dispositivo\", \n"
                      "\"status\": \"-1 \",\"str_error\" :\"no existen dispositivos actualmente conectados\" }");
          return send_response (connection, jsonresp,400); 
        }
      }   
    }
  }  else if (0 == strcmp (method, "GET") && !strncasecmp(url, "/leer_archivo", 13)){
    struct ColaJson *cola = NULL;
    cola = (struct ColaJson*)*con_cls;
    if(cola == NULL) {
      cola = malloc(sizeof(struct ColaJson));
      cola->info = 0;
      *con_cls = cola;
      return MHD_YES;
    }
    if(*loadate_size != 0) {
      cola->stringjson= malloc(*loadate_size + 1);
      strncpy(cola->stringjson, loadate, *loadate_size);
      *loadate_size = 0;
      return MHD_YES;
    }else {
      printf ("\nNueva  %s solicitud en  %s con  version %s \n", method, url, version);
      MHD_get_connection_values (connection, MHD_HEADER_KIND, Headers_Interator,NULL);
      printf ("obteniendo json con información........\n");
      char * solicitud=jsonEscribir(cola->stringjson);
      if(strstr(solicitud, "str_error")==NULL){
        sprintf(jsonresp,"{\"solicitud\": \"escribir_dispositivo\", \n"
                      "\"status\": \"-1 \", %s }",solicitud);
          return send_response (connection, jsonresp,404); 
      }
      printf ("\n\ncompleto procesamiento json.... enviando al daemon........\n");
      char *respuestadaemon=init_cliente(solicitud);
      if(strstr(respuestadaemon, "str_error")==NULL){
        sprintf(jsonresp,"%s,\"status\": \"0\", \"str_error\" : 0}",respuestadaemon);
        return send_response (connection, jsonresp, MHD_HTTP_OK);
        }else{
          sprintf(jsonresp,"{\"solicitud\": \"escribir_dispositivo\", \n"
                      "\"status\": \"-1 \",%s }",respuestadaemon);
          return send_response (connection, jsonresp,400); 
        }
    }
  }
  else if (0 == strcmp (method, "POST") && !strncasecmp(url, "/escribir_archivo", 17)){
    struct ColaJson *cola = NULL;
    cola = (struct ColaJson*)*con_cls;
    if(cola == NULL) {
      cola = malloc(sizeof(struct ColaJson));
      cola->info = 0;
      *con_cls = cola;
      return MHD_YES;
    }
    if(*loadate_size != 0) {
      cola->stringjson= malloc(*loadate_size + 1);
      strncpy(cola->stringjson, loadate, *loadate_size);
      *loadate_size = 0;
      return MHD_YES;
    }else {
      printf ("\nNueva  %s solicitud en  %s con  version %s \n", method, url, version);
      MHD_get_connection_values (connection, MHD_HEADER_KIND, Headers_Interator,NULL);
      printf ("obteniendo json con información........\n");
      char * solicitud=jsonEscribir(cola->stringjson);
      if(strstr(solicitud, "str_error")==NULL){
        sprintf(jsonresp,"{\"solicitud\": \"escribir_dispositivo\", \n"
                      "\"status\": \"-1 \", %s }",solicitud);
          return send_response (connection, jsonresp,404); 
      }
      printf ("\n\ncompleto procesamiento json.... enviando al daemon........\n");
      char *respuestadaemon=init_cliente(solicitud);
      if(strstr(respuestadaemon, "str_error")==NULL){
        sprintf(jsonresp,"%s,\"status\": \"0\", \"str_error\" : 0}",respuestadaemon);
        return send_response (connection, jsonresp, MHD_HTTP_OK);
        }else{
          sprintf(jsonresp,"{\"solicitud\": \"escribir_dispositivo\", \n"
                      "\"status\": \"-1 \",%s }",respuestadaemon);
          return send_response (connection, jsonresp,400); 
        }
    }
  }
  else{
        printf ("\nNueva  %s solicitud en  %s con  version %s \n", method, url, version);
        MHD_get_connection_values (connection, MHD_HEADER_KIND, Headers_Interator,NULL);
        char * resp="\"str_error\":\"ERROR 404 Not Found\"";
        sprintf(jsonresp,"{\"solicitud\": \"%s\", \n"
                   "\"status\": \" -1\", %s }",url,resp);
        int r=send_response (connection, jsonresp,404); 
        printf("%d \n",r) ;
        return r;
    }
    return MHD_NO;
}

int main (int argc, char *argv[]){
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