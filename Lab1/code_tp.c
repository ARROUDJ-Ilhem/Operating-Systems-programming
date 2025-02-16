#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char sourceFile[256];
    char objectFile[256];
    char executableFile[256];
    char path[256];
    
    strcpy(path,"./");
    

    
    printf("Veuillez entrer le nom du fichier source (***.c) : ");
    scanf("%s", sourceFile);
    
    printf("Veuillez entrer le nom du fichier objet (***.o) : ");
    scanf("%s", objectFile);
    
    printf("Veuillez entrer le nom du fichier exécutable : ");
    scanf("%s", executableFile);
    
    strcat(path,executableFile);
    

    int p;
    int codeRetour;

    //****************************P1******************************************//
    p = fork();
    if (p == -1) {
        // Fils non créé
        perror("Une erreur est survenue dans la création de P1");
        exit(1);
    } else if (p == 0) { // Processus fils P1
        execlp("gcc", "gcc", sourceFile, "-c", "-o", objectFile, (char *)NULL);
        perror("Erreur dans execlp de P1");
        exit(1);
    } else { // Processus parent P1
        // Attendre la terminaison de P1
        wait(&codeRetour);
        if (WIFEXITED(codeRetour)) {
            printf("Terminaison normale de P1\n");

            //****************************P2******************************************//
            p = fork();
            if (p == -1) {
                // Fils non créé
                perror("Une erreur est survenue dans la création de P2");
                exit(1);
            } else if (p == 0) { // Processus fils P2
                execlp("gcc", "gcc", objectFile, "-o", executableFile, (char *)NULL);
                perror("Erreur dans execlp de P2");
                exit(1);
            } else { // Processus parent P2
                // Attendre la terminaison de P2
                wait(&codeRetour);
                if (WIFEXITED(codeRetour)) {
                    printf("Terminaison normale de P2\n");

                   //****************************P3******************************************//
                    p = fork();
                    if (p == -1) {
                        // Fils non créé
                        perror("Une erreur est survenue dans la création de P3");
                        exit(1);
                    } else if (p == 0) { // Processus fils P3
                        if (execlp(path, executableFile, (char *)NULL) == -1) {
                        //Si ya une erreur
                          perror("Erreur dans execlp de P3");
                          exit(1); 
                         }
                      
                     exit(0);//Termine normalement sans erreurs
                    } else { // Processus parent P3
                        // Attendre la terminaison de P3
                        wait(&codeRetour);
                        if (WIFEXITED(codeRetour)) {
                            int statut = WEXITSTATUS(codeRetour);
                            if (statut == 0) {
            printf("Terminaison normale de P3 sans erreur (code de sortie est : %d)\n", statut);
                           } else {
            printf("Terminaison normale de P3 avec erreur (code de sortie est : %d)\n", statut);
                            }
                            
                        } else {
                            printf("Terminaison anormale de P3\n");
                        }
                    }
                } else {
                    printf("Terminaison anormale de P2\n");
                }
            }
        } else {
            printf("Terminaison anormale de P1\n");
        }
    }

    return 0;
}