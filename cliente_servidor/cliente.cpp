#include <iostream>
#include <limits> //Para manejar los limites de los tipos de datos
#include <winsock2.h> //Para trabajar con sockets en Windows
#include <fstream> //Para opciones de archivo
#include <regex> //libreria necesaria para validar formato de texto al insertar nueva traduccion
#include <set> //Arreglo de datos

using namespace std;

class Client {
public:
    /* Inicializar WinSock en la aplicación Cliente */

    WSADATA WSAData;
    SOCKET server;
    SOCKADDR_IN direccionServer;
    char buffer[1024];

    Client() {
        cout << "Conectando al servidor..." << endl << endl;

        // Inicializa la biblioteca Winsock con la versión 2.0.
        WSAStartup(MAKEWORD(2, 0), &WSAData);

        // Crea un socket para el cliente con IPv4 y protocolo TCP.
        server = socket(AF_INET, SOCK_STREAM, 0);

        // Configura la dirección IP y el puerto del servidor al que se conectará el cliente.
        direccionServer.sin_addr.s_addr = inet_addr("192.168.0.19"); // dirección IP del servidor, corresponde a la de misma ip de la maquina si el servidor y cliente estan en la misma.
        direccionServer.sin_family = AF_INET;

        //Vamos a solicitarle al cliente que ingrese al puerto que desea conectarse.
        int puerto;
        cout << "\nIngrese un puerto al que se quiere conectar: " << endl;
        cin >> puerto;
        direccionServer.sin_port = htons(puerto); // Puerto 5005 (debe coincidir con el del servidor).

        // Establece la conexión con el servidor. Si es 0 la conexion se realizo con exito, sino informa error
        if(connect(server, (SOCKADDR *)&direccionServer, sizeof(direccionServer)) != 0){
            printf("No se pudo establecer conexion con el servidor\n");
            WSACleanup();
            return;
        }else {
            cout << "Conectado al Servidor!" << endl; //si la conexion fue exitosa pasara a confirmar usuario y contrasena
            EnviarCredenciales();
        }
    }

///Metodo en que se realiza el envio de credenciales al servidor para las validaciones correspondientes
void EnviarCredenciales() {
    //bandera que sirve como salida del bucle de envio y recepcion de credenciales
    bool solicitando = true;
    while (solicitando) {

        //Ingresamos el usuario y la contraseña que vamos a enviarle al servidor para que nos valide
        string usuario, contrasena;
        cout << "Ingrese su usuario: ";
        cin >> usuario;

        // Limpia el búfer antes de leer la contraseña.
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        cout << "Ingrese su contrasena: ";
        cin >> contrasena;

        // Formateo las credenciales como una cadena entera "usuario|contraseña" y las envío al servidor
        string credenciales = usuario + "|" + contrasena;
        send(server, credenciales.c_str(), credenciales.length(), 0);// le envio el puntero a la credencial
        memset(buffer, 0, sizeof(buffer)); // Limpia el buffer después de enviar

        //el servidor me enviara el rol una vez pasado por la validacion, si este no paso la validacion recibiremos algun msjs distinto
        //que se guardara en la variable rol
        char rol[1024];
        recv(server, rol, sizeof(rol), 0);
        memset(buffer, 0, sizeof(buffer)); // Limpia el buffer después de leer.

        //cout << "aca esta la variable REY " << rol <<endl;

        //suponiendo que lo que regresa del parte del servidor es la palabra bloqueado guardada en la variable rol
        //este ingresara al if para informar esto y darnos la opcion de intentar nuevamente lo cual hara que vuelva a iterar para intentar
        //recibir la palabra que corresponde a la "autenticacion". Si le enviamos algo distinto de "S" o "s" saldremos del programa.
        if(strcmp(rol, "BLOQUEADO") == 0){
            cout<< "Usuario momentaneamente BLOQUEADO, intentos fallidos superados!" << endl;
            cout << "¿Quieres conectar con otro usuario? (S/N): ";
            char opcion;
            cin >> opcion;
            if (opcion != 'S' && opcion != 's') {
                CerrarSocket(); // Cierra la conexión si el usuario no desea intentar nuevamente.
                return;
            }
        }
        //pasa algo similar en este caso pero para este caso el usuario no esta bloqueado sino que por algun otro motivo no pudo autenticarse.
        //el msjs para este caso de parte del servidor sera "fallido".
        if(strcmp(rol, "FALLIDO") == 0){
            cout << "Credenciales invalida, ¿Desea intentar nuevamente? (S/N): ";
            char opcion;
            cin >> opcion;
            if (opcion != 'S' && opcion != 's') {
                    CerrarSocket(); // Cierra la conexión si el usuario no desea intentar nuevamente.
            }
        }

        // Espera la respuesta del servidor sobre la autenticación.
        char respuesta[1024];
        recv(server, respuesta, sizeof(respuesta), 0);
        memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de leer.
        cout << "Respuesta del servidor: " << respuesta << endl;

        //Dependiendo la respuesta del servidor y el tipo de rol con el que se encuentra trabajando el cliente es con el menu que va a contar
        //ROL DEL ADMIN = MENU DEL ADMIN
        if (strcmp(respuesta, "Autenticado") == 0 && strcmp(rol, "ADMIN") == 0) {
            cout << "Usuario autenticado correctamente." << endl;
            solicitando = false; // Salgo del bucle una vez que pude validarme para no repetir la accion si este decide cerrar el socket
            cout << "Entraste como ADMIN " << endl;
            // Limpia la consola
            system("cls");
            MenuAdmin(); // Si la autenticación es exitosa, muestra el menu correspondiente para ADMIN
        }
        //ROL DEL CONSULTA = MENU DEL USUARIO CONSULTA
        if (strcmp(respuesta, "Autenticado") == 0 && strcmp(rol, "CONSULTA") == 0){
            cout << "Usuario autenticado correctamente." << endl;
            solicitando = false; // Salgo del bucle una vez que pude validarme para no repetir la accion si este decide cerrar el socket
            cout << "Entraste como Consulta " << endl;
            // Limpia la consola
            system("cls");
            MenuConsulta();// Si la autenticación es exitosa, muestra el menu correspondiente para CONSULTA
        }
    }
}

///Metodo que se encarga de cerrar el socket
void CerrarSocket() {
    // Cierra el socket del cliente y limpia la biblioteca Winsock
    closesocket(server);
    WSACleanup();
    cout << "\nSocket cerrado." << endl;
    exit(0);
}

///En este procedimiento se van a realizar todas las interaciones con el menu de admin que debe trabajar el servidor
void MenuAdmin() {

    int opcion;
        do{
			cout << "\n------------------ MENU ADMIN ------------------\n\nIngrese la opcion correcta:\n\n";
			cout << "1 - Ingresar Nueva Traduccion \n";
			cout << "2 - Ingresar a Usuarios-Alta \n";
			cout << "3 - Ingresar a Usuarios-Desbloqueo \n";
			cout << "4 - Ver registro de actividades \n";
			cout << "0 - Salir \n\n";
			cin >> opcion; //guardo la seleccion y seguido se la envio al servidor para que realice las validaciones y demas
			send(server, (char*)&opcion, sizeof(opcion), 0); //envio el numero entero que recibira el servidor para entrar a la opcion correcta
			// Limpia la consola
            system("cls");
			if (opcion == 1){
                cout << "------- ALTA NUEVA TRADUCCION ------\n\n";
                std::string palabraInglesEspaniol;
                cout << "Ingrese palabra para insertar: FORMATO ====> university:universidad\n";
                cin >> palabraInglesEspaniol;

                // Limpia el búfer después de leer palabraInglesEspaniol
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                cout << "Estas enviando: "+ palabraInglesEspaniol << endl; //Mostramos la palabra que se esta enviando
                send(server, palabraInglesEspaniol.c_str(), palabraInglesEspaniol.length(), 0); //enviamos la palabra para insertar al servidor
                //Luego de haber hecho el envio de la palabra esperamos una respuesta de parte del servidor
                char respuesta[1024];
                recv(server, respuesta, sizeof(respuesta), 0);
                cout << "Respuesta del servidor: " << respuesta << endl;
			}

			if(opcion == 2){
                cout << "------- ALTA USUARIO ------\n\n";
			    //Creo usuario y contraseña que recibira el servidor
                std::string usuario, contrasena;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Limpia el búfer antes de leer
                cout << "Ingrese usuario: ";
                cin >> usuario;
                send(server, usuario.c_str(), usuario.length(), 0); //Envio al servidor el usuario

                //De esta siguiente manera puedo revisar que el caracter que se este ingresando esta o no vacio
                cout << "Ingrese contrasena: ";
                cin.ignore(); // Ignora el carácter de nueva línea anterior
                // Leer la contraseña como una línea completa
                getline(cin, contrasena);

                // Si la contraseña está vacía, asigna una cadena vacía a contrasena
                if (contrasena.empty()) {
                    contrasena = "";
                }
                send(server, contrasena.c_str(), contrasena.length(), 0);//Envio al servidor la contraseña tenga o no caraceres

                //Espero la respuesta de parte del servidor
                char respuesta[1024];
                recv(server, respuesta, sizeof(respuesta), 0);
                cout << "Respuesta del servidor: " << respuesta << endl;
			}

			if(opcion == 3){
                    cout << "------- USUARIOS BLOQUEADOS ------\n\n";
                    // Recibir la cantidad de usuarios bloqueados del servidor
                    int numUsuariosBloqueados;
                    recv(server, (char*)&numUsuariosBloqueados, sizeof(numUsuariosBloqueados), 0);

                    //si es igual a 0 por tanto no tenemos usuarios bloqueados
                    if (numUsuariosBloqueados > 0) {
                        std::cout << "Usuarios bloqueados:" << std::endl;

                        // Recibir y mostrar cada usuario bloqueado con su indice
                        for (int i = 0; i < numUsuariosBloqueados; ++i) {
                            int usuarioLength;
                            recv(server, (char*)&usuarioLength, sizeof(usuarioLength), 0);

                            char usuarioBuffer[1024]; // usuario
                            recv(server, usuarioBuffer, usuarioLength, 0);

                            // Convertimos el usuario recibido a una cadena de C++
                            std::string usuario(usuarioBuffer, usuarioLength);

                            // Mostramos el usuario con su índice
                            std::cout << i + 1 << " - " << usuario << std::endl;
                        }
                        //Luego de recibir los usuarios bloqueados vamos a enviar el que queremos desbloquear
                        std::string usuarioADesbloquear;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Limpia el búfer antes de leer
                        //Podemos ingresar cualquier palabra pero la que es igual a "salir" ejecutara un cierre de parte del server
                        cout << "\nIngrese el nombre del usuario que quieres desbloquear o 'salir' para volver: " << endl;
                        cin >> usuarioADesbloquear;
                        //guardo y envio al servidor el usuario que quiero desbloquear
                        send(server, usuarioADesbloquear.c_str(), usuarioADesbloquear.length(), 0);
                        memset(buffer, 0, sizeof(buffer)); // Limpia el buffer después de enviar.
                        //luego de enviar se espera una respuesta del servidor para mostrar
                        char respuesta[1024];
                        recv(server, respuesta, sizeof(respuesta), 0);
                        cout << "Respuesta del servidor: " << respuesta << endl;
                        memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de enviar.
                    } else {
                        cout << "No hay usuarios bloqueados." << std::endl;
                    }
			}
            if (opcion == 4) {

                // Declaro un buffer con un tamaño fijo pero que esta vez se va a ir concatenando
                char bufferTemp[1024];

                // Bandera para controlar la salida del bucle
                bool seguirRecibiendo = true;
                std::cout << "------- INICIO HISTORIAL ------\n\n" << std::endl;
                // Bucle para recibir datos en fragmentos
                while (seguirRecibiendo) {
                    // Ahora recibo del servidor y guardo en la variable bufferTemp
                    int bytesRecibidos = recv(server, bufferTemp, sizeof(bufferTemp) - 1, 0);
                    // Manejar errores de recepcion, en el caso que no este llegando info
                    if (bytesRecibidos <= 0) {
                        if (bytesRecibidos == 0) { // igual a 0 es porque se perdió la conexión
                            // La conexión se cerró
                            cout << "Error se cerró la conexión del servidor" << endl;
                            seguirRecibiendo = false; // Cambiamos la bandera para salir del bucle
                        } else { // sino hubo un error al recibir los datos
                            // Error de recepción
                            cout << "Error de recepción de datos" << endl;
                            seguirRecibiendo = false; // Cambiamos la bandera para salir del bucle
                        }
                    }

                    // termino el buffer temporal con el carácter nulo
                    bufferTemp[bytesRecibidos] = '\0';
                    // Mostrar los datos recibidos hasta ahora
                    std::cout << bufferTemp;

                    // Si no hay mas datos para recibir, salgo del bucle
                    if (bytesRecibidos < sizeof(bufferTemp) - 1) {
                        seguirRecibiendo = false; // Cambiamos la bandera
                    }
                }
                std::cout << "------- FIN HISTORIAL ------\n\n" << std::endl;
            }

			if (opcion == 0){
                CerrarSocket(); // llamamos al metodo cerrarSocket para la opcion == 0
			}
        }while(opcion != 0);
}

///menu para usuarios tipo consulta
void MenuConsulta(){
            int opcion;
        do{
			cout << "\n------------------ MENU CONSULTA ------------------\n\nIngrese la opcion correcta:\n\n";
			cout << "1 - TRADUCIR \n";
			cout << "0 - Salir \n";
			cin >> opcion;
			if (opcion == 1){
			    system("cls");
                Traducir();
			}
			if (opcion == 0){
                CerrarSocket();
			}
        }while(opcion != 0);
    }

///metodo que envia al servidor la palabra que debe buscar para traducir
void Traducir() {
    cout << "------- TRADUCCION ------\n\n";
    // Solicita al usuario la palabra a traducir.
    std::string palabra;
    cout << "Ingrese la palabra a traducir: ";
    cin >> palabra;

    // Envía la palabra al servidor.
    send(server, palabra.c_str(), palabra.length(), 0);

    // Recibe la traducción del servidor.
    char traduccion[1024];
    recv(server, traduccion, sizeof(traduccion), 0);
    cout << "Traducción: " << traduccion << endl;
}

};
