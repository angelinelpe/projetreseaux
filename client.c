/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

static void * clientManager(void * socket_descriptor){
    int  length;
    int* socket = (int *) socket_descriptor;
    char buffer[256];
    
    for(;;){
        if((length = read(*socket, buffer, (int)sizeof(buffer)))<=0){
        	exit(1);
        }
        buffer[length]='\0';
        printf("%s \n", buffer);
    }
}

int main(int argc, char **argv) {
  
    int 	socket_descriptor, 	/* descripteur de socket */
		longueur; 		/* longueur d'un buffer utilisé */
    sockaddr_in adresse_locale; 	/* adresse de socket local */
    hostent *	ptr_host; 		/* info sur une machine hote */
    servent *	ptr_service; 		/* info sur service */
    char 	buffer[256];
    char *	prog; 			/* nom du programme */
    char *	host; 			/* nom de la machine distante */
    char 	mesg[256]; 			/* message envoyé */
	char	pseudo[50];		/* pseudo du client */
    pthread_t   thread_listen; 

     
    if (argc != 2) {
	perror("usage : client <adresse-serveur>");
	exit(1);
    }
   
    prog = argv[0];
    host = argv[1];
    
    printf("nom de l'executable : %s \n", prog);
    printf("adresse du serveur  : %s \n", host);
    
    if ((ptr_host = gethostbyname(host)) == NULL) {
	perror("erreur : impossible de trouver le serveur a partir de son adresse.");
	exit(1);
    }
    
    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
    
    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
    adresse_locale.sin_port = htons(5005);
    /*-----------------------------------------------------------*/
    
    printf("numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("erreur : impossible de creer la socket de connexion avec le serveur.");
	exit(1);
    }
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
	perror("erreur : impossible de se connecter au serveur.");
	exit(1);
    }
    
    printf("connexion etablie avec le serveur. \n\n");

    printf("-----------------------------------\n");
    printf("Voici les commandes du chat:\n");
    printf("'/who' pour savoir qui est connecté\n");
    printf("'/w destinataire message' pour chuchoter un message à un destinataire\n");
    printf("'/join channel motDePasse' pour rejoindre un channel. Si il n'exste pas il sera créé. Vous ne pouvez rejoindre qu'un channel à la fois.\n");
    printf("'/chan channel message' pour chuchoter un message à un channel\n");
    printf("'/leave' pour quitter \n");
    printf("'/cmd' pour un rappel des commandes\n");
    printf("-----------------------------------\n\n");

	printf("Merci de choisir votre pseudo\n");
    fgets(pseudo, sizeof pseudo, stdin);
	pseudo[strcspn(pseudo, "\n")] = '\0'; 
    if ((write(socket_descriptor, pseudo, strlen(pseudo))) < 0) {
            perror("erreur : impossible d'ecrire le message destine au serveur.");
            exit(1);
    }
  
    if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
	perror("erreur : impossible d'ecrire le message destine au serveur.");
	exit(1);
    }

    pthread_create(&thread_listen, NULL, clientManager, &socket_descriptor);

	while(strcmp(mesg,"/leave")!=0){
        
        fgets(mesg, sizeof(mesg), stdin);
        mesg[strcspn(mesg, "\n")] = '\0'; 

            if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
                perror("erreur : impossible d'ecrire le message destine au serveur.");
                exit(1);
            }
	}
	printf("\nVous partez déjà ? A très vite !\n");
	printf("-----------------------------------\n");   
    printf("\nfin de la reception.\n");
    
    close(socket_descriptor);
    pthread_exit(NULL);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    
    exit(0);
    
}
