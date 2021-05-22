/*
codigo de referencia: https://developers.google.com/protocol-buffers/docs/cpptutorial
https://stackoverflow.com/questions/9496101/protocol-buffer-over-socket-in-c
https://blog.conan.io/2019/03/06/Serializing-your-data-with-Protobuf.html
https://www.geeksforgeeks.org/socket-programming-cc/
*/

#include <iostream>
#include <fstream>
#include <string.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include "payload.pb.h"
#define BUFFER_SIZE 8192

using namespace std;
struct sCliente //cliente
{
    int fileDecriptor;
    string username;
    char ipAddr[INET_ADDRSTRLEN];
    string status;
};

// Lista de clientes
unordered_map<string, sCliente *> Clientes;


void mMensajeError(int fileDecriptor, string errorMsj) //notificación de error
{
    string protoMensaje; //string a ser serializado
    chat::Payload *mError = new chat::Payload();
    mError->set_sender("server");
    mError->set_message(errorMsj);
    mError->set_code(500);

    mError->SerializeToString(&protoMensaje);
    char buffer[protoMensaje.size() + 1];
    strcpy(buffer, protoMensaje.c_str());
    send(fileDecriptor, buffer, sizeof buffer, 0);
}

void *tServidor(void *params) //funciones del servidor con cada cliente
{
    // Cliente actual
    struct sCliente ClienteTemp;
    struct sCliente *newClientParams = (struct sCliente *)params;
    int fileDecriptor = newClientParams->fileDecriptor;
    char buffer[BUFFER_SIZE];

    string protoMensaje;
    chat::Payload messageReceived ;

	while (1)
    {
        if (recv(fileDecriptor, buffer, BUFFER_SIZE, 0) < 1)
        {
            if (recv(fileDecriptor, buffer, BUFFER_SIZE, 0) == 0)
            {
                // cliente cerro conexion
                cout << "Servidor: el cliente "<< ClienteTemp.username<< "cerro sesion"<< endl;
            }
            break;
        }
        messageReceived.ParseFromString(buffer);
   

        if (messageReceived.flag() == chat::Payload_PayloadFlag_register_)//se registra usuario
        {
            cout << "Servidor: se recibio informacion de: "<< messageReceived.sender()<< endl;
			if (Clientes.count(messageReceived.sender()) > 0)//verificacion de duplicados
            {
                cout << "Servidor: ya hay un usuario usando ese nombre" << endl;
                mMensajeError(fileDecriptor, "ya hay un usuario usando ese nombre");
                break;
            }

            
            chat::Payload *mensajeTemp = new chat::Payload(); //registro exitoso

           mensajeTemp->set_sender("server");
           mensajeTemp->set_message("Registro completado");
           mensajeTemp->set_code(200);

           mensajeTemp->SerializeToString(&protoMensaje);

            strcpy(buffer, protoMensaje.c_str());
            send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);
            cout << "Servidor: usuario agregado : "<< fileDecriptor<< endl;

            ClienteTemp.username = messageReceived.sender();//infor del cliente
            ClienteTemp.fileDecriptor = fileDecriptor;
            ClienteTemp.status = "ACTIVO";
            strcpy(ClienteTemp.ipAddr, newClientParams->ipAddr);
            Clientes[ClienteTemp.username] = &ClienteTemp; //insercicón en mapa de clientes
        }
        else if (messageReceived.flag() == chat::Payload_PayloadFlag_user_list)//listado de clientes
        {
            cout << "Servidor: " << ClienteTemp.username<< " ha solicitado una lista de los usuarios conectados"<< endl;
            chat::Payload *mensajeTemp = new chat::Payload();

            string list_to_sent = "";
            
            for (auto item = Clientes.begin(); item != Clientes.end(); ++item)
            {
                list_to_sent = list_to_sent + "Id: " + to_string(item->second->fileDecriptor) + " usuario: " + (item->first) + " Status: " + (item->second->status)+ " ip: " + (item->second->ipAddr) + "\n";//info de cliente a un mensaje
            }
           mensajeTemp->set_sender("server");
           mensajeTemp->set_message(list_to_sent);
           mensajeTemp->set_code(200);

           mensajeTemp->SerializeToString(&protoMensaje);
            strcpy(buffer, protoMensaje.c_str());
            send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);
        }
        // Servicio que brinda informacion de un cliente en especifico
        else if (messageReceived.flag() == chat::Payload_PayloadFlag_user_info)
        {
            // Verificar que el cliente especificado exista
            if (Clientes.count(messageReceived.extra()) > 0)
            {
                // notificación de la solicitud de listado de clientes
                cout << "Servidor: el usuario " << ClienteTemp.username<< " ha solicitado la informacion del usuario " << messageReceived.extra()<< endl;

                // Se llena con la informacion del cliente especificado
                chat::Payload *mensajeTemp = new chat::Payload();
                struct sCliente *reqClient = Clientes[messageReceived.extra()];
                string mssToSend = "Id: " + to_string(reqClient->fileDecriptor) + " Username: " + (reqClient->username) + " Ip: " + (reqClient->ipAddr) + " Status: " + (reqClient->status) + "\n";

               mensajeTemp->set_sender("Server");
               mensajeTemp->set_message(mssToSend);
               mensajeTemp->set_code(200);

               mensajeTemp->SerializeToString(&protoMensaje);
                strcpy(buffer, protoMensaje.c_str());
                send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);
            }
            else
            {
                cout << "Servidor: No existe el nombre de usuario solicitado" << endl;
                mMensajeError(fileDecriptor, "No existe el nombre de usuario especificado.");
            }
        }
        // Servicio que cambia el estado
        else if (messageReceived.flag() == chat::Payload_PayloadFlag_update_status)
        {
            //Log del server
            cout << "Servidor: usuario " << ClienteTemp.username << " ha solicitado un nuevo estado: "<< messageReceived.extra()<< endl;

            // Actualizar estado de cliente
            ClienteTemp.status = messageReceived.extra();

            
            chat::Payload *mensajeTemp = new chat::Payload();
           mensajeTemp->set_sender("Server");
           mensajeTemp->set_message("Se ha actualizado el status correctamente a " + messageReceived.extra());//cambio de status correcto
           mensajeTemp->set_code(200);

           mensajeTemp->SerializeToString(&protoMensaje);
            strcpy(buffer, protoMensaje.c_str());
            send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);
        }
        
        else if (messageReceived.flag() == chat::Payload_PayloadFlag_general_chat)//mensaje general
        {
            // Log del server
            cout << "Servidor: el usuario " << ClienteTemp.username<< " desea enviar un mensaje general:\n"<< messageReceived.message() << endl;
            
            // Indicarle al remitente que el mensaje se mando exitosamente
            chat::Payload *serverResponse = new chat::Payload();
            serverResponse->set_sender("Server");
            serverResponse->set_message("Mensaje general mandado correctamente");
            serverResponse->set_code(200);

            serverResponse->SerializeToString(&protoMensaje);
            strcpy(buffer, protoMensaje.c_str());
            send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);

            // Mandar el mensaje general a todos los clientes conectados menos al remitente
            chat::Payload *genMessage = new chat::Payload();
            genMessage->set_sender("Server");
            genMessage->set_message("Mensaje general de "+messageReceived.sender()+": "+messageReceived.message()+"\n");
            genMessage->set_code(200);

            genMessage->SerializeToString(&protoMensaje);
            strcpy(buffer, protoMensaje.c_str());
            for (auto item = Clientes.begin(); item != Clientes.end(); ++item)
            {
                if (item->first != ClienteTemp.username)
                {
                    send(item->second->fileDecriptor, buffer, protoMensaje.size() + 1, 0);
                }
            }
        }
        // Servicio que manda un mensaje directo a un usuario especificado
        else if (messageReceived.flag() == chat::Payload_PayloadFlag_private_chat)
        {
            // Validar que usuario destinatario exista y este conectado
            if (Clientes.count(messageReceived.extra()) < 1)
            {
                cout << "Servidor: el usuario " << messageReceived.extra() << " no existe o no estÃ¡ conectado" << endl;
                mMensajeError(fileDecriptor, "Destinario no existe o no esta conectado");
                continue;
            }

            // Log del server
            cout << "Servidor: el usuario " << ClienteTemp.username
                      << " desea enviar un mensaje privado a " << messageReceived.extra()
                      << ". El mensaje es: \n" <<  messageReceived.message()
                      << endl;
            
            // Indicarle al remitente que el mensaje se mando exitosamente
            chat::Payload *serverResponse = new chat::Payload();
            serverResponse->set_sender("Server");
            serverResponse->set_message("Mensaje privado mandado correctamente a "+messageReceived.extra());
            serverResponse->set_code(200);

            serverResponse->SerializeToString(&protoMensaje);
            strcpy(buffer, protoMensaje.c_str());
            send(fileDecriptor, buffer, protoMensaje.size() + 1, 0);

            // Enviar mensaje a destinatario
            chat::Payload *privMessage = new chat::Payload();
            privMessage->set_sender(ClienteTemp.username);
            privMessage->set_message("Mensaje privado de "+messageReceived.sender()+": "+messageReceived.message()+"\n");
            privMessage->set_code(200);

            privMessage->SerializeToString(&protoMensaje);
            int destSocket = Clientes[messageReceived.extra()]->fileDecriptor;
            strcpy(buffer, protoMensaje.c_str());
            send(destSocket, buffer, protoMensaje.size() + 1, 0);

        }
        // En el hipotetico caso que se mande una opcion que no existe
        else
        {
            mMensajeError(fileDecriptor, "Opcion indicada no existe.");
        }
        // Se indica siempre cuantos usuarios estan conectados
        printf("________________________________________________________\n");
        cout << endl<< "Usuarios conectados: " << Clientes.size() << endl;
        printf("________________________________________________________\n");
    }

    // Cuando el cliente se desconecta se elimina de la lista y se cierra su socket
    Clientes.erase(ClienteTemp.username);
    close(fileDecriptor);
    string thisUser = ClienteTemp.username;
    if (thisUser.empty())
        thisUser = "No cliente";
    // Log del server
    cout << "Servidor: socket de " << thisUser<< " cerrado."<< endl;
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    cout<<"	                             .__     .___       "<<endl;        
    cout<<"  ______  ____ _______ ___  __|__|  __| _/ ____ _______ "<<endl;
	cout<<" /  ___/_/ __ \\_  __ \\  \/ /|  | / __ | /  _ \\_  __ \""<<endl;
	cout<<" \___ \ \  ___/ |  | \/ \   / |  |/ /_/ |(  <_> )|  | \/"<<endl;
    cout<<"/____  > \___  >|__|     \_/  |__|\____ | \____/ |__|"<<endl;   
	cout<<"     \/      \/                        \/ " <<endl;
	    
    //Cuando no se indica el puerto del server
    if (argc != 2)
    {
        fprintf(stderr, "prueba usarlo así./server <puertodelservidor>\n");
        return 1;
    }

    long port = strtol(argv[1], NULL, 10); //numero de puerto

    sockaddr_in server, incoming_conn;
    socklen_t new_conn_size;
    int socket_fd, new_conn_fd;
    char incoming_conn_addr[INET_ADDRSTRLEN];

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(server.sin_zero, 0, sizeof server.sin_zero);
    //------------------------------------***Funciones de coneccion socket***----------------------------------------
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) //se crea el socket
    {
        fprintf(stderr, "Servidor: no se puddo crear el socket.\n");
        return 1;
    }

    if (bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) == -1)//se une el puerto al socket
    {
        close(socket_fd);
        fprintf(stderr, "Servidor: no se pudo bindear el puerto al socket.\n");
        return 2;
    }

    // escuchar conexiones
    if (listen(socket_fd, 5) == -1)
    {
        close(socket_fd);
        fprintf(stderr, "Servidor: error en la escucha.\n");
        return 3;
    }
    printf("-----------------------------------------------\n");
    printf("Servidor-- escuchando en el puerto: %ld\n", port);

    // Aceptar conecciones
    while (1)
    {
        new_conn_size = sizeof incoming_conn;
        new_conn_fd = accept(socket_fd, (struct sockaddr *)&incoming_conn, &new_conn_size);
        if (new_conn_fd == -1)
        {
            perror("error aceptando la conexión");
            continue;
        }
	//-------------------------------------------------------------------------------------------------------------------
        // Aceptar nuevo cliente
        struct sCliente newClient;
        newClient.fileDecriptor = new_conn_fd;
        inet_ntop(AF_INET, &(incoming_conn.sin_addr), newClient.ipAddr, INET_ADDRSTRLEN);

        // Thread para le nuevo cliente
        pthread_t thread_id;
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        pthread_create(&thread_id, &attrs, tServidor, (void *)&newClient);
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
