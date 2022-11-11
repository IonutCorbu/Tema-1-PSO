În alcătuirea temei, am construit structura SO_FILE care conține:
	-un file descriptor/handle pentru fisierul la care ne referim;
	-un șir de caractere pentru a memora modul de deschidere;
	-un buffer pentru operatiile de citire/scriere;
	-o pozitie in acel buffer pentru a continua operațiile de la poziția corectă;
	-o pozitie in file pentru a retine pozitia reala în fișier;
	-un flag pentru eroare care se va schimba pe 1 în cazul în care se întâmpină o eroare;
	-un flag pentru EndOfFile care se va schimba pe 1 când ajung la finalul fișierului;
	-un șir de caractere pentru a memora ultima operație realizată asupra fișierului astfel încât să putem schimba bufferul și să efectuăm so_fflush/so_fseek în functie de tranziție.
 
Pentru so_fopen primesc ca parametrii calea fisierului și modul de deschidere al fisierului. Aloc spațiu pentru un pointer de tip SO_FILE. Setez flagurile necesare pentru fiecare mod de deschidere (O_TRUNC/O_WRONLY/O_RDONLY etc pentru Linux, respectiv GENERIC_WRITE/GENERIC_READ/ pentru Windows). În cazul în care se întâmpină vreo eroare, valoarea stream->_error este setată pe 1. Dacă nu, fiecare câmp este inițializat.

Pentru so_fclose, dezaloc structurile care au necesitat alocare dinamică. Dacă nu există erori se va returna 0, altfel -1.

Pentru so_fgetc, încep prin a verifica dacă modul de deschidere al fișierului îmi permite să realizez operația. Dacă mi se permite și constat că ultima operație a fost write, voi afișa bufferul în fișier și îl voi popula cu caractere pentru a citi caractere. Dacă am ajuns la finalul fișierului, setez flagul de EOF. Dacă am din read primesc valoarea -1 în Linux sau dacă din ReadFile se returnează 0, voi seta flagul de error.

Pentru so_fputc, încep prin a verifica dacă modul de deschidere al fișierului îmi permite să realizez operația.
Dacă mi se permite și constat că ultima operație a fost read, voi goli bufferul și voi realiza un so_fseek pentru a ajunge la poziția corectă în fișier. Dacă bufferul nu este plin, adăugăm caractere în el, altfel vom apela so_fflush pentru a afișa în fișier conținutul.

Pentru so_fread, am folosit în mod repetat funcția so_fgetc și am returnat 0 în cazul în care întâmpinăm o eroare sau EOF. Altfel, se returnează numărul de elemente citite.

Pentru so_fwrite, am folosit în mod repetat funcția so_fputc. Dacă numărul de caractere scrise este diferit față de cel așteptat, flagul error este setat pe 1. Întoarce numărul de elemente scrise.

Pentru so_fseek, în cazul în care ultima operație a fost read, vom renunța la bufferul actual, iar dacă ultima operație a fost write, vom afișa bufferul. Ne folosim de apelul de sistem lseek în Linux și de wrapperul SetFilePointer din Windows pentru a modifica poziția.

Pentru so_ftell, se returnează -1 în cazul unei erori. Dacă nu, se returnează poziția în fișier.

Pentru so_fflush, se printează bufferul în fișier într-o buclă astfel încât să prevenim situațiile în care scrierea este întreruptă și nu sunt afișate toate caracterele.

Pentru so_fileno se returnează file descriptorul/handle-ul corespunzător fișierului.

Funcția so_feof returnează valoarea actuală a flagului de EOF.

Funcția so_ferror returnează valoarea actuală a flagului de error.

Funcția so_popen deschide un pipe materializat printr-un fișier între procesul părinte și cel copil. Am detaliat implementarea prin comentarii în so_stdio.c pentru Linux.

Funcția so_pclose închide fișierul folosit ca pipe.
 