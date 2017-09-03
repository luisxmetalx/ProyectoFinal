#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>

#define BUFFSIZE 1
#define	ERROR	-1
//declaramos las funciones a usar en el socket
void recibirArchivo(int SocketFD, FILE *file, char *name);
void enviarConfirmacion(int SocketFD);

static struct udev_device* get_child(struct udev* udev, struct udev_device* parent, const char* subsystem)
{
    struct udev_device* child = NULL;
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_parent(enumerate, parent);
    udev_enumerate_add_match_subsystem(enumerate, subsystem);
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        child = udev_device_new_from_syspath(udev, path);
        break;
    }

    udev_enumerate_unref(enumerate);
    return child;
}

static void enumerate_usb_mass_storage(struct udev* udev)
{
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, "scsi");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "scsi_device");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* scsi = udev_device_new_from_syspath(udev, path);

        struct udev_device* block = get_child(udev, scsi, "block");
        struct udev_device* scsi_disk = get_child(udev, scsi, "scsi_disk");

        struct udev_device* usb
            = udev_device_get_parent_with_subsystem_devtype(scsi, "usb", "usb_device");

        if (block && scsi_disk && usb) {
            printf("Node = %s\nID vendedor:ID Producto= %s:%s\nInformacion del Fabricante= %s\nTamano: %s\n",
                   udev_device_get_devnode(block),
                   udev_device_get_sysattr_value(usb, "idVendor"),
                   udev_device_get_sysattr_value(usb, "idProduct"),
                   udev_device_get_sysattr_value(scsi, "vendor"),
                   udev_device_get_sysattr_value(scsi, "size"));
        }

        if (block) {
            udev_device_unref(block);
        }

        if (scsi_disk) {
            udev_device_unref(scsi_disk);
        }

        udev_device_unref(scsi);
    }

    udev_enumerate_unref(enumerate);
}

void recibirArchivo(int SocketFD, FILE *file,char *name){
	char buffer[BUFFSIZE];
	int recibido = -1;

	/*Se abre el archivo para escritura*/
	file = fopen(name,"wb");
	while((recibido = recv(SocketFD, buffer, BUFFSIZE, 0)) > 0){
		//printf("%s",buffer);
		fwrite(buffer,sizeof(char),1,file);
	}//Termina la recepción del archivo
	fclose(file);
}//End recibirArchivo procedure

void enviarConfirmacion(int SocketFD){
	char mensaje[80] = "Paquete Recibido";
	printf("\nConfirmación enviada\n");
	if(write(SocketFD,mensaje,sizeof(mensaje)) == ERROR)
			perror("Error al enviar la confirmación:");
}//End enviarConfirmacion


int main(int argc, char **argv)
{
    /*
    pid_t process_id = 0;
    pid_t sid = 0;
    // Create child process
    process_id = fork();
    // Indication of fork() failure
    if (process_id < 0)
    {
        printf("fork failed!\n");
        // Return failure in exit status
        exit(1);
    }
    // PARENT PROCESS. Need to kill it.
    if (process_id > 0)
    {
        printf("process_id of child process %d \n", process_id);
        // return success in exit status
        exit(0);
    }
    //unmask the file mode
    umask(0);
    //set new session
    sid = setsid();
    if(sid < 0)
    {
        // Return failure
        exit(1);
    }
    // Change the current working directory to root.
    chdir(".");
    // Close stdin. stdout and stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);*/
    // Open a log file in write mode.
    while(1)
    {
        struct udev* udev = udev_new();
        
            enumerate_usb_mass_storage(udev);
        
            udev_unref(udev);

            sleep(5);
    }
    // a partir de aqui se iniciliaza el proceso socket
    struct sockaddr_in stSockAddr;
	int Res;
	int SocketFD;
	int puerto;
	FILE *archivo;

	if(argc == 1){
		printf("Uso: ./cliente <ip> <puerto> <directorio a leer> <nombre del archivo>\n");
		exit(-1);
	}

	if(argc != 5){
		printf( "por favor especificar un numero de puerto\n");
	}

	/*Se crea el socket*/
	SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
	/*Se verifica la integridad del socket*/
	if (SocketFD == ERROR){
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	/*Se configura la dirección del socket del cliente*/
	memset(&stSockAddr, 0, sizeof stSockAddr);
	puerto = atoi(argv[2]);
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(puerto);
	Res = inet_pton(AF_INET, argv[1], &stSockAddr.sin_addr);

	if (0 > Res){
		perror("error: El primer parametro no es una familia de direcciónes");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}else if (Res == 0){
		perror("char string (El segundo parametro no contiene una dirección IP válida");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	if (connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof stSockAddr) == ERROR){
		perror("Error a la hora de conectarse con el servidor");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	printf("Se ha conectado con el servidor:%s\n",(char *)inet_ntoa(stSockAddr.sin_addr));
	printf("%s\n",argv[3]);
	if(write(SocketFD,argv[3],sizeof(argv[3])) == ERROR)
	{
			perror("Error al enviar la confirmación:");
	}
	sleep(5);
	// se recibe el archivo
	recibirArchivo(SocketFD,archivo,argv[4]);
	enviarConfirmacion(SocketFD);
	close(SocketFD);
    return 0;
}