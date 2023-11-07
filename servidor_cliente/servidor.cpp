#include <iostream>
#include <winsock2.h> //se utiliza para programar aplicaciones de red en C++.
//Proporciona las funciones y estructuras necesarias para crear y gestionar conexiones de red utilizando sockets.
#include <fstream> //Puedes abrir, leer y escribir archivos utilizando objetos ifstream (entrada) y ofstream (salida).
#include <sstream> //se utiliza para trabajar con flujos de texto en memoria.
//Puedes usarla para convertir entre tipos de datos y cadenas de caracteres, especialmente útil cuando necesitas procesar texto.
#include <string> //std::string se tiliza para trabajar con flujos de texto en memoria.
#include <regex> //libreria necesaria para validar formato de texto al insertar nueva traduccion
#include <set> //Se utiliza para manejar coleccion de elemento (Arrays)
#include <limits>

using namespace std;

class Server {

    private:
    ofstream logFile; // Agrega un miembro para el archivo de registro.

public:
    /* Inicializar WinSock en la aplicación Servidor */

    WSADATA WSAData; // Estructura para almacenar información sobre la biblioteca Winsock.
    SOCKET server, client; // Descriptores de socket para el servidor y el cliente.
    SOCKADDR_IN direccionLocal, clientAddr; // Estructuras para almacenar información de direcciones IP y puertos.
    char buffer[1024]; // Búfer para almacenar mensajes.

