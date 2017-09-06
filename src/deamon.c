#include <sys/types.h>        
#include <sys/stat.h>
#include <stdio.h>          
#include <stdlib.h>            
#include <stddef.h>         
#include <string.h>            
#include <unistd.h>                  
#include <netdb.h> 
#include <errno.h> 
#include <syslog.h> 
#include <sys/socket.h> 
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <libudev.h>
#include <mntent.h> 
#define BUFF_DISP 100000
#define QLEN 50 
#define BUFLEN 1024

#ifndef HOST_NAME_MAX 
#define HOST_NAME_MAX 256 
#endif	

int clienteFd;  
int SkFd;
int fd;

struct udev_device* obtenerHijo(struct udev* udev, struct udev_device* padre, const char* subsistema){
	struct udev_device* son = NULL;
	struct udev_enumerate *enumerar = udev_enumerate_new(udev);

	udev_enumerate_add_match_parent(enumerar, padre);
	udev_enumerate_add_match_subsystem(enumerar, subsistema);
	udev_enumerate_scan_devices(enumerar);

	struct udev_list_entry *dispositivos = udev_enumerate_get_list_entry(enumerar);
	struct udev_list_entry *entrada;
	udev_list_entry_foreach(entrada, dispositivos){
		const char *ruta = udev_list_entry_get_name(entrada);
		son = udev_device_new_from_syspath(udev, ruta);
		break;
	}
	udev_enumerate_unref(enumerar); 
	return son;
}

int InicializarServidor(int type, const struct sockaddr *addr, socklen_t alen, int qlen){
	int err = 0;
	if((fd = socket(addr->sa_family, type, 0)) < 0)
		return -1;
	if(bind(fd, addr, alen) < 0)
		goto errout;
	if(type == SOCK_STREAM || type == SOCK_SEQPACKET){
		if(listen(fd, QLEN) < 0)
			goto errout;
	}
	return fd;
errout:
	err = errno;
	close(fd);
	errno = err;
	return (-1);
}

const char* AddressDisp(const char *direccion_fisica){
	FILE *fp;
	struct mntent *fs;
	/*function opens the filesystem description file filename and returns a file pointer*/
	fp = setmntent("/etc/mtab", "r");
	if (fp == NULL) {
		return "\"str_error\":\"ERROR: Al intentar abrir el fichero: /etc/mtab que contiene la direccion logico de los disp USB\"";
	}
	/* que leerá UNA linea del mtab, y les devolverá una estructura:*/
	while ((fs = getmntent(fp)) != NULL){
		/* resulta que direccion_fisica no contiene un numero al final que indica la particion correspondiente
		en caso de solo poseer una sola particion posee el numero 1 (esto es lo mas comun para un dispositivo usb)*/
		if(strstr(fs->mnt_fsname,direccion_fisica)>0){
			endmntent(fp);
			return fs->mnt_dir;
		}
	}
	endmntent(fp);
	return  "no se encuentra montado dicho dispositivo";
}

int escribir_archivo(char* direccion, char* nombre_archivo, int tamano, char* contenido){
	printf("%s-%s\n",direccion,nombre_archivo );
	char *resultado=malloc((strlen(direccion)+strlen(nombre_archivo)+1)* sizeof(char*));
	  memset(resultado,0,strlen(direccion)+strlen(nombre_archivo)+1);
	sprintf(resultado,"%s/%s", direccion,nombre_archivo);
	printf("\n%s\n",resultado );
    FILE* fichero;
    fichero = fopen(resultado, "w");
    if(fichero<=0){
    	return 0;
    }
    fputs(contenido, fichero);
    fclose(fichero);
    return 1;
}

char* leer_archivo(char* direccion, char* nombre_archivo){
	FILE *archivo;
	int caracter;
	char resultado[1000];
	char* texto_final=NULL;
	sprintf(resultado,"%s/%s", direccion,nombre_archivo);
	archivo = fopen(resultado,"r");
	if (archivo == NULL){
            printf("\nError de apertura del archivo. \n\n");
    }else{
        while((caracter = fgetc(archivo)) != EOF) sprintf(texto_final,"%s%c",texto_final,caracter);
	}
    fclose(archivo);
    return texto_final;
}

