PROYECTO FINAL
PROGRAMACION DE SISTEMAS

Autores

-LUIS ANDRADE DEL PEZO

-JONATHAN PARRALES NEIRA

1 - Introducción

En este proyecto referente a la materia de Programación de Sistemas periodo 2017 T1 de la Escuela Superior Politécnica de Litoral dirigido por el Ing. Eduardo Murillo presenta como contexto principal el monitoreo de unidades de almacenamiento masivo (USB)  en el cual se podrá hacer peticiones tipo POST y GET a través de un servidor web que estará conectado mediante socket a un proceso en segundo plano encargado de monitorear las unidades USB y enviar la información de las mismas, así mismo para la prueba de las funcionalidades de los programas se ha creado un cliente escrito en python que realiza:

Realizar una solicitud GET para listar los dispositivos USB conectados en formato JSON

Realizar una solicitud POST darle un nombre a una de las unidades USB conectadas, como una especie de apodo.

Realizar una solicitud GET para leer un archivo que se encuentre dentro de alguna de las unidades USB conectadas al equipo

Realizar una solicitud POST para escribir un archivo en una unidad USB especificada.

2 - Instalación

Para en funcionamiento correcto de los ejecutables se deben tener instaladas las siguientes Librerías:

Libreria libudev

sudo apt-get install libudev-dev

Libreria libmicrohttpd

sudo apt-get install libmicrohttpd*

3 – Ejecución

Ejecutar el Makefile que creará en la carpeta bin/ los ejecutables para el deamon y el WebServer

	make
Para ejecutar los archivos siga los siguientes pasos: 

Archivo demonio: daemon.

	./bin/daemon
	
iniciar servidor web. (	No cerrar esta terminal)

	./bin/WebServer <puerto> // debe ingresar el puerto al que se conectará
	
Para las pruebas con el Cliente.py se debe especificarle el puerto con el cual se inicio el servidor web

	python ./bin/Cliente.py <puerto>
	
Finalizar el proceso daemon.

	ps -A | grep daemon
	
	kill -TERM <NUMERO PID>