    Server() {
        // Inicializa la biblioteca Winsock con la versión 2.0.
        WSAStartup(MAKEWORD(2, 0), &WSAData);

        // Crea un socket para el servidor con IPv4 y protocolo TCP.
        server = socket(AF_INET, SOCK_STREAM, 0);

        // Configura la dirección y el puerto del servidor.
        direccionLocal.sin_addr.s_addr = INADDR_ANY;
        direccionLocal.sin_family = AF_INET;
        int puerto = 5005;
        direccionLocal.sin_port = htons(puerto);  // Puerto 5005.

        /*La función bind() utilizada asocia una dirección local con el socket.
        En este caso estamos especificando que el socket deberá recibir llamadas
        entrantes en cuyo port destino se especifice 5005.*/
        bind(server, (SOCKADDR *)&direccionLocal, sizeof(direccionLocal));

        //listen(x,x) habilita al servidor para escuchar conexiones entrantes.
        //El último parámetro especifica la cantidad de conexiones que pueden ser encoladas en espera de ser atendidas
        //Si se recibe más llamadas, las mismas serán rechazadas automáticamente.
        listen(server, 1); // 0 no permite conexiones en espera, 1 vamos a decir que permite 1 cliente en espera

        // Abre el archivo de registro en modo de escritura.
        logFile.open("server.log.txt", ios::out | ios::app);
        if (!logFile.is_open()) {
            cerr << "No se pudo abrir el archivo de registro." << endl;
        }

        coutLog("==================================");
        coutLog("========Inicia Servidor===========");
        coutLog("==================================");
        //tuve que concatenar la palabra transformando a string el int.
        coutLog("Socket creado. Puerto de escucha: " + std::to_string(puerto));

        //Inicia el servidor y se queda siempre a la escucha
        while(true){
            int clientAddrSize = sizeof(clientAddr);

            // La función accept() espera la llegada de conexiones.
            // La llamada a esta función no retornará hasta que se reciba una llamada entrante.
            /* El código verifica si el valor devuelto por accept es diferente de INVALID_SOCKET.
            Si es diferente, significa que la conexión se ha aceptado correctamente,
            y se muestra un mensaje en la consola que indica que el cliente se ha conectado. */
            if ((client = accept(server, (SOCKADDR *)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {

                coutLog("---------Cliente conectado---------");
                ManejarConexion(); //si la conexion fue exitosa el servidor pasara con el cliente al manejo de conexion
        }
    }
}

///Voy a utilizar este metodo para no solo hacer los cout sino que tambien me va a servir para guardar todo lo que salga por consola
void coutLog(const string& mensaje) {
        cout << mensaje << endl; // Muestra el mensaje en la consola.
        if (logFile.is_open()) {
            logFile << mensaje << endl; // Escribe el mensaje en el archivo de registro.
        }
}

///cierra la conexion cuando el usuario lo hace
void CerrarConexion(std::string usuario) {
        closesocket(client);
        coutLog("Cierre de sesión – usuario: "+usuario);
    }

///Metodo que va a manejar todas las condiciones para enviar y recibir informacion
void ManejarConexion() {

    std::string usuario, contrasena; // Se crean 2 variables, que se utilizarán para guardar lo recibido del cliente

    // Si se reciben bytes de parte del cliente se guardara el tamaño de estos
    int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        coutLog("Error al recibir datos del cliente.");
        return; // Salir de la función si hay un error en la recepción.
    } else if (bytesReceived == 0) {
        CerrarConexion(usuario);
        send(client, "ERROR", sizeof("ERROR"), 0); // agregado 18/9
        return; // Salir de la función si el cliente se ha desconectado.
    }

    std::string credencialesRecibidas(buffer);//guardo las credenciales recibidas para luego usarlas
    memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de leer.
    // cout << credencialesRecibidas << endl; //cout para ver que esta llegando del lado del cliente y si esta llegando de forma correcta
    // Divide las credenciales en usuario y contraseña, esto lo hacemos para luego pasarlos por parametro al metodo
    //que se encargara de realizar las validaciones correspondientes.
    size_t pos = credencialesRecibidas.find('|'); //cuando este se encuentre con el simbolo "|" guardara esa posicion
    if (pos != string::npos) {
        usuario = credencialesRecibidas.substr(0, pos);//guardo usuario
        contrasena = credencialesRecibidas.substr(pos + 1); //guardo contraseña
    }

    // Llama a la función de autenticación para verificar las credenciales son correctas.
    bool autenticado = AutenticarUsuarioYContrasena(usuario, contrasena);

    // Envía una respuesta al cliente.
    if (autenticado) {
        send(client, "Autenticado", sizeof("Autenticado"), 0);
        coutLog("Autenticado Inicio de sesion -- usuario: "+usuario);
    } else {
        send(client, "ERROR", sizeof("ERROR"), 0);
        coutLog("--- Cliente no pudo autenticarse ---");
        ManejarConexion(); //Reiniciamos el proceso si se presenta algun error
    }

    //Busco el rol del usuario que ya se valido anteriormente para decidir a que while voy a ingresar y cual no
    string rol = obtenerRolDeUsuario(usuario);

    //El cliente está autenticado podemos continuar con la comunicacion
    //Manejo de las solicitudes del usuario ADMIN mientras este respete la condicion de autenticado y de rol
    while (autenticado && rol == "ADMIN") {

        coutLog("Cliente conectado puede usar el menu ADMIN, esperando opcion....\n");
        int opcion; //Inicializo la variable en la que voy a captar la opcion enviada por el cliente
        int bytesReceived = recv(client, (char*)&opcion, sizeof(opcion), 0);

        //si la opcion recibida es 1 ingresamos al metodo que inserta la nueva traducción
        if(opcion == 1){
            char palabraInglesEspaniol[1024];
            int bytesReceived = recv(client, palabraInglesEspaniol, sizeof(palabraInglesEspaniol), 0);
            //este condicional es para que no falle la palabra que recibo, estaba recibiendo basura anteriormente
            //garantizo que la cadena no tenga caracteres de mas
            palabraInglesEspaniol[bytesReceived] = '\0'; //agrego un carácter nulo para marcar el final de la cadena
            std::string palabraRecibida(palabraInglesEspaniol); //casteamos a string
            coutLog("Recibí del cliente: "+palabraRecibida);
            //si la validacion de la palabra es true se transformara a minuscula y se insertara todo dentro de ese metodo
            //enviamos al cliente el msjs de exito, caso contrario el metodo enviara otro msjs al cliente.
            if(ingresarNuevaTraduccion(palabraRecibida)){
                send(client, "Palabra Insertada Correctamente!", sizeof("Palabra Insertada Correctamente!"), 0);
                coutLog("---- Nueva traduccion insertada ---- por usuario: "+usuario);
            }
        }
        //si la opcion recibida es 2 ingresamos al metodo que inserta un nuevo usuario
        if(opcion == 2){
            char usuarioRecibido[1024]; //variable donde guardare el usuario enviado por el cliente
            int bytesReceived = recv(client, usuarioRecibido, sizeof(usuarioRecibido), 0);
            //este condicional es para que no falle la palabra que recibo, estaba recibiendo basura anteriormente
            //no fue necesario usarlo, comentar //

            //garantizo que la cadena no tenga caracteres de mas
            usuarioRecibido[bytesReceived] = '\0'; //agrego un carácter nulo para marcar el final de la cadena
            std::string usuarioR(usuarioRecibido); //se castea como string para que no haya errores al recibir
            coutLog("Recibí del cliente el usuario: "+usuarioR);
            memset(buffer, 0, sizeof(buffer)); // Limpia el bufer después de leer

            char contrasenaRecibida[1024]; //variable donde guardare el usuario enviado por el cliente
            bytesReceived = recv(client, contrasenaRecibida, sizeof(contrasenaRecibida), 0);
            //este condicional es para que no falle la palabra que recibo, estaba recibiendo basura anteriormente
            //garantizo que la cadena no tenga caracteres de mas
            contrasenaRecibida[bytesReceived] = '\0'; //agrego un carácter nulo para marcar el final de la cadena
            std::string contrasenaR(contrasenaRecibida);
            coutLog("Recibí del cliente la contrasena: "+contrasenaR);
            // Ahora puedes usar contrasenaR como un std::string.
            memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de leer.
            //pasamos por la validacion del usuario si es correcto devolvemos el msjs exitoso al cliente
            if(ingresarAltaUsuario(usuarioR, contrasenaR)){
                    send(client, "Usuario Dado de alta correctamente!", sizeof("Usuario Dado de alta correctamente!"), 0);
                    coutLog("---- Alta nueva cargada correctamente ---- por usuario: "+usuario);
                }
        }

        if(opcion == 3){
            // Obtener el conjunto de usuarios bloqueados
            std::set<std::string> usuariosBloqueados = obtenerUsuariosBloqueados();

            // Enviar la cantidad de usuarios bloqueados al cliente
            int numUsuariosBloqueados = usuariosBloqueados.size(); //cuento la cantidad de usuarios bloqueados
            send(client, (char*)&numUsuariosBloqueados, sizeof(numUsuariosBloqueados), 0); // envio esa cantidad

            // Enviar cada usuario bloqueado al cliente
            for (const std::string& usuario : usuariosBloqueados) {
                int usuarioLength = usuario.length();
                send(client, (char*)&usuarioLength, sizeof(usuarioLength), 0); // Enviar la longitud del usuario
                send(client, usuario.c_str(), usuarioLength, 0); // Enviar el usuario en si
            }

            char usuarioADesbloquear[1024];
            int bytesReceived = recv(client, usuarioADesbloquear, sizeof(usuarioADesbloquear), 0);
            //este condicional es para que no falle la palabra que recibo, estaba recibiendo basura anteriormente
            memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de leer.
            //garantizo que la cadena no tenga caracteres de mas
            usuarioADesbloquear[bytesReceived] = '\0'; //agrego un carácter nulo para marcar el final de la cadena
            std::string usuarioB(usuarioADesbloquear);
            coutLog("Recibí del cliente el usuario: " + usuarioB);

            if(usuarioB == "salir"){
                send(client, "Saliste de la seleccion", sizeof("Saliste de la seleccion"), 0);
            }
            //Si el usuario que estoy intentando desbloquear existe entonces solo en ese caso realizamos el seteo de intentos
            if(usuarioBloqueadoExiste(usuariosBloqueados, usuarioB)){
                ResetearIntentos(usuarioB);
                coutLog(usuarioB+" desbloqueado correctamente!");
                std::string mensaje = usuarioB+" desbloqueado correctamente!";
                send(client, mensaje.c_str(), mensaje.length(), 0);
            } else{
                coutLog("El usuario "+usuarioB+" no se encuentra bloqueado");
                std::string mensaje = "El usuario " + usuarioB + " no se encuentra bloqueado";
                send(client, mensaje.c_str(), mensaje.length(), 0);
            }
        }

        if(opcion == 4){
<<<<<<< HEAD
            std::string contenido = traerArchivoLog("server.log.txt");
            send(client, contenido.c_str(), contenido.length(), 0);
=======
            enviarServerLog("server.log.txt");
>>>>>>> f2daa05 (buffer solucionado definitivo, se achico server.log para poder ahcer push)
        }

        if (bytesReceived == SOCKET_ERROR) {
            coutLog("Error al recibir datos del cliente.");
            break; // Salir del bucle si hay un error en la recepción.
        } else if (bytesReceived == 0) {
            CerrarConexion(usuario);
            break; // Salir del bucle si el cliente se ha desconectado.
        }

        if (opcion == 0) {
            CerrarConexion(usuario);
            autenticado = false;
        }
    }

    //Manejo de las solicitudes del usuario "CONSULTA"
    while (autenticado && rol == "CONSULTA") {

        coutLog("Cliente conectado puede usar el menu CONSULTA, esperando palabra para traducir....\n");
        //recibimos del cliente la palabra y la guardamos en una variable para luego compararla
        char palabraTraducir[1024];
        int bytesReceived = recv(client, palabraTraducir, sizeof(palabraTraducir), 0);

        //Como del otro lado estoy enviando un tipo de dato string debo castearlo para luego poder darle un uso correcto.
        palabraTraducir[bytesReceived] = '\0'; //agrego un carácter nulo para marcar el final de la cadena
        std::string palabraATraducir(palabraTraducir); //realizo el casteo
        palabraATraducir = convertirAMinusculas(palabraATraducir); //convierto la palabra a minuscula
        coutLog("Recibí del cliente la contrasena: "+palabraATraducir); // muestro del lado del server la palabra
        // Ahora puedes usar palabraATraducir como un std::string.
        memset(buffer, 0, sizeof(buffer)); // Limpia el búfer después de leer para evitar al repetir la operacion se junte basura.
        std::string traduccion = BuscarTraduccionEnArchivo(palabraATraducir); //realizo la busqueda con la palabra en minuscula y retorno la respuesta
        send(client, traduccion.c_str(), traduccion.length(), 0); //envio la traduccion o la respuesta del metodo de busqueda

        //si se recibe nada o error me salgo del bucle
        if (bytesReceived == SOCKET_ERROR) {
            coutLog("Error al recibir datos del cliente.");
            break; // Salir del bucle si hay un error en la recepción.
        } else if (bytesReceived == 0) {
            CerrarConexion(usuario);
            break; // Salir del bucle si el cliente se ha desconectado.
        }
        //Si recibo del cliente la palabra salir cierro la conexion, paso a false la bandera de salida del bucle while
        if (strcmp(palabraTraducir, "Salir") == 0) {
            CerrarConexion(usuario);
            autenticado = false;
        }
    }
}

///Metodo que se encarga de buscar un usuario y contraseña el cual se reciben por parametro
///la busqueda se realiza en un archivo.txt y en cas de que haya o no coincidencias o algun bloqueo adicional este metodo
///es el encargado de informar al cliente por medio de msjs que es lo que esta sucediendo con la interaccion
///dentro de este tenemos metodos adicionales que se usan para dicha validacion.
bool AutenticarUsuarioYContrasena(std::string usuarioRecibido, std::string contrasenaRecibida) {
    //bandera inicializada en false hasta realizar la busqueda y ver si existe el cliente
    bool autenticado = false;
    std::ifstream archivoCredenciales("credenciales.txt"); // Abre el archivo de credenciales

    if (!archivoCredenciales.is_open()) { // Verifica si el archivo se pudo abrir
        coutLog("No se pudo abrir el archivo de credenciales.");
        return autenticado;
    }

    std::string renglon;
//Mientras haya lineas disponibles en el archivo voy a iterar y las mismas se guardaran en la variable renglon
while (std::getline(archivoCredenciales, renglon)) {
    std::istringstream ss(renglon);//ss se inicializa con el contenido de la linea renglon
    // se declaran las variables que se van a utilizar para verificar las condiciones
    std::string usuario, contrasena, rol;
    int intentos;
    // leemos ss y obtenemos el usuario hasta toparnos con "|" lo mismo para los demas casos
    if (std::getline(ss, usuario, '|') && std::getline(ss, contrasena, '|') && getline(ss, rol, '|')) {
        //una vez obtenidas las variables solo queda comparar, verifica si el usuario coincide
        if (usuario == usuarioRecibido) {
            // Verifica si la contraseña coincide
            if (contrasena == contrasenaRecibida) {
                // Verifica el rol después de la autenticación exitosa
                if (rol == "ADMIN") {
                    autenticado = true;
                    send(client, "ADMIN", sizeof("ADMIN"), 0); //enviamos la palabra que dara lugar al menu corespondiente
                } else if (rol == "CONSULTA") {
                    //si el rol es de tipo consulta vamos a revisar los intentos de este usuario
                    intentos = obtenerIntentosDeUsuario(usuarioRecibido);
                    //si son mayores o igual 3 este no podra ingresar y el cliente recibira el msjs BLOQUEADO
                    if (intentos >= 3) {
                        coutLog("Usuario < " + usuarioRecibido + " > BLOQUEADO!");
                        send(client, "BLOQUEADO", sizeof("BLOQUEADO"), 0); //enviamos la palabra que dara lugar al menu corespondiente
                    } else {
                        //sino autenticamos y pasamos el msjs CONSULTA
                        autenticado = true;
                        send(client, "CONSULTA", sizeof("CONSULTA"), 0); //enviamos la palabra que dara lugar al menu corespondiente
                    }
                }
            } else {
                // Contraseña incorrecta, solo se incrementan los intentos si el rol es CONSULTA
                //aumento solo si es distinto a 3
                if (rol == "CONSULTA" && obtenerIntentosDeUsuario(usuarioRecibido) != 3) {
                    IncrementarIntentos(usuarioRecibido);
                }
                //si son mayores este usuario esta bloqueado asi que volvemos a enviar el msjs de BLOQUEADO
                if(obtenerIntentosDeUsuario(usuarioRecibido) >= 3){
                    coutLog("Usuario < " + usuarioRecibido + " > BLOQUEADO!");
                    send(client, "BLOQUEADO", sizeof("BLOQUEADO"), 0); //enviamos la palabra que dara lugar al menu corespondiente
                }else{
                    send(client, "FALLIDO", sizeof("FALLIDO"), 0); //enviamos la palabra que dara lugar al menu corespondiente
                }
            }
        }
    }
}
//si la persona que quiso ingresar no existe devolvemos otro msjs
if (!autenticado) {
    // Si no se autenticó, el usuario no se encontró en el archivo
    send(client, "FALLIDO", sizeof("FALLIDO"), 0); //enviamos la palabra que dara lugar al menu corespondiente
    }

    return autenticado; // Devuelve el estado de autenticación
}

///Metoto que incrementa el intento del usuario que equivoco su contraseña o no la sabe etc
void IncrementarIntentos(const string& usuario) {
    // Abre el archivo "credenciales.txt" para lectura.
    ifstream archivoCredenciales("credenciales.txt");
    string linea;
    stringstream nuevoContenido; //Este objeto se utiliza para construir el nuevo contenido

    // Itera a traves de las lineas en el archivo de credenciales
    while (getline(archivoCredenciales, linea)) {
        stringstream ss(linea);
        string usuarioArchivo, contrasena, rol;
        int intentos;

        // Divide la línea en sus componentes (usuario, contraseña, rol e intentos)
        if (getline(ss, usuarioArchivo, '|') && getline(ss, contrasena, '|') && getline(ss, rol, '|') && (ss >> intentos)) {

            // Si el usuario de la línea actual coincide con el usuario que se quiere modificar
            if (usuarioArchivo == usuario) {
                intentos++; // Incrementa el contador de intentos
            }

            // Reescribe la línea con los nuevos valores en stringstream
            nuevoContenido << usuarioArchivo << "|" << contrasena << "|" << rol << "|" << intentos << "\n";
        }
    }

    // Cierra el archivo de credenciales
    archivoCredenciales.close();

    // Abre el archivo "credenciales.txt" para escritura y reemplaza su contenido con el nuevo contenido
    ofstream archivoSalida("credenciales.txt");
    archivoSalida << nuevoContenido.str(); // Escribe el nuevo contenido en el archivo
    archivoSalida.close(); // Cierra el archivo
}

///Metodo que se encarga de buscar un determinado usuario recibido por parametro y resetear su atributo de intentos a 0
void ResetearIntentos(const string& usuario) {
    // Abre el archivo "credenciales.txt" para lectura.
    ifstream archivoCredenciales("credenciales.txt");
    string linea;
    stringstream nuevoContenido;

    // Itera a traves de las lineas en el archivo de credenciales
    while (getline(archivoCredenciales, linea)) {
        stringstream ss(linea);
        string usuarioArchivo, contrasena, rol;
        int intentos;

        // Divide la línea en sus componentes (usuario, contraseña, rol e intentos)
        if (getline(ss, usuarioArchivo, '|') && getline(ss, contrasena, '|') && getline(ss, rol, '|') && (ss >> intentos)) {

            // Si el usuario de la línea actual coincide con el usuario que se quiere modificar
            if (usuarioArchivo == usuario) {
                intentos = 0; // Incrementa el contador de intentos
            }

            // Reescribe la línea con los nuevos valores en stringstream
            nuevoContenido << usuarioArchivo << "|" << contrasena << "|" << rol << "|" << intentos << "\n";
        }
    }

    // Cierra el archivo de credenciales
    archivoCredenciales.close();

    // Abre el archivo "credenciales.txt" para escritura y reemplaza su contenido con el nuevo contenido
    ofstream archivoSalida("credenciales.txt");
    archivoSalida << nuevoContenido.str(); // Escribe el nuevo contenido en el archivo
    archivoSalida.close(); // Cierra el archivo
}

///Metodo que se encargara de recibir la palabra para traducir, buscarla y retornar la traduccion
string BuscarTraduccionEnArchivo(const string &palabra) {
    ifstream archivo("traducciones.txt");
    string linea;
    //utilizamos nuevamente el metodo getline para obtener linea por linea o renglon por renglon lo que el archivo.txt tiene
    while (getline(archivo, linea)) {
        size_t pos = linea.find(":"); //buscamos la posicion en la que se encuentra el simbolo ":" para luego dividir la palabra
        if (pos != string::npos) {
            std::string palabraEnIngles = linea.substr(0, pos); //parte ingles
            std::string traduccionEnEspanol = linea.substr(pos + 1); //parte español
            //si existe coincidencia entonces en este caso retornamos la pare en español
            if (palabraEnIngles == palabra) {
                archivo.close();
                return traduccionEnEspanol;
            }
        }
    }
//si no hubo coincidencia retornaremos el msjs de traduccion no encontrada
    archivo.close();
    return "Traducción no encontrada";
}

///Tuve que hacer una funcion para obtener el rol del usuario luego de saber que ya se que existe por el metodo de busqueda anterior.
std::string obtenerRolDeUsuario(const std::string& usuario) {
    //creamos el tipo de variable archivoCredenciales que vamos a leer
    std::ifstream archivoCredenciales("credenciales.txt");
    std::string renglon;
    //si no podemos abrirlo retornamos error
    if (!archivoCredenciales.is_open()) {
        coutLog("No se pudo abrir el archivo de credenciales.");
        return "Error"; // Puedes manejar el error de la manera que desees.
    }
//voy a iterar linea por linea o renglon por renglon para obtener en este caso usuario,contraseña, y rol.
    while (std::getline(archivoCredenciales, renglon)) {
        std::istringstream ss(renglon);
        std::string usuarioArchivo, contrasena, rol;
//si hay coincidencia en el usuario vamos a retornar su rol en formato string
        if (std::getline(ss, usuarioArchivo, '|') && std::getline(ss, contrasena, '|') && getline(ss, rol, '|')) {
            if (usuarioArchivo == usuario) {
                archivoCredenciales.close();
                return rol;
            }
        }
    }
//caso que no haya coincidencias retornaremos el string usuario no encontrado
    archivoCredenciales.close();
    return "Usuario no encontrado"; // caso en que el usuario no existe.
}

///Esta funcion se encarga de buscar y obtener los intentos de un determinado usuario que se recibe por parametro
int obtenerIntentosDeUsuario(const std::string& usuario) {
    int intentos = -1; // Valor predeterminado para indicar que el usuario no fue encontrado

    std::ifstream archivoCredenciales("credenciales.txt");
    std::string renglon;

    if (!archivoCredenciales.is_open()) {
        coutLog("No se pudo abrir el archivo de credenciales.");
        return intentos;
    }

    while (std::getline(archivoCredenciales, renglon)) {
        std::istringstream ss(renglon);
        std::string usuarioArchivo, contrasena, rol;
        int intentosUsuario; // Variable para almacenar los intentos

        // Dividimos la línea en cuatro partes usando el carácter '|'
        if (std::getline(ss, usuarioArchivo, '|') && std::getline(ss, contrasena, '|') && getline(ss, rol, '|') && (ss >> intentosUsuario)) {
            if (usuarioArchivo == usuario) {
                intentos = intentosUsuario; // Obtenemos los intentos del usuario
                break; // Salimos del bucle una vez que encontramos el usuario
            }
        }
    }

    archivoCredenciales.close();
    return intentos; // Retorna el numero de intentos del usuario o -1 si no lo encontro
}

///Metodo que recibe una palabra enviada la cual vamos a revisar si es posible insertar pasando por varias validaciones
///la misma retorna un valor boleano dependiendo si la palabraya existe,ademas de si la palabra tiene un formato correcto.
bool ingresarNuevaTraduccion(string palabraInglesEspaniol){
    //creo la bandera de retorno en false para el caso que falle la insercion
    bool formatoCorrecto = false;
    //convierto a minuscula para que no haya errores en la busqueda o carga de palabras repetidas
    //escriba mayuscula o minuscula el sistema solo aceptara minusculas
    palabraInglesEspaniol = convertirAMinusculas(palabraInglesEspaniol);
    coutLog("Palabra recibida para insertar: "+palabraInglesEspaniol);

    // Verificamos si el formato es valido, en caso que no lo sea retornara falso y un msjs para la persona que interactua con el menu
    if (!esFormatoTraduccionValido(palabraInglesEspaniol)) {
        coutLog("No fue posible insertar la traducción.\nEl formato de inserción debe ser palabraEnInglés:traducciónEnEspañol");
        send(client, "No fue posible insertar la traducción.\nEl formato de inserción debe ser palabraEnInglés:traducciónEnEspañol", sizeof("No fue posible insertar la traducción.\nEl formato de inserción debe ser palabraEnInglés:traducciónEnEspañol"), 0);
        return formatoCorrecto; // Salimos del programa con un código de error
    }

    std::string rutaCompleta = "traducciones.txt"; // Ruta completa del archivo a leer y escribir
    std::set<std::string> traduccionesExistentes = cargarDatosExistentes(rutaCompleta); // Cargar las traducciones existentes

    /*El resultado de find se compara con traduccionesExistentes.end() para verificar si la palabra existe en el conjunto.
    Si find devuelve traduccionesExistentes.end(), significa que la palabra no se encontró en el conjunto  y, por lo tanto,
    no existe en el archivo. Si find no devuelve traduccionesExistentes.end(), significa que
    la palabra ya existe en el conjunto y, por lo tanto, en el archivo "traducciones.txt". */
    if (traduccionesExistentes.find(palabraInglesEspaniol) != traduccionesExistentes.end()) {
        coutLog("La traducción ya existe en el archivo.");
        send(client, "La traducción ya existe en el archivo.", sizeof("La traducción ya existe en el archivo."), 0);
        return formatoCorrecto;
    }

    // Abrimos el archivo en modo de escritura (si no existe, se creará)
    std::ofstream archivo(rutaCompleta, std::ios::app);
    if (archivo.is_open()) {
        archivo <<palabraInglesEspaniol<< endl;
        archivo.close();
        formatoCorrecto = true;
    } else {
        coutLog("No se pudo abrir el archivo.");
        send(client, "No se pudo abrir el archivo para insertar palabra", sizeof("No se pudo abrir el archivo para insertar palabra."), 0);
    }

return formatoCorrecto;

}
///Esta funcion se encarga de buscar los usuarios bloqueados y retornarlos para su uso, en este caso se devuelve una arreglo de usuarios
std::set<std::string> obtenerUsuariosBloqueados() {

    std::string rutaCompleta = "credenciales.txt"; // Ruta completa del archivo a leer y escribir
    std::set<std::string> usuariosBloqueados; // Lugar donde se cargarán los usuarios bloqueados para luego retornar

    std::ifstream archivo(rutaCompleta);

    if (!archivo.is_open()) {
        coutLog("Error al abrir el archivo.");
        return usuariosBloqueados; // Devuelve un conjunto vacío en caso de error.
    }

    std::string renglon;

    while (std::getline(archivo, renglon)) {
        // Busca el indice del primer caracter '|' en la renglon
        size_t indice = renglon.find('|');

        if (indice != std::string::npos) {
            // Extrae el usuario desde el principio hasta el carácter '|'
            std::string usuario = renglon.substr(0, indice);

            // Verifica si el usuario está bloqueado.
            if (obtenerIntentosDeUsuario(usuario) == 3) {
                usuariosBloqueados.insert(usuario);
            }
        }
    }

    archivo.close();
    return usuariosBloqueados;
}

///Metodo que retornara una cadena de caracteres de tipo std:string obtenida del archivo txt de la ruta que se pase por parametro
std::set<std::string> cargarDatosExistentes(const std::string& ruta) {

    std::set<std::string> datosExistentes; // Crear un conjunto local para almacenar las traducciones

    std::ifstream archivo(ruta); //abre el archivo recibido en modo lectura
//si podemos abrir el archivo realizamos el recorrido renglon por renglon para obtener los datos existentes
    if (archivo.is_open()) {
        std::string linea;
        while (std::getline(archivo, linea)) {
            datosExistentes.insert(linea);
        }
        archivo.close();
    }

    return datosExistentes; // Devuelve el conjunto con las traducciones que se encontraron en el archivo
}

///Valida el formato en el que se ingresa una traduccion
bool esFormatoTraduccionValido(const std::string& texto) {
    // Definimos una expresión regular para verificar el formato con solo letras
    regex formato("^[a-zA-Z]+:[a-zA-Z]+$");
    return regex_match(texto, formato); // Retornamos true si coincide con el formato especificado
}

///Metodo para transformar todo lo pasado por parametro a minuscula
std::string convertirAMinusculas(const std::string& texto) {
    std::string resultado = texto; // Crear una copia del texto original
//bucle for que va iterar en cada caracter para convertir este a minuscula
    for (char& c : resultado) {
        c = std::tolower(c); // convertimos cada caracter a minuscula recibiendo este por parametro
    }

    return resultado; //retornamos la palabra ya transformada a minuscula
}

///Metodo que valida el usuario nuevo que se va a ingresar
bool ingresarAltaUsuario(std::string usuario, std::string contrasena){

    //creo la bandera de retorno en false para el caso que falle la insercion
    bool formatoCorrecto = false;
    string usuarioContrasena; // creamos la variable donde se va a alojar los datos recibidos por parametro

    // Verificamos si la contraseña recibida está vacía o contiene solo espacios en blanco
    //Para el caso que la cadena contenga un espacio en blanco o llegue vacia enviaremos el msjs al cliente indicando esto mismo.
    if (contrasena.empty() || contrasena.find_first_not_of(' ') == std::string::npos) {
        coutLog("La contraseña no puede estar vacía o contener solo espacios en blanco!");
        send(client, "La contraseña no puede estar vacía o contener solo espacios en blanco!", sizeof("La contraseña no puede estar vacía o contener solo espacios en blanco!"), 0);
        return formatoCorrecto; //devolvemos false y salimos del metodo en caso de que falle la contraseña
    }

    //si todo salio bien y anteriormente no entramos al if podemos continuar con la insercion
    //convierto a minuscula para que no haya errores en la busqueda o carga de palabras repetidas
    //escriba mayuscula o minuscula el sistema solo aceptara minusculas para el caso usuario, no asi con la contraseña
    usuario = convertirAMinusculas(usuario);
    usuarioContrasena = usuario+"|"+contrasena; // ahora si podemos concatenar los string quedando usuario|contraseña

    //A continuacion vamos a traer todos los usuarios para ver si tenemos coincidencia con el que se esta queriendo insertar
    std::string rutaCompleta = "credenciales.txt"; // Ruta del archivo para poder leer y escribir
    std::set<std::string> usuariosExistentes = cargarDatosExistentes(rutaCompleta); // traemos las credenciales existentes

    // Verificamos si el usuario ya existe en el archivo para esto hacemos uso del metodo usuarioExiste, devolvera true o false
    //dependiendo el caso correspondiente, para funcionar necesita un arreglo de usuarios y el usuario que debe buscar
    if (usuarioExiste(usuariosExistentes, usuario)) {
        coutLog("El usuario ya existe en el archivo.");
        send(client, "El usuario ya existe en el archivo.", sizeof("El usuario ya existe en el archivo."), 0);
        return formatoCorrecto; //salimos en el caso que haya coincidencia retornando la bandera y arriba el msjs para el cliente
    }
    //Si para este momento el metodo no devolvio la bandera antes podemos pasar a abrir el archivo y escribir este nuevo usuario
    // Abrimos el archivo en modo de escritura (si no existe, se creará)
    std::ofstream archivo(rutaCompleta, std::ios::app); // el segundo argumento determina que lo que se agregue vaya al final
    if (archivo.is_open()) {
        archivo <<usuarioContrasena+"|CONSULTA|0"<< endl; //ahora insertamos lo recibido y le concatenamos el resto (tipo|intentos)
        archivo.close();
        formatoCorrecto = true;  //cambiamos la bandera ya que todo salio ok
    } else {
        //caso contrario enviamos al cliente el msjs correspondiente
        coutLog("No se pudo abrir el archivo.");
        send(client, "No se pudo abrir el archivo para insertar palabra", sizeof("No se pudo abrir el archivo para insertar palabra."), 0);
    }

return formatoCorrecto;

}

///Metodo que recorre el array que contiene los usuarios existentes, se precisa por parametro el array y ademas el usuario que se busca
bool usuarioExiste(const std::set<std::string>& usuariosExistentes, const std::string& nuevoUsuario) {

    bool existe = false; //creamos la bandera establecida en false inicialmente
    for (const std::string& entrada : usuariosExistentes) {
        // Buscamos la posición de "|" en la entrada
        size_t posicionBarra = entrada.find('|'); //si el metodo find no encuentra este simbolo devolvera std::string::npos
        //revisamos si se encontro este simbolo, si se hizo entonces en esa posicion vamos a obtener solo el usuario
        if (posicionBarra != std::string::npos) {
            // Extraemos la primera palabra antes de "|"
            std::string usuarioExistente = entrada.substr(0, posicionBarra);

            // Comparamos el usuario existente con el nuevo usuario
            if (usuarioExistente == nuevoUsuario) {
                existe = true; // El usuario ya existe cambiamos la bandera a true
            }
        }
    }
    return existe; // Retornamos la bandera sea false o true dependiendo lo que haya sucedido
}

///Metodo que se encarga de informar si el usuario recibido existe en el archivo de credenciales
bool usuarioBloqueadoExiste(const std::set<std::string>& usuariosExistentes, const std::string& usuarioADesbloquear) {
    bool existe = false;
    //iteramos con for each guardando cada usuario del arreglo de usuarios para luego ver si hay coincidencia con el recibido
    for (const std::string& usuario : usuariosExistentes) {
            // Comparamos el usuario existente con el nuevo usuario
            if (usuario == usuarioADesbloquear) {
                existe = true; // El usuario ya existe pasamos la bandera a verdadera
            }
        }

    return existe; // El usuario no existe
}

<<<<<<< HEAD
///Metodo que se encarga de abrir un determinado archivo y leer su contenido linea po linea
std::string traerArchivoLog(const std::string &nombreArchivo) {
=======
///Metodo que se encarga de abrir un determinado archivo y leer su contenido para enviarselo linea por linea al cliente.
void enviarServerLog(const std::string &nombreArchivo) {
>>>>>>> f2daa05 (buffer solucionado definitivo, se achico server.log para poder ahcer push)
    // Abre el archivo
    std::ifstream archivo(nombreArchivo);

    // Verifica si se pudo abrir el archivo
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo: " << nombreArchivo << std::endl;
<<<<<<< HEAD
        return "";
    }

    // Lee el contenido del archivo línea por línea
    std::string contenido;
    std::string linea;
    //mientras se pueda leer el archivo vamos a concatenar todo su contenido
    while (std::getline(archivo, linea)) {
        contenido += linea + "\n";
    }

    // Cierra el archivo
    archivo.close();

    return contenido; //retornamos todo lo obtenido
=======
        return;
    }

    // Variable para almacenar una linea del archivo
    std::string linea;

    // Bucle para leer y enviar el contenido linea por linea
    while (std::getline(archivo, linea)) {
            linea = linea+"\n";
        // Envio la linea al cliente
        int bytesEnviados = send(client, linea.c_str(), linea.length(), 0);

        // Reviso si sigue enviando datos
        if (bytesEnviados == -1) {
            std::cerr << "Error al enviar datos al cliente" << std::endl;
            break;
        }
    }
    // Cierra el archivo
    archivo.close();
>>>>>>> f2daa05 (buffer solucionado definitivo, se achico server.log para poder ahcer push)
}

};
