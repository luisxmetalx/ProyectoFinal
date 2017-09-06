import requests
import json
import sys

if len(sys.argv) == 2:
    while True:
        print("***********Proyecto Programacion de Sistemas***********")
        print("\nEscoja una de las siguientes opciones para probar el proceso deamon y servidor web: ")
        print("1-) Solicitud GET Listar dispositivos")
        print("2-) Solicitud POST Nombrar dispositivo")
        print("3-) Solicitud GET Leer archivo")
        print("4-) Solicitud POST Escribir archivo")
        print("5-) Salir")
        opcion=input("----Ingrese la opción que desea: ") 
        if opcion=="1":
                url="http://127.0.0.1:"+str(sys.argv[1])+"/listar_dispositivo"
                print(url)
                r=requests.get(url)
                dic=r.json()
                print("ENCABEZADOS:",r.headers)
                print("CODIGO ESTADO:",r.status_code)
                print("JSON RESPUESTA:",dic)
                print("JSON STATUS:",dic['status'])
        if opcion=="2":
                url="http://127.0.0.1:"+str(sys.argv[1])+"/nombrar_dispositivo"
                nodo=input("Ingrese el nodo del dispositivo: ")
                nombre=input("Ingrese el nombre a asignar: ")
                jsons={"solicitud":"nombrar_dispositivo","nodo": nodo,"nombre":nombre}
                print(url)
                r=requests.post(url,json=json.dumps(jsons))
                dic=r.json()
                print("ENCABEZADOS:",r.headers)
                print("CODIGO ESTADO:",r.status_code)
                print("JSON RESPUESTA:",dic)
                print("JSON STATUS:",dic['status'])
        if opcion=="3":
                url="http://127.0.0.1:"+str(sys.argv[1])+"/leer_archivo"
                nombre=input("Ingrese el nodo del dispositivo: ")
                nomfile=input("Ingrese el nombre a asignar: ")
                jsons={"solicitud":"leer_archivo","nombre": nombre,"nombre_archivo":nomfile}
                print(url)
                r=requests.get(url,data=jsons)
                dic=r.json()
                print("ENCABEZADOS:",r.headers)
                print("CODIGO ESTADO:",r.status_code)
                print("JSON RESPUESTA:",dic)
                print("JSON STATUS:",dic['status'])
        if opcion=="4":
                url="http://127.0.0.1:"+str(sys.argv[1])+"/escribir_archivo"
                nombre=input("Ingrese el nodo del dispositivo: ")
                dirfile=input("Ingrese la direccion del archivo a escribir: ")
                archivo = open(dirfile)
                contenido=""
                for linea in archivo:
                    contenido=contenido+linea
                archivo.close()
                jsons={"solicitud":"escribir_archivo","nombre": nombre,"nombre_archivo":dirfile,"tamano_contenido":len(contenido),"contenido":contenido}
                print(url)
                r=requests.post(url,json=jsons)
                dic=r.json()
                print("ENCABEZADOS:",r.headers)
                print("CODIGO ESTADO:",r.status_code)
                print("JSON RESPUESTA:",dic)
                print("JSON STATUS:",dic['status'])
        if opcion=="5":
            print("Gracias por su visita")
            break
else:
    print("Modo de Uso: python ./src/Cliente.py <puerto>")