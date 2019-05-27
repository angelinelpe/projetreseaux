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
#define MAX_CLIENTS 100 
#define MAX_CHANNELS 100  


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

typedef struct 
{
    char channel[100];
    char password[100];
} Channel;

// Déclaration des procédures et fonction:
void commands(Client * client);
void leave(Client * client);
void printConnectedClients(Client * client);
int findClient(Client * client);
static void * serverManager (void * sender);
void sendWhisper(Client * client, char destination[], char buffer[]);
void sendMessageClient(Client * client, char buffer[]);
void sendMessageServer(Client * client, int arrive, int leave);
void welcomeMessage (Client * client);
void byeMessage (Client * client);
void join(Client * client, char channel[], char password[]);
int comparePassword(int position, char password[]);
int findChannel(char channel[]);
int chanExists(char channel[]);
void sendToChan(Client * client, char destination[], char buffer[]);
int connectedToChan(Client * client, char channel[]);
int pseudoExists(char pseudo[]);


// Compteur du nombre de clients connectés au server
int nbClientsConnected = 0;
int nbChannels = 0;

// Création d'un tableau de Clients dans lequel on va stocker les clients qui sont connectés
Client clientsLoggedIn[MAX_CLIENTS];
Channel channels[MAX_CHANNELS];


/*------------------------------------------------------*/
// Gestion des messages envoyés au serveur par les clients
static void * serverManager (void * sender){
    Client * client = (Client *) sender;
    char buffer[256];
    int length;
    int exists;


    // Tant que le client n'a pas défini son pseudo il reste en mode saisie
    while(strlen((*client).pseudo)<=1){
        length = read((*client).socket, buffer, sizeof(buffer));
        buffer[length]='\0'; 
        exists = pseudoExists(buffer);
        if (exists == 1){
            char *message = malloc (sizeof (*message) * 256);
            strcpy(message,"Ce pseudo est déjà prit ! Merci d'en saisir un autre.");
            if((write((*client).socket,message,strlen(message)+1)) < 0){
                        perror("Erreur: le message n'a pas été transmit");
                        exit(1);
                } 

        } else {
            strcpy((*client).pseudo, buffer);
            welcomeMessage(client);
        }

    }


    
    for(;;){
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

        }else if (strcmp(action,"/join")==0){
            //Si le client souhaire rejoindre un channel
            join(client, destination, message);

        }else if (strcmp(buffer,"/who")==0){
            //Si le client souhaite savoir qui est connecté
            printConnectedClients(client);

        }else if (strcmp(action,"/w")==0){
            //Si le client souhaite chuchoter à un autre client
            sendWhisper(client, destination, message);

        } else if (strcmp(action,"/chan")==0){
            //Si le client souhaite parler dans le chan
            sendToChan(client,destination,message);

        }else if (strcmp(buffer,"/cmd")==0){
            //Si le cliend souhaite un rappel des commandes du chat
            commands(client);

        }else if(length > 0){
            //Si le client souhaite envoyer un message sur le channel général
            sendMessageClient(client, transition);  
        }
    }
}


int pseudoExists(char pseudo[]){
    int equal;
    equal = 0;
    for (int i = 0; i < nbClientsConnected; ++i){
        if (strcmp(pseudo,clientsLoggedIn[i].pseudo)==0){
            equal = 1;
        }
    }

    return equal;
}

void sendToChan(Client * client, char destination[], char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat

    //test si chan existe
    int exists;
    exists = chanExists(destination);

    if (exists == 1){   
        int connected;
        connected = connectedToChan(client,destination);
        if (connected == 1){
            strcpy(message, (*client).pseudo);
            strcat(message," au channel ");
            strcat(message, destination);
            strcat(message," : ");
            strcat(message,buffer);
            printf("%s\n", message);
            for (int i=0;i<nbClientsConnected;i++){
                if(strcmp((*client).pseudo,clientsLoggedIn[i].pseudo)!=0 && (clientsLoggedIn[i].connected == 1) && strcmp(clientsLoggedIn[i].channel,destination)==0){
                    if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
                        perror("Erreur: le message n'a pas été transmit");
                        exit(1);
                    } 
                }     
            }           
        } else {
            strcpy(message, "Erreur, vous n'êtes pas dans le channel.");
            printf("%s\n", message);

            int pos;
            pos = findClient(client);

            if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
                if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
                    perror("Erreur: le message n'a pas été transmit");
                    exit(1);
                }
            }
        }   
    } else {        
        strcpy(message, "Erreur,le channel n'existe pas.");
        printf("%s\n", message);

        int pos;
        pos = findClient(client);

        if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
            if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
            }
        }
    }
}

