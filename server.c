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
#include <stdlib.h>        
#include <unistd.h>


#define TAILLE_MAX_NOM 256
#define MAX_CLIENTS 100 // Nombre max de clients supportés par l'application


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
	int connected;
    char channel[100];
} Client;

// Déclaration des procédures et fonction:
void commands(Client * client);
void leave(Client * client);
void printConnectedClients(Client * client);
static void * renvoi (void * sender);
void sendWhisper(Client * client, char destination[], char buffer[]);
void sendMessageClient(Client * client, char buffer[]);
void sendMessageServer(Client * client, int arrive, int leave);
void welcomeMessage (Client * client);
void byeMessage (Client * client);

// Compteur du nombre de clients connectés au server
int nbClientsConnected = 0;

// Création d'un tableau de Clients dans lequel on va stocker les clients qui sont connectés
Client clientsLoggedIn[MAX_CLIENTS];


/*------------------------------------------------------*/
// Gestion des messages envoyés au serveur par les clients
static void * renvoi (void * sender){
    Client * client = (Client *) sender;
    char buffer[256];
    int length;


    // Tant que le client n'a pas défini son pseudo il reste en mode saisie
    while(strlen((*client).pseudo)<=1){
        length = read((*client).socket, buffer, sizeof(buffer));
        buffer[length]='\0'; 
        strcpy((*client).pseudo, buffer);
        write(1,buffer,length);
    }

    welcomeMessage(client);
    
    while(1){
        length = read((*client).socket, buffer, sizeof(buffer));
        buffer[length]='\0';

        // On extrait l'action, le destinataire et le message afin de savoir si le client veut 
        // envoyer un message sur le channel général ou chuchoter à un autre client
        char transition[sizeof(buffer)];
        strcpy(transition,buffer);
        char *action;
        action = strtok(buffer," ");
        char *destination;
        destination = strtok(NULL," ");
        char *message;
        message = strtok(NULL,"");

        if(strcmp(buffer, "/leave")==0){
            //Si le client souhaite quitter le chat
            leave(client);

        }else if (strcmp(buffer,"/join")==0){
            

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
            sendMessageClient(client, transition);  
        }
    }
}

int findClient(Client * client){
    int trouve, pos;
    trouve = 0;
    pos = 9999;

    for (int i = 0; i < nbClientsConnected; ++i)
    {
        if (strcmp(clientsLoggedIn[i].pseudo,client->pseudo)==0)
        {
            trouve = 1;
            pos = i;
        }
    }

    return pos;
}

int findPositionPseudo(char client[]){
    int trouve, pos;
    trouve = 0;
    pos = 9999;

    for (int i = 0; i < nbClientsConnected; ++i)
    {
        if (strcmp(clientsLoggedIn[i].pseudo,client)==0)
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
    pos = findClient(client);

    // Si on l'a trouvé on va afficher le message de commandes
    if (pos != 9999)
    {
        strcpy(message,"\nVoici les commandes du chat:\n");
        strcat(message,"'/who' pour savoir qui est connecté\n");
        strcat(message,"'/w destinataire message' pour chuchoter un message à un destinataire\n");
        strcat(message,"'/leave' pour quitter \n");
        strcat(message,"'/cmd' pour un rappel des commandes\n");
        strcat(message,"-----------------------------------\n\n");
        write(clientsLoggedIn[pos].socket,message,strlen(message)+1);
    }  
}

void leave(Client * client){
    
    byeMessage(client);
    
    int pos;
    pos = findClient(client);

    if (pos != 9999)
    {
        clientsLoggedIn[pos].connected = 0;
        close((*client).socket);
        pthread_exit(NULL);
    }               
}


void printConnectedClients(Client * client){
    char *message;
    int pos;

    pos = findClient(client);

    // Si on l'a trouvé on va afficher tous les pseudos des clients connectés au client qui le souhaite
    if (pos != 9999)
    {
        strcpy(message,"Les clients connectés actuellement sont:\n");
        for (int j = 0; j < nbClientsConnected; ++j)
        {
            if (clientsLoggedIn[j].connected == 1)
            {
                strcat(message,clientsLoggedIn[j].pseudo);
                strcat(message,"\n");
            }
        }
        write(clientsLoggedIn[pos].socket,message,strlen(message)+1);
    }  
}


void sendWhisper(Client * client, char destination[], char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat
    strcpy(message, (*client).pseudo);
    strcat(message," vous chuchote : ");
    strcat(message,buffer);
    printf("%s\n", message);

    int pos;
    pos = findPositionPseudo(destination);
   
    if (pos != 9999 && (clientsLoggedIn[pos].connected == 1))
    {
        write(clientsLoggedIn[pos].socket,message,strlen(message)+1);
    }
}


void sendMessageClient(Client * client, char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat
    strcpy(message, (*client).pseudo);
    strcat(message," dit : ");
    strcat(message,buffer);
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(strcmp((*client).pseudo,clientsLoggedIn[i].pseudo)!=0 && (clientsLoggedIn[i].connected == 1)){
            if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
            } 
        }     
    }    
}


