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
//rajouté
#include <unistd.h>
#include <pthread.h>

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

// Fonction d'écoute
// Le client reste à l'écoute du serveur
//rajouté
static void * ecoute (void * socket_descriptor){
    int* socket = (int *) socket_descriptor;
    char buffer[256];
    int  taille;
    while(1){
        if((taille = read(*socket, buffer, (int)sizeof(buffer)))<=0)
            exit(1);
        buffer[taille]='\0';
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
    //rajouté
    char 	mesg[256]; 			/* message envoyé */
    //rajouté
	char	pseudo[30];		/* pseudo du client */
    pthread_t   thread_listen; 

    //on a enlevé la variable suivante car on suit pas le même principe d'envoi de msg
    //char *    mesg;           /* message envoyé */
    //mesg = argv[2];
     
     // rajouté 2 au lieu de 3
    if (argc != 2) {
	perror("usage : client <adresse-serveur>");
	exit(1);
    }
   
    prog = argv[0];
    host = argv[1];
    
    printf("nom de l'executable : %s \n", prog);
    printf("adresse du serveur  : %s \n", host);
    //printf("message envoye      : %s \n", mesg);
    
    if ((ptr_host = gethostbyname(host)) == NULL) {
	perror("erreur : impossible de trouver le serveur a partir de son adresse.");
	exit(1);
    }
    
    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
    
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
    
    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
    //rajouté 5001 au lieu de 5000
    adresse_locale.sin_port = htons(5001);
    /*-----------------------------------------------------------*/
    
    printf("numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));
    
    /* creation du socket */
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

	printf("  Le chat est ouvert ! \n");


	//Saisie du pseudo 
	printf("Veuillez saisir votre pseudo : \n");
    fgets(pseudo, sizeof pseudo, stdin);
	pseudo[strcspn(pseudo, "\n")] = '\0'; 
    if ((write(socket_descriptor, pseudo, strlen(pseudo))) < 0) {
            perror("erreur : impossible d'ecrire le message destine au serveur.");
            exit(1);
    }
  
    // printf("envoi d'un message au serveur. \n");
      
    /* envoi du message vers le serveur */
    if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
	perror("erreur : impossible d'ecrire le message destine au serveur.");
	exit(1);
    }

    printf("Vous avez choisi le pseudo : %s !\n", pseudo);
	printf("tapez 'quit' si vous voulez quitter le chat. \n\n");



	// Le client se met maintenant en écoute
    pthread_create(&thread_listen, NULL, ecoute, &socket_descriptor);

	// Tant que les messages émis sont différents de "quit"
	while(strcmp(mesg,"quit")!=0){
        
        fgets(mesg, sizeof(mesg), stdin);
        mesg[strcspn(mesg, "\n")] = '\0'; 

            //Le client envoie le message
            if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
                perror("erreur : impossible d'ecrire le message destine au serveur.");
                exit(1);
            }
        

	}
	printf("Vous quittez le chat, au revoir et à bientôt !\n");
    
    printf("\nfin de la reception.\n");
    
    close(socket_descriptor);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    
    exit(0);
    
}
