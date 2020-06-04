#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
	Creamos una estructura de datos que obligatoriamente su primer atributo deba ser un long para identificar el tipo de mensajes,
	esto le sirve a la cola de mensajes para que el proceso que reciba pueda recibir mensajes de un tipo específico.
	Los demás atributos son decisión del programador, en este caso quise que el buffer contenga los PID de los procesos que envian y reciben mensajes.
	Y un campo para el mensaje en sí.
*/

typedef struct messageBuffer {
	long mtype;
	int sender;
	int receiver;
	char message;
} msg_t;

int main()
{
	// Instancio el buffer
	msg_t message;
	// Instancio la key que será única para comunicar los procesos a una única cola de mensajes.
	key_t msgkey;
	// Este int será el identificador de la cola de mensajes, luego de crearla se puede referir a este id para enviar y recibir mensajes a la cola.
	int msgqueue_id;
	// Dos identificadores de PID para los procesos que envia y recibe.
	pid_t sender, receiver;

	// La función ftok recibe dos parametros: un fichero que sea accesible para todos los procesos y un entero.
	// Nos va a retornar una clave única.
	msgkey = ftok("key", 22);
	// Creamos la cola de mensajes, la función msgget recibe la key común entre los procesos y luego unos bits que indican si la cola debe crearse si no existe
	// y si existe utilizar la misma, junto a bits de permisos para los procesos.
	msgqueue_id = msgget(msgkey, IPC_CREAT | 0660);
	
	//Genero un nuevo proceso
	sender = fork();
	
	// Si soy el proceso hijo voy a enviar el mensaje.
	if(sender == 0) {
		printf("Soy el proceso con PID%d y voy a enviar un mensaje a la cola con id %d\n", getpid(), msgqueue_id);
		// Asigno el tipo de mensaje que será.
		message.mtype = 1;
		// Al buffer en su campo de sender le paso el PID del proceso que envia.
		message.sender = getpid();
		// Seteo el mensaje.
		message.message = 'm';
		// Utilizo la función msgsnd para enviar un mensaje a la cola, la función recibe cuatro parametros:
		// el id de la cola, el mensaje en sí (la estructura de datos que se castea para que cumpla los requisitos de la función),
		// el tercer parametro seria la cantidad de bytes del mensaje restandole los bytes del long (esto lo vi en varios ejemplos)
		// y el ultimo parametro 0, le indica a la funcion que debe esperar si no puede enviar el mensaje a la cola (si es que está llena).
		// Esto nos facilita una herramienta de sincronización entre los dos procesos ya que el proceso que envia mensaje se va a bloquear hasta
		// que pueda enviarlo y luego el proceso que recibe se bloquea hasta que pueda recibir un mensaje desde la cola.
		msgsnd(msgqueue_id, &message, sizeof(message) - sizeof(long), 0);
		printf("Mensaje enviado\n");
		exit(0);
	} else {
		wait(NULL);
		// En el proceso padre vuelvo a crear un proceso, esta vez el proceso que va a recibir el mensaje.
		receiver = fork();
		if(receiver == 0) {
			printf("Soy el proceso con PID%d y voy a recibir un mensaje de la cola con id %d\n", getpid(), msgqueue_id);
			// Con la función msgrcv el proceso va a recibir el mensaje, recibe cuatro parametros:
			// el id de la cola de mensajes, la estructura de datos del mensaje, el tamaño de bytes,
			// el tipo de mensaje que va a recibir (es decir, el primer campo del buffer en este caso es 1)
			// y luego una señal que le indica al proceso que debe bloquearse hasta que pueda recibir un mensaje.
			msgrcv(msgqueue_id, &message, sizeof(message) - sizeof(long), 1, 0);
			message.receiver = getpid();
			printf("Mensaje recibido\n");
			printf("Mensaje recibido: %c. Del proceso: %d. Al proceso: %d.\n", message.message, message.sender, message.receiver);
			// Luego de que recibi el mensaje destruyo la cola.
			msgctl(msgqueue_id, IPC_RMID, 0);
			exit(0);
		}
		wait(NULL);
	}
	return 0;
}
