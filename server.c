/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <netdb.h> 		    /* pour hostent, servent */
#include <string.h> 		/* pour bcopy, ... */ 
#include <pthread.h> 
//rajouté
#include <stdlib.h>        
#include <unistd.h>


#define TAILLE_MAX_NOM 256
//rajouté
#define MAX_CLIENTS 42 // Nombre max de clients supportés par l'application


typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;


// Structure associée aux clients connectés sur le serveur 
//rajouté
typedef struct
{
	int socket;
	char pseudo[30];
	int is_connected;
	pthread_t thread;
} Client;

// Nombre de clients connectés
//Rajouté
int nb_clients = 0;


// On initialise un tableau contenant les clients connectés
//rajouté
Client listClients[MAX_CLIENTS];

//rajouté
int run;


/*------------------------------------------------------*/

// Envoi d'un message aux clients du serveur
//rajouté
void send_message(Client * client, char buffer[]){
    char *answer = malloc (sizeof (*answer) * 256);
    strcpy(answer, (*client).pseudo);
    strcat(answer,"-> ");
    strcat(answer,buffer);
    printf("%s\n", answer);
    int i;
    for (i=0;i<nb_clients;i++){
        if(strcmp((*client).pseudo,listClients[i].pseudo)!=0){
            if((write(listClients[i].socket,answer,strlen(answer)+1)) < 0){
                perror("erreur : impossible d'ecrire le message destine au serveur.");
                exit(1);
            } 
        }     
    }    
}

// Seconde fonction d'envoi de message aux clients 
//rajouté
void send_message_2(char buffer[]){
    char *answer = malloc (sizeof (*answer) * 256);
    strcat(answer,buffer);
    printf("%s\n", answer);
    int i;
    for (i=0;i<nb_clients;i++){
            write(listClients[i].socket,answer,strlen(answer)+1); 
    }    
}


// Affichage des joueurs connectés au chat
//rajouté
void afficher_joueurs(){
	int i = 0;
	for(i; i<nb_clients; i++){
		printf("\n");
		send_message_2(listClients[i].pseudo);
	}
}

// Affichage au sein du serveur des informations relatives aux clients connectés
//rajouté
void infos_client(){
    int i = 0;
    for(i; i<nb_clients; i++){
        printf("\nUtilisateur %i\n", i);
        printf("Pseudo: %s\n", listClients[i].pseudo);
        printf("Socket: %i\n", listClients[i].socket);
        printf("Connecté: %i\n", listClients[i].is_connected);
    }
}

	



// Gestion des messages envoyés au serveur par les clients
//rajouté
static void * control (void * cli){
	Client * client = (Client *) cli;
	char buffer[256];
	char *rep = malloc (sizeof (*rep) * 256);
	int length;

	// Le client n'a pas défini son pseudo
    while(strlen((*client).pseudo)<=1){
        length = read((*client).socket, buffer, sizeof(buffer));
        sleep(3);
        buffer[length]='\0'; 
        strcpy((*client).pseudo, buffer);
        write(1,buffer,length);
    }
	
	while(1){
		length = read((*client).socket, buffer, sizeof(buffer));
		buffer[length]='\0';
		sleep(2);
		
		//Le client souhaite quitter le chat
		if(strcmp(buffer, "quit")==0){
			printf("%s quitte le chat\n", (*client).pseudo);
			close((*client).socket);
			pthread_exit(NULL);
		}
		
			
		//Envoi d'un message dans le chat
	    else if(length > 0){
            send_message(client, buffer);	
    	}

	}
}
	

	
/*------------------------------------------------------*/

/*------------------------------------------------------*/
main(int argc, char **argv) {
  
    int 		socket_descriptor, 		/* descripteur de socket */
			nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
			longueur_adresse_courante; 	/* longueur d'adresse courante d'un client */
    sockaddr_in 	adresse_locale, 		/* structure d'adresse locale*/
			adresse_client_courant; 	/* adresse client courant */
    hostent*		ptr_hote; 			/* les infos recuperees sur la machine hote */
    servent*		ptr_service; 			/* les infos recuperees sur le service de la machine */
    char 		machine[TAILLE_MAX_NOM+1]; 	/* nom de la machine locale */
    
    gethostname(machine,TAILLE_MAX_NOM);		/* recuperation du nom de la machine */
    
    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
    }
    
    /* initialisation de la structure adresse_locale avec les infos recuperees */			
    
    /* copie de ptr_hote vers adresse_locale */
    bcopy((char*)ptr_hote->h_addr, (char*)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family		= ptr_hote->h_addrtype; 	/* ou AF_INET */
    adresse_locale.sin_addr.s_addr	= INADDR_ANY; 			/* ou AF_INET */

    /* 2 facons de definir le service que l'on va utiliser a distance */
    /* (commenter l'une ou l'autre des solutions) */
    
    /*-----------------------------------------------------------*/
    /* SOLUTION 1 : utiliser un service existant, par ex. "irc" */
    /*
    if ((ptr_service = getservbyname("irc","tcp")) == NULL) {
        perror("erreur : impossible de recuperer le numero de port du service desire.");
        exit(1);
    }
    adresse_locale.sin_port = htons(ptr_service->s_port);
    */
    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
    //rajouté 5001 au lieu de 5000
    adresse_locale.sin_port = htons(5001);
    /*-----------------------------------------------------------*/
    
    printf("numero de port pour la connexion au serveur : %d \n", 
		   ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/);
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
    }

    /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }
    

    /* initialisation de la file d'ecoute */
    listen(socket_descriptor,5);


    /* attente des connexions et traitement des donnees recues */
    for(;;) {
    
		longueur_adresse_courante = sizeof(adresse_client_courant);
		
		/* adresse_client_courant sera renseigné par accept via les infos du connect */
		// On vérifie que le nombre max de clients n'est pas atteint
		//rajouté à la place de la procédure en dessous
		if (nb_clients >= MAX_CLIENTS) {
			perror("Serveur plein, nombre max de clients atteint");
			exit(1);
		}
		else{
			if(listClients[nb_clients].is_connected == 0){
				if ((nouv_socket_descriptor = 
					accept(socket_descriptor, 
						   (sockaddr*)(&adresse_client_courant),
						   &longueur_adresse_courante))
					 < 0) {
					perror("erreur : impossible d'accepter la connexion avec le client.");
					exit(1);
				}
				// Un thread est crée pour le client
				else{
					listClients[nb_clients].is_connected = 1;
					listClients[nb_clients].pseudo[0] = '\0';
					listClients[nb_clients].socket = nouv_socket_descriptor;
					pthread_create(&listClients[nb_clients].thread, NULL, control, &listClients[nb_clients]);
					nb_clients++;
				}
			}
		}

		/* adresse_client_courant sera renseigné par accept via les infos du connect 
        if ((nouv_socket_descriptor = 
            accept(socket_descriptor, 
                   (sockaddr*)(&adresse_client_courant),
                   &longueur_adresse_courante))
             < 0) {
            perror("erreur : impossible d'accepter la connexion avec le client.");
            exit(1);
        }
        
        // traitement du message 
        printf("reception d'un message.\n");
        
        renvoi(nouv_socket_descriptor);
                        
        close(nouv_socket_descriptor); */
	}

	//rajouté	
	return 0;
    
}