int connectedToChan(Client * client, char channel[]){
    int pos;
    pos = findClient(client);

    if (pos != 9999 && strcmp(clientsLoggedIn[pos].channel, channel)==0){
        return 1;
    } else return 0;
}

void join(Client * client, char chan[], char pwd[]){
    int exists;
    exists = chanExists(chan);

    int pos;
    pos = findClient(client);

    int enter;
    enter = 0;

    char *message = malloc (sizeof (*message) * 256);

    if (exists == 0){ 
        strcpy(channels[nbChannels].channel, chan);
        strcpy(channels[nbChannels].password, pwd);
        nbChannels ++;  
        enter = 1;
    
    }else {
        int posChannel;
        posChannel = findChannel(chan);
        int passwordOk;
        passwordOk = comparePassword(posChannel, pwd);

        if (passwordOk == 0){           
            
            if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
                strcpy(message, "Mot de passe incorrect");
                if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
                    perror("Erreur: le message n'a pas été transmit");
                    exit(1);
                } 
            }      

        } else { 
            enter = 1;
        }

    }   
    if (enter == 1){
        strcpy(message," Vous entrez dans le channel ");
        strcat(message,chan);

        //Envoi du message au client 
       
        if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
            if ((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                    exit(1);
            }   
        }

        strcpy(message, (*client).pseudo);
        strcat(message," a rejoint le channel ");
        strcat(message,chan);
        printf("%s\n", message);

        for (int i=0;i<nbClientsConnected;i++){
            if(strcmp((*client).pseudo,clientsLoggedIn[i].pseudo)!=0 && (clientsLoggedIn[i].connected == 1) && strcmp(clientsLoggedIn[i].channel, chan)==0){
                if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
                    perror("Erreur: le message n'a pas été transmit");
                    exit(1);
                } 
            }     
        }   


        strcpy((*client).channel, chan);
    } 
}


int comparePassword(int position, char password[]){
    int passwordOk;

    if (strcmp(channels[position].password, password)==0){
        passwordOk = 1;
    } else {
        passwordOk = 0;
    }

    return passwordOk;
}

int findChannel(char channel[]){
    int pos;
    pos = 9999;

    for (int i = 0; i < nbChannels; ++i){
        if (strcmp(channels[i].channel, channel)==0){
            pos = i;
        }
    }

    return pos;
}

int chanExists(char channel[]){

    int trouve = 0;

    for (int i = 0; i < nbChannels; ++i){
        if (strcmp(channels[i].channel,channel)==0){
            trouve = 1;
        }
    }

    return trouve;
}

int findClient(Client * client){
    int pos;
    pos = 9999;

    for (int i = 0; i < nbClientsConnected; ++i){
        if (strcmp(clientsLoggedIn[i].pseudo,client->pseudo)==0){
            pos = i;
        }
    }

    return pos;
}

int findPositionPseudo(char client[]){
    int pos;
    pos = 9999;

    for (int i = 0; i < nbClientsConnected; ++i){
        if (strcmp(clientsLoggedIn[i].pseudo,client)==0){
            pos = i;
        }
    }

    return pos;
}

void commands(Client * client){
    char *message;
    int pos;
    pos = findClient(client);

    if (pos != 9999){
        strcpy(message,"\nVoici les commandes du chat:\n");
        strcat(message,"'/who' pour savoir qui est connecté\n");
        strcat(message,"'/w destinataire message' pour chuchoter un message à un destinataire\n");
        strcat(message,"'/join channel motDePasse' pour rejoindre un channel. Si il n'exste pas il sera créé. Vous ne pouvez rejoindre qu'un seul channel à la fois.\n");
        strcat(message,"'/chan channel message' pour chuchoter un message à un channel\n");
        strcat(message,"'/leave' pour quitter \n");
        strcat(message,"'/cmd' pour un rappel des commandes\n");
        strcat(message,"-----------------------------------\n\n");
        if ((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
            perror("Erreur: le message n'a pas été transmit");
            exit(1);
        }
        
    }  
}

void leave(Client * client){
    
    byeMessage(client);
    
    int pos;
    pos = findClient(client);

    if (pos != 9999){
        clientsLoggedIn[pos].connected = 0;
        close((*client).socket);
        pthread_exit(NULL);
    }               
}


void printConnectedClients(Client * client){
    char *message;
    int pos;

    pos = findClient(client);

    if (pos != 9999){
        strcpy(message,"Les clients connectés actuellement sont:\n");
        for (int j = 0; j < nbClientsConnected; ++j){
            if (clientsLoggedIn[j].connected == 1){
                strcat(message,clientsLoggedIn[j].pseudo);
                strcat(message,"\n");
            }
        }
        if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0){
            perror("Erreur: le message n'a pas été transmit");
            exit(1);
        }
    }  
}


