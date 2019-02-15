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


// Structure client qui permet de stocker le pseudo, le socket et le thread du client
// Un booléen "connected" permet de savoir si le client est bien connecté
typedef struct
{
	char pseudo[50];
    int socket;
    pthread_t thread;
	int connected; // voir si on peut l'enlever
} Client;

// Déclaration des procédures et fonction:
void commands(Client * client);
void leave(Client * client);
void printConnectedClients(Client * client);
static void * renvoi (void * sender);
void sendWhisper(Client * client, char destination[], char buffer[]);
void sendMessageClient(Client * client, char buffer[]);

// Compteur du nombre de clients connectés au server
//Rajouté
int nbClientsConnected = 0;

// Création d'un tableau de Clients dans lequel on va stocker les clients qui sont connectés
//rajouté
Client arrClientsConnected[MAX_CLIENTS];


/*------------------------------------------------------*/
// Gestion des messages envoyés au serveur par les clients
//rajouté
static void * renvoi (void * sender){
    Client * client = (Client *) sender;
    char buffer[256];
    int length;

    // Le client n'a pas défini son pseudo
    while(strlen((*client).pseudo)<=1){
        length = read((*client).socket, buffer, sizeof(buffer));
        //sleep(3);
        buffer[length]='\0'; 
        strcpy((*client).pseudo, buffer);
        write(1,buffer,length);
    }
    
    while(1){
        length = read((*client).socket, buffer, sizeof(buffer));
        buffer[length]='\0';
        //sleep(2);
        
        // On extrait l'action, le destinataire et le message afin de savoir si le client veut 
        // envoyer un message sur le channel général ou chuchoter à un autre client
        char *action;
        action = strtok(buffer," ");
        char *destination;
        destination = strtok(NULL," ");
        char *message;
        message = strtok(NULL,"");

        if(strcmp(buffer, "/leave")==0){
            //Si le client souhaite quitter le chat
            leave(client);

        }else if (strcmp(buffer,"/who")==0){
            //Si le client souhaite savoir qui est connecté
            printConnectedClients(client);

        }else if (strcmp(action,"/w")==0){
            //Si le client souhaite chuchoter à un autre client
            sendWhisper(client, destination, message);

        }else if (strcmp(buffer,"/cmd")==0){
            //Si le cliend souhaite un rappel des commandes du chat
            commands(client);

        }else if(length > 0){
            //Si le client souhaite envoyer un message sur le channel général
            sendMessageClient(client, buffer);  
        }
    }
}

int findPos(Client * client){
    int trouve, pos;
    trouve = 0;
    pos = 9999;

    for (int i = 0; i < nbClientsConnected; ++i)
    {
        if (strcmp(arrClientsConnected[i].pseudo,client->pseudo)==0)
        {
            trouve = 1;
            pos = i;
        }
    }

    return pos;
}

void commands(Client * client){
    char *message;
    int pos;
    pos = findPos(client);

    // Si on l'a trouvé on va afficher le message de commandes
    if (pos != 9999)
    {
        strcpy(message,"Voici les commandes du chat:\n");
        strcat(message,"/who' pour savoir qui est connecté\n");
        strcat(message,"'/w destinataire message' pour chuchoter à un autre utilisateur\n");
        strcat(message,"'/leave' pour quitter \n'");
        strcat(message,"'/cmd' pour un rappel des commandes\n\n");
        write(arrClientsConnected[pos].socket,message,strlen(message)+1);
    }  
}

void leave(Client * client){
    printf("%s est parti du chat\n", (*client).pseudo);
    close((*client).socket);
    pthread_exit(NULL);
}


void printConnectedClients(Client * client){
    char *message;
    int pos;

    pos = findPos(client);

    // Si on l'a trouvé on va afficher tous les pseudos des clients connectés au client qui le souhaite
    if (pos != 9999)
    {
        strcpy(message,"Les clients connectés sont:\n");
        for (int j = 0; j < nbClientsConnected; ++j)
        {
            strcat(message,arrClientsConnected[j].pseudo);
            strcat(message,"\n");
        }
        write(arrClientsConnected[pos].socket,message,strlen(message)+1);
    }  
}

// Procédure qui permet d'envoyer un message sur la console des clients
//rajouté
void sendWhisper(Client * client, char destination[], char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat
    strcpy(message, (*client).pseudo);
    strcat(message," vous chuchote : ");
    strcat(message,buffer);
    printf("%s\n", message);

    int pos;
    pos = findPos(client);
   
    if (pos != 9999)
    {
        write(arrClientsConnected[pos].socket,message,strlen(message)+1);
    }

}

// Procédure qui permet d'envoyer un message de chuchotement à un client
//rajouté
void sendMessageClient(Client * client, char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat
    strcpy(message, (*client).pseudo);
    strcat(message," dit : ");
    strcat(message,buffer);
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(strcmp((*client).pseudo,arrClientsConnected[i].pseudo)!=0){
            if((write(arrClientsConnected[i].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
            } 
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
    adresse_locale.sin_port = htons(5000);
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
		if (nbClientsConnected >= MAX_CLIENTS) {
			perror("Serveur plein, nombre max de clients atteint");
			exit(1);
		}
		else{
			if(arrClientsConnected[nbClientsConnected].connected == 0){
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
					arrClientsConnected[nbClientsConnected].connected = 1;
					arrClientsConnected[nbClientsConnected].pseudo[0] = '\0';
					arrClientsConnected[nbClientsConnected].socket = nouv_socket_descriptor;
					pthread_create(&arrClientsConnected[nbClientsConnected].thread, NULL, renvoi, &arrClientsConnected[nbClientsConnected]);
					nbClientsConnected++;
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

