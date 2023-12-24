#include <stdio.h>
#include <string.h>
#include "client.h"
#include "server.h"

#define CHUNK_SIZE 1024
#define HEADER_SIZE 256


void sendFileChunk(char *data, size_t dataSize, int numPort, const char* userId) 
{
    char header[HEADER_SIZE];
    //on met l' user ID dans le header
    snprintf(header, sizeof(header), "Header: UserID:%s_FileName: file_chunk", userId);

   
    size_t totalSent = 0;
    size_t remainingData = dataSize;

    while (remainingData > 0) {
        // Déterminer la taille du chunk pour cet envoi
        size_t chunkSize = (remainingData > CHUNK_SIZE) ? CHUNK_SIZE : remainingData;

        char combinedData[CHUNK_SIZE + HEADER_SIZE];
        snprintf(combinedData, sizeof(combinedData), "%s%.*s", header, (int)chunkSize, data + totalSent);

        // Envoyer le chunk de données
        sndmsg(combinedData, numPort);

        // Mettre à jour les compteurs pour le prochain chunk
        remainingData -= chunkSize;
        totalSent += chunkSize;
    }
}



void uploadFile(char *filename, int numPort, const char* userID) {
    printf("Uploading file '%s' to the server on port %d...\n", filename,numPort);
  
    char command[1024];
    snprintf(command, sizeof(command), "-up %s", filename);
    sndmsg(command, numPort); 

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    

    char header[256];
    snprintf(header, sizeof(header), "Header: SenderID_FileName: %s", filename);
    sndmsg(header, numPort);

    

    char buffer[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE - strlen(header), file)) > 0) {
        sendFileChunk(buffer, bytesRead, numPort, userID); 
    }


    fclose(file);

    char eofMessage[4]; 
    strcpy(eofMessage, "EOF");
    sndmsg(eofMessage, numPort);


    printf("Uploading file with success \n");

}

void listFiles(int numPort) {
    char command[1024];
    snprintf(command, sizeof(command), "-list");
    sndmsg(command, numPort); 

    char msg[1024];
    printf("Récupération de la liste des fichiers sur le serveur...\n");
    startserver(numPort + 1);
    sndmsg("-list", numPort); 
    getmsg(msg);
    printf("Liste des fichiers sur le serveur : %s\n", msg);
    stopserver();
}




void downloadFile(char *filename, int numPort, const char* userID) {
    printf("Downloading file '%s' from the server...\n", filename);
    
    // Inclure l'UserID dans la demande de téléchargement
    char command[1024];
    snprintf(command, sizeof(command), "-down %s UserID:%s", filename, userID);
    sndmsg(command, numPort); // Envoie la commande au serveur
    
    // Ouvre un serveur côté client pour recevoir le fichier
    startserver(numPort + 1);
    char getFileCommand[256];
    snprintf(getFileCommand, sizeof(getFileCommand), "get:%s UserID:%s", filename,userID);
    sndmsg(getFileCommand, numPort);
    
    // Prépare à recevoir le fichier
    char receivedData[CHUNK_SIZE + HEADER_SIZE];
    FILE *file = fopen(filename, "wb"); // Ouvre le fichier local pour l'écriture
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier local");
        stopserver();
        return;
    }
    
    // Boucle pour recevoir les données du fichier
    while (1) {
        getmsg(receivedData); // Attends et reçoit les données du serveur
        if (strstr(receivedData, "EOF") != NULL) { // Vérifie le marqueur de fin de fichier
            break; // Sortie de la boucle si la fin du fichier est atteinte
        }
        size_t headerLength = strcspn(receivedData, "\n"); // Calcule la longueur du header
        fwrite(receivedData + headerLength, 1, strlen(receivedData) - headerLength, file); // Écrit les données dans le fichier
    }
    
    // Fermeture du fichier et du serveur
    fclose(file);
    stopserver();
    printf("Fichier '%s' téléchargé avec succès\n", filename);
    
    
    }








int main(int argc, char *argv[]) {

    int numPort = 5000;

    if (argc < 2) {
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }
    
    const char* userId = "UserID124"; // a changer plus tard pour le mettre dynamique

    if (strcmp(argv[1], "-up") == 0 && argc == 3) {
        uploadFile(argv[2], numPort,userId);
    } else if (strcmp(argv[1], "-list") == 0) {
        listFiles(numPort);
    } else if (strcmp(argv[1], "-down") == 0 && argc == 3) {
        downloadFile(argv[2],numPort,userId);
    } else {
        printf("Invalid command or arguments\n");
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    return 0;
}