void sendWhisper(Client * client, char destination[], char buffer[]){
    char *message = malloc (sizeof (*message) * 256);
    // Création de la chaîne de caractère à afficher sur le chat
    strcpy(message, (*client).pseudo);
    strcat(message," vous chuchote : ");
    strcat(message,buffer);

    int pos;

    if (strcmp(client->pseudo,destination)==0){
        pos = findClient(client);
            if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
                strcpy(message, "Vous tentez de parler tout seul ? C'est étrange ...");
                if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0 ){
                    perror("Erreur: le message n'a pas été transmit");
                    exit(1);
                }
            }
    } else {
        pos = findPositionPseudo(destination);
   
        if (pos != 9999){
            if (clientsLoggedIn[pos].connected == 1){
                if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0 ){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
                }
            } else {
                pos = findClient(client);
                if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
                    strcpy(message, "Le client n'est pas connecté");
                    if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0 ){
                        perror("Erreur: le message n'a pas été transmit");
                        exit(1);
                    }
                }
            }     
        } else {
            pos = findClient(client);
            if (pos != 9999 && (clientsLoggedIn[pos].connected == 1)){
                strcpy(message, "Le pseudo n'existe pas.");
                if((write(clientsLoggedIn[pos].socket,message,strlen(message)+1)) < 0 ){
                    perror("Erreur: le message n'a pas été transmit");
                    exit(1);
                }
            }
        }
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
            strcat(message, " est là, comme la prophécie l'a décrit.");             
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
            strcat(message, " est arrivé. Finit de jouer.");  
        break;
        
        case 5 : 
            strcpy(message, (*client).pseudo);
            strcat(message, " nous a rejoint. Vite, tout le monde fait semblant d'être occupé !"); 
        break;
    }

    
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(clientsLoggedIn[i].connected == 1){
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
            strcat(message, " est parti(e). C'est bon, on peut recommencer à parler dans son dos.");  
        break;
        
        case 5 : 
            strcpy(message, (*client).pseudo);
            strcat(message, " a préféré rejoindre un autre serveur... Zut !"); 
        break;
    }

    
    printf("%s\n", message);
    for (int i=0;i<nbClientsConnected;i++){
        if(clientsLoggedIn[i].connected == 1){
            if((write(clientsLoggedIn[i].socket,message,strlen(message)+1)) < 0){
                perror("Erreur: le message n'a pas été transmit");
                exit(1);
            } 
        }     
    } 
}
	
/*------------------------------------------------------*/

/*------------------------------------------------------*/
int main(int argc, char **argv) {
  
    int 		socket_descriptor, 		/* descripteur de socket */
			nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
			longueur_adresse_courante; 	/* longueur d'adresse courante d'un client */
    sockaddr_in 	adresse_locale, 		/* structure d'adresse locale*/
			adresse_client_courant; 	/* adresse client courant */
    hostent*		ptr_hote; 			/* les infos recuperees sur la machine hote */
    servent*		ptr_service; 			/* les infos recuperees sur le service de la machine */
    char 		machine[TAILLE_MAX_NOM+1]; 	/* nom de la machine locale */
    char    mesg[256];          /* message envoyé */
    
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
    adresse_locale.sin_port = htons(5005);
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
			perror("Impossible d'ajouter un client car le serveur est plein !");
			exit(1);
		}
		else{
			if(clientsLoggedIn[nbClientsConnected].connected == 0){
				if ((nouv_socket_descriptor = accept(socket_descriptor,(sockaddr*)(&adresse_client_courant),&longueur_adresse_courante))< 0) {
					perror("erreur: echec de la création du client");
					exit(1);
				}
				// Un thread est crée pour le client
				else{
					clientsLoggedIn[nbClientsConnected].connected = 1;
                    strcpy(clientsLoggedIn[nbClientsConnected].channel,"'\0'");
					clientsLoggedIn[nbClientsConnected].pseudo[0] = '\0';
					clientsLoggedIn[nbClientsConnected].socket = nouv_socket_descriptor;
					pthread_create(&clientsLoggedIn[nbClientsConnected].thread, NULL, serverManager, &clientsLoggedIn[nbClientsConnected]);
					nbClientsConnected++;
				}
			}
		}

	}

	return 0;
    
}

