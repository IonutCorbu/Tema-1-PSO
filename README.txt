În alcătuirea temei, am construit structura SO_FILE care conține:
	-un file descriptor/handle pentru fisierul la care ne referim;
	-un șir de caractere pentru a memora modul de deschidere;
	-un buffer pentru operatiile de citire/scriere;
	-o pozitie in acel buffer pentru a continua operațiile de la poziția corectă;
	-o pozitie in file pentru a retine pozitia reala în fișier;
	-un flag pentru eroare care se va schimba pe 1 în cazul în care se întâmpină o eroare;
	-un flag pentru EndOfFile care se va schimba pe 1 când ajung la finalul fișierului;
	-un șir de caractere pentru a memora ultima operație realizată asupra fișierului astfel încât să putem schimba bufferul și să efectuăm so_fflush/so_fseek în functie de tranziție.
 
Pentru fopen primesc ca parametrii calea fisierului și modul de deschidere al fisierului. Aloc spațiu pentru un pointer de tip SO_FILE.