char* ListarDispAlmMasivo(struct udev* udev){

	struct udev_enumerate* enumerar = udev_enumerate_new(udev);

	//Buscamos los dispositivos USB del tipo SCSI (MASS STORAGE)
	udev_enumerate_add_match_subsystem(enumerar, "scsi");
	udev_enumerate_add_match_property(enumerar, "DEVTYPE", "scsi_device");
	udev_enumerate_scan_devices(enumerar);
	
	//Obtenemos los dispositivos con dichas caracteristicas
	struct udev_list_entry *dispositivos = udev_enumerate_get_list_entry(enumerar);
	struct udev_list_entry *entrada;

	//Recorremos la lista obtenida
	
	char *lista = (char *)malloc(BUFF_DISP);
	int n=0;
	udev_list_entry_foreach(entrada, dispositivos) {
		char *concat_str = (char *)malloc(BUFF_DISP);
		const char* ruta = udev_list_entry_get_name(entrada);
		struct udev_device* scsi = udev_device_new_from_syspath(udev, ruta);
		
		//obtenemos la información pertinente del dispositivo
		struct udev_device* block = obtenerHijo(udev, scsi, "block");
		struct udev_device* scsi_disk = obtenerHijo(udev, scsi, "scsi_disk");

		struct udev_device* usb= udev_device_get_parent_with_subsystem_devtype(scsi, "usb", "usb_device");
		
		if (block && scsi_disk && usb){
			const char *nodo=udev_device_get_devnode(block);
			const char * validarerror=AddressDisp(nodo);
			if(strstr(validarerror, "str_error")!=NULL ){
				return (char *)validarerror;
			}
			n=sprintf(concat_str, "{\"nodo\":\"%s\", \"nombre\":\" \",\"montaje\":\"%s\",\"Vendor:idProduct\":\"%s:%s\",\"scsi\":\"%s\"}\n", 
				nodo,
				AddressDisp(nodo),
				udev_device_get_sysattr_value(usb, "idVendor"),
				udev_device_get_sysattr_value(usb, "idProduct"),
				udev_device_get_sysattr_value(scsi, "vendor"));
			if(strstr(lista, "nodo")!=NULL){
				char *copia = (char *)malloc(BUFF_DISP);
				sprintf(copia, "%s",lista);
				sprintf(lista, "%s,%s",copia,concat_str);
			}else{
				sprintf(lista, "%s",concat_str);
			}
		}
		if (block) udev_device_unref(block);
		if (scsi_disk) udev_device_unref(scsi_disk);
		udev_device_unref(scsi);
		//validar para mas de dos dispositivos con contatenacion
		
	//		concat_str=NULL;
	}
	if(n==0) lista=" ";
	udev_enumerate_unref(enumerar);
	return lista;

}
//funcion que nos da a conocer la estructura de mntent
void mntent(const struct mntent *fs){
	printf("nodo :%s \n direccion logica :%s \n %s \n %s \n %d \n %d\n",
		fs->mnt_fsname,  /* name of mounted filesystem(es el nodo del dispositivo) */
		fs->mnt_dir,    /* filesystem path prefix (el directorio donde está montado.)*/
		fs->mnt_type,	/* mount type  */
		fs->mnt_opts,	/* mount options  */
		fs->mnt_freq,	/* dump frequency in days */
		fs->mnt_passno);	/* pass number on parallel fsck */
}


char* Disp(char *direccion_fisica){
	FILE *fp;
	struct mntent *fs;
	/*funcion que habre los fileSystem*/
	fp = setmntent("/etc/mtab", "r");
	if (fp == NULL) {
		return "\"str_error\":\"ERROR: Al intentar abrir el fichero: /etc/mtab que contiene la direccion logico de los disp USB\"";
	}
	/* que leerá UNA linea del mtab, y nos devolverá un tipo struc:*/
	while ((fs = getmntent(fp)) != NULL){
		/* resulta que direccion fisica no contiene un numero al final que indica la particion correspondiente*/
		if(strstr(fs->mnt_fsname,direccion_fisica)>0){
			endmntent(fp);
			return(char*) fs->mnt_dir;
		}
	}
	endmntent(fp);
	return  "no se encuentra montado dicho dispositivo";
}
char* Split_Writing(char* solicitud){
	char* lista[6];
   const char delimitadores[2] = "|";
   char *token;
   token = strtok(solicitud, delimitadores);
   int i=0;
   while( token != NULL ) {
	   lista[i]=token;
	   printf("%d:%s\n",i,token );
	   i++;
	 token = strtok(NULL, delimitadores);
   }
   escribir_archivo(lista[5],lista[0],atoi(lista[1]),lista[3]);
   char *concat_str = malloc(BUFLEN* sizeof(char*));
   sprintf(concat_str, "{\"solicitud\":\"%s\", \"nombre\":\" %s\",\"nombre_archivo\":\"%s\",",lista[2],lista[4],lista[0]); 
   return concat_str;
}