void welcomeMessage (Client * client){
    int randomNumber = rand() % 5;
    char *message = malloc (sizeof (*message) * 256);

    switch(randomNumber){
        case 0 : 
            strcpy(message, (*client).pseudo);
            strcat(message, "est là, comme la prophécie l'a décrit.");             
        break;

        case 1 : 
            strcpy(message, "Bienvenue, ");
            strcat(message, (*client).pseudo);
            strcat(message, ". On éspère que t'as apporté de la pizza.");
        break;
        
        case 2 :  
            strcpy(message, "Joie, félicitée ! ");
            strcat(message, (*client).pseudo);
            strcat(message, " est là !");
        break;
        
        case 3 : 
            strcpy(message, "C'est un oiseau ! C'est un avion ! Non, en fait c'est juste ");
            strcat(message, (*client).pseudo);
            strcat(message, ".");
        break;
        
        case 4 :  
            strcpy(message, (*client).pseudo);
            strcat(message, "est arrivé. Finit de jouer.");  
        break;
        
        case 5 : 
            strcpy(message, (*client).pseudo);
            strcat(message, "nous a rejoin. Vite, tout le monde fait semblant d'être occupé !"); 
        break;
    }

    
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(strcmp((*client).pseudo,clientsLoggedIn[i].pseudo)!=0 && (clientsLoggedIn[i].connected == 1)){
            if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
            } 
        }     
    } 
}

void byeMessage (Client * client){
    int randomNumber = rand() % 5;
    char *message = malloc (sizeof (*message) * 256);

    switch(randomNumber){
        case 0 : 
            strcpy(message, (*client).pseudo);
            strcat(message, " nous a quitté. RIP.");             
        break;

        case 1 : 
            strcpy(message, "Tu nous quitte déjà ");
            strcat(message, (*client).pseudo);
            strcat(message, " ? :(");
        break;
        
        case 2 :  
            strcpy(message, "Adieu ");
            strcat(message, (*client).pseudo);
            strcat(message, ", on se reverra de l'autre côté.");
        break;
        
        case 3 : 
            strcpy(message, "Tu pars sans dire au revoir ");
            strcat(message, (*client).pseudo);
            strcat(message, " ?");
        break;
        
        case 4 :  
            strcpy(message, (*client).pseudo);
            strcat(message, "est parti(e). C'est bon, on peut recommencer à parler dans son dos.");  
        break;
        
        case 5 : 
            strcpy(message, (*client).pseudo);
            strcat(message, " a préféré rejoindre un autre serveur... Zut !"); 
        break;
    }

    
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(strcmp((*client).pseudo,clientsLoggedIn[i].pseudo)!=0 && (clientsLoggedIn[i].connected == 1)){
            if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
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

    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
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
		if (nbClientsConnected >= MAX_CLIENTS) {
			perror("Serveur plein, nombre max de clients atteint");
			exit(1);
		}
		else{
			if(clientsLoggedIn[nbClientsConnected].connected == 0){
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
					clientsLoggedIn[nbClientsConnected].connected = 1;
					clientsLoggedIn[nbClientsConnected].pseudo[0] = '\0';
					clientsLoggedIn[nbClientsConnected].socket = nouv_socket_descriptor;
					pthread_create(&clientsLoggedIn[nbClientsConnected].thread, NULL, renvoi, &clientsLoggedIn[nbClientsConnected]);
					nbClientsConnected++;
				}
			}
		}
	}

	return 0;
    
}

