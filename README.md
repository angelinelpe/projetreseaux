# projetreseaux

Pour mettre à jour le git:
git add .
git commit -m "nom de la maj"
git push -u origin master


Procédure de lancement:

git pull 
Pour compiler le serveur: gcc -pthread server.c -o server
Pour compiler le client: gcc -pthread client.c -o client

Lancer le server: ./server
lancer le/les clients depuis un autre terminal (un client par terminal): ./client localhost
