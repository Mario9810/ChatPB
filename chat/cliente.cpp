/*
codigo de referencia: https://developers.google.com/protocol-buffers/docs/cpptutorial
https://stackoverflow.com/questions/9496101/protocol-buffer-over-socket-in-c
https://blog.conan.io/2019/03/06/Serializing-your-data-with-Protobuf.html
https://www.geeksforgeeks.org/socket-programming-cc/
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
#include "payload.pb.h"
#include <queue>
using namespace std;
int connected, waitingForServerResponse, waitingForInput;
string statusArray[3] = {"activo", "ocupado", "inactivo"};


// utilizando obtener sockaddr
void *obtenerAddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// escuha mensajes en el servidor
void *escMensajes(void *args)
{
	while (1)
	{
		// mensaje
		char bufferMsg[8192];
		int *sockmsg = (int *)args;
		chat::Payload serverMsg;

		int bytesReceived = recv(*sockmsg, bufferMsg, 8192, 0);

		serverMsg.ParseFromString(bufferMsg);

		// mensaje de error
		if (serverMsg.code() == 500)
		{
			printf("--------------------------------------------------------\n");
			cout << "Error: "
			  << serverMsg.message()
			  << endl;
		}
		// respuesta del servidor
		else if (serverMsg.code() == 200)
		{
			printf("--------------------------------------------------------\n");
			cout << "servidor: \n"
			  << serverMsg.message()
			  << endl;
		}
		// respuesta no reconocida
		else
		{
			printf("no disponible\n");
			break;
		}

		waitingForServerResponse = 0;

		if (connected == 0)
		{
			pthread_exit(0);
		}
	}
}

// menu
void menu(char *usrname)
{
	printf("-------------------------------------------------------- \n");
	printf("Menu, %s. \n", usrname);
	printf("1: Usuarios conectados. \n");
	printf("2: Dinformacion de un usuario. \n");
	printf("3: Cambiar estado. \n");
	printf("4: Mandar mensaje a todos. \n");
	printf("5: Mandar mensaje personal. \n");
	printf("6: Help. \n");
	printf("7: Cerrar. \n");
	printf("-------------------------------------------------------- \n");
}

// ayuda
void ayuda() {
	printf("-------------------------------------------------------- \n");
	printf("Help \n");
	printf("Usuarios conectados: muestra todos los usuarios conectados con su informacion\n");
	printf("Informacion de un usuario: muestra informacion de usuario \n");
	printf("Cambiar estado: cambia estados disponibles, son vivo, muerto y lleno. \n");
	printf("Mandar mensaje a todos: mensaje a todos los usuarios en sesion \n");
	printf("Mandar mensaje personal: mensaje privado al usuario en sesion \n");
	printf("Help: muestra ayuda \n");
	printf("Cerrar: sale del sistema. \n");
	printf("-------------------------------------------------------- \n");
}

// opcion cliente
int obtenerCliente()
{
	// opcion del cliente
	int client_opt;
	cin >> client_opt;

	while (cin.fail())
	{
		cout << "Ingrese opcion correcta: " << endl;
		cin.clear();
		cin.ignore(256, '\n');
		cin >> client_opt;
	}

	return client_opt;
}

int main(int argc, char *argv[])
{
	cout<<"	        .__   .__                  __"<<endl;           
	cout<<"   ____  |  |  |__|  ____    ____ _/  |_   ____ " <<endl;
	cout<<" _/ ___\ |  |  |  |_/ __ \  /    \\   __\_/ __ \""<<endl; 
	cout<<" \  \___ |  |__|  |\  ___/ |   |  \|  |  \  ___/ "<<endl;
	cout<<"  \___  >|____/|__| \___  >|___|  /|__|   \___  >"<<endl;
	cout<<"      \/                \/      \/            \/ "<<endl;
                                                
	// conexion
	int sockfd, numbytes;
	char buf[8192];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 4)
	{
		fprintf(stderr, "Uso: cliente <username> <server_ip> <server_port>\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// conecta opcion disponible
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("cliente: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("cliente: connect");
			close(sockfd);
			continue;
		}

		break;
	}
	// error
	if (p == NULL)
	{
		fprintf(stderr, "No se pudo conectar\n");
		return 2;
	}

	//finalizar conexion
	inet_ntop(p->ai_family, obtenerAddr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	printf("esta conectado con %s\n", s);
	freeaddrinfo(servinfo);


	// Escribir mensaje
	char buffer[8192];
	string message_serialized;

	chat::Payload *firstMessage = new chat::Payload;

	firstMessage->set_sender(argv[1]);
	firstMessage->set_flag(chat::Payload_PayloadFlag_register_);
	firstMessage->set_ip(s);

	firstMessage->SerializeToString(&message_serialized);

	strcpy(buffer, message_serialized.c_str());
	send(sockfd, buffer, message_serialized.size() + 1, 0);

	// response
	recv(sockfd, buffer, 8192, 0);

	chat::Payload serverMessage;
	serverMessage.ParseFromString(buffer);

	// Error de registro
	if(serverMessage.code() == 500){
			cout << "Error: "
			  << serverMessage.message()
			  << endl;
			return 0;
	}

	// Exito en registro
	cout << "Servidor: "
			  << serverMessage.message()
			  << endl;	

	connected = 1;

	// mustra hilo que muestra mensajes del server
	pthread_t thread_id;
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_create(&thread_id, &attrs, escMensajes, (void *)&sockfd);

	menu(argv[1]);
	int client_opt;

	// Lciclo que sigue mostrando menu
	while (true)
	{
		// Esto mantiene el orden
		while (waitingForServerResponse == 1){}

		printf("\nSeleccione opcion:\n");
		client_opt = obtenerCliente();

		string msgSerialized;
		int bytesReceived, bytesSent;

		// usuarios en sesion
		if (client_opt == 1)
		{
			chat::Payload *userList = new chat::Payload();
			userList->set_sender(argv[1]);
			userList->set_ip(s);
			userList->set_flag(chat::Payload_PayloadFlag_user_list);

			userList->SerializeToString(&msgSerialized);

			strcpy(buffer, msgSerialized.c_str());
			bytesSent = send(sockfd, buffer, msgSerialized.size() + 1, 0);
			waitingForServerResponse = 1;
		}
		// datos de usuarion
		else if (client_opt == 2)
		{
			string user_name;
			printf("Escribir nombre de usuario para extraer sus datos \n");
			cin >> user_name;

			chat::Payload *userInf = new chat::Payload();
			userInf->set_sender(argv[1]);
			userInf->set_flag(chat::Payload_PayloadFlag_user_info);
			userInf->set_extra(user_name);
			userInf->set_ip(s);

			userInf->SerializeToString(&msgSerialized);

			strcpy(buffer, msgSerialized.c_str());
			bytesSent = send(sockfd, buffer, msgSerialized.size() + 1, 0);
			waitingForServerResponse = 1;
		}
		// cambio de estado
		else if(client_opt == 3){
			printf("Ingresar estado: \n");
			printf("1. VIVO\n");
			printf("2. LLENO\n");
			printf("3. MUERTO\n");
			int option;
			cin >> option;
			string newStatus;
			if (option < 4){
				newStatus = statusArray[option-1];
			}
			else
			{
				printf("Este estado no se puede usar.\n");
				continue;
			}
			chat::Payload *userInf = new chat::Payload();
			userInf->set_sender(argv[1]);
			userInf->set_flag(chat::Payload_PayloadFlag_update_status);
			userInf->set_extra(newStatus);
			userInf->set_ip(s);

			userInf->SerializeToString(&msgSerialized);

			strcpy(buffer, msgSerialized.c_str());
			bytesSent = send(sockfd, buffer, msgSerialized.size() + 1, 0);
			waitingForServerResponse = 1;
		}
		// mensaje a todos
		else if (client_opt == 4)
		{
			waitingForInput = 1;
			printf("Escribir mensaje \n");
			cin.ignore();
			string msg;
			getline(cin, msg);


			chat::Payload *clientMsg= new chat::Payload();

			clientMsg->set_sender(argv[1]);
			clientMsg->set_message(msg);
			clientMsg->set_flag(chat::Payload_PayloadFlag_general_chat);
			clientMsg->set_ip(s);
			clientMsg->SerializeToString(&msgSerialized);

			strcpy(buffer, msgSerialized.c_str());
			bytesSent = send(sockfd, buffer, msgSerialized.size() + 1, 0);
			waitingForServerResponse = 1;
			waitingForInput = 0;
		}

		// Mensaje a usuario en linea
		else if (client_opt == 5)
		{
			printf("Escribir nombre de usuario: \n");
			cin.ignore();
			string user_name;
			getline(cin, user_name);

			printf("Escribir mensaje: \n");
			string msg;
			getline(cin, msg);

			chat::Payload *clientMsg= new chat::Payload();

			clientMsg->set_sender(argv[1]);
			clientMsg->set_message(msg);
			clientMsg->set_flag(chat::Payload_PayloadFlag_private_chat);
			clientMsg->set_extra(user_name);
			clientMsg->set_ip(s);
			clientMsg->SerializeToString(&msgSerialized);

			strcpy(buffer, msgSerialized.c_str());
			bytesSent = send(sockfd, buffer, msgSerialized.size() + 1, 0);
			waitingForServerResponse = 1;
		}
		else if (client_opt == 6)
		{
			ayuda();
		}
		
		else if (client_opt == 7)
		{
			int option;
			printf("Salir? \n");
			printf("1: Si \n");
			printf("2: No \n");

			cin >> option;

			if (option == 1)
			{
				printf("ADIOS\n");
				break;
			}
		}
		else
		{
			cout << "La selecion no existe" << endl;
		}
		
		
	}

	// sale de la conexion
	pthread_cancel(thread_id);
	connected = 0;
	close(sockfd);

	return 0;
}