char* Split_Reading(char* solicitud){
	char* lista[6];
    const char delimitadores[2] = "|";
	char *token;
	char *file;
    token = strtok(solicitud, delimitadores);
    int i=0;
    while( token != NULL ) {
	   lista[i]=token;
	   printf("%d:%s\n",i,token );
	   i++;
	   token = strtok(NULL, delimitadores);
    }
    file=leer_archivo(lista[0],lista[1]);
    char *concat_str = malloc(BUFLEN* sizeof(char*));
    sprintf(concat_str, "{\"solicitud\":\"%s\", \"nombre\":\" %s\",\"nombre_archivo\":\"%s\",\"contenido\":\"%s\"",lista[2],lista[4],lista[0],file); 
    return concat_str;
}

void ListenRequestClient(){ 
	int  n,puerto = 8888;
	char *host; 
	if (( n = sysconf(_SC_HOST_NAME_MAX)) < 0) n = HOST_NAME_MAX; /* best guess */ 
	if ((host = malloc(n)) == NULL) printf(" malloc error"); 
	if (gethostname( host, n) < 0) 		//Obtenemos nombre del host
		printf(" gethostname error"); 
	//Direccion del servidor
	struct sockaddr_in direccion_servidor;
	memset(&direccion_servidor, 0, sizeof(direccion_servidor));	//ponemos en 0 la estructura direccion_servidor

	//llenamos los campos
	direccion_servidor.sin_family = AF_INET;		//IPv4
	direccion_servidor.sin_port = htons(puerto);		//Convertimos el numero de puerto al endianness de la red
	direccion_servidor.sin_addr.s_addr = inet_addr("127.0.0.1") ;	//Nos vinculamos a la interface localhost o podemos usar INADDR_ANY para ligarnos A TODAS las interfaces

	if( (SkFd = InicializarServidor(SOCK_STREAM, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor), 1000)) < 0){	//Hasta 1000 solicitudes en cola 
		printf("existe un proceso ya ejecutanse. eliminar proceso daemonUSB\n");	
	}		

	while(1){
		//Ciclo para enviar y recibir mensajes
		if (( clienteFd = accept( SkFd, NULL, NULL)) < 0) { 		//Aceptamos una conexion
			close(clienteFd);
			continue;
		} 
	    char *solicitud = malloc(BUFLEN*sizeof(char *));
		recv(clienteFd, solicitud, BUFLEN, 0);
	  	//tratamiento tipo de solicitud
	  	if ((strstr(solicitud, "GET") != NULL) && (strstr(solicitud, "listar_dispositivos") != NULL)) {
  			//solicitud : GET - listar_dispositivo
  			struct udev *udeva;
			udeva = udev_new();
			char* lista=ListarDispAlmMasivo(udeva);
		    send(clienteFd,lista,strlen(lista),0);
		    close(clienteFd);
		}else if((strstr(solicitud, "escribir_archivo") != NULL) ){
			char* cadenaLarga=malloc(BUFF_DISP*sizeof(char *));
			 memset(cadenaLarga,0,BUFF_DISP);
			recv(clienteFd, cadenaLarga,BUFF_DISP, 0);
			char* respuesta=malloc(BUFLEN*sizeof(char *));
			 memset(respuesta,0,BUFLEN);
			respuesta=Split_Writing(cadenaLarga);
			send(clienteFd,respuesta,BUFLEN,0);
			close(clienteFd);
		}
		else if((strstr(solicitud, "leer_archivo") != NULL) ){
			char* cadenaLarga=malloc(BUFF_DISP*sizeof(char *));
			 memset(cadenaLarga,0,BUFF_DISP);
			recv(clienteFd, cadenaLarga,BUFF_DISP, 0);
			char* respuesta=malloc(BUFLEN*sizeof(char *));
			 memset(respuesta,0,BUFLEN);
			respuesta=Split_Reading(cadenaLarga);
			send(clienteFd,respuesta,BUFLEN,0);
			close(clienteFd);
		}
	}
	close(clienteFd);
	
}



int main(void) {
	pid_t pid, sid;
  	    
	pid = fork();
	/* validar fork retorno */
	if (pid < 0) {
		return -1;
	}
	/* finalizar proceso padre */
	if (pid > 0) {
		return -1;
	}

	/*  mascara de ficheros cambiado: acceso de otro usuario a ficheros generados aqui*/
	umask(0);
	/* asignar un nuevo pid evitando problemas que se genere un proceso zombie*/
	/* y validar nuevo id para procesos */
	
	sid = setsid();
	
	if (sid < 0) {
		perror("new SID failed");
		
	}

	/* recomendable cambiar wd (medida de seguridad)*/

	if ((chdir("/")) < 0) {
		perror("error al cambiar directorio de trabajo");
		return -1;
	}

	/* descriptores standard deben ser cerrados (medida de seguridad) */

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	/* proceso daemon */
	/* bucle infinito del daemon */
	/* aqui responder las solicitudes que pida el webserver*/
	while (1) {
		/*falta describir*/
		ListenRequestClient();
	}	
}