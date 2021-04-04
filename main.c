#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG
#define MAX_LINE_LEN 1025       // Massima lunghezza di una riga
#define MAX_INPUT_LEN 16        // Massima lunghezza di un input


// STRUTTRE DATI
typedef struct line {
    char *text;
    struct line *prev;
    struct line *next;
} line_t;
typedef struct event {;
    struct event *next;
    struct event *prev;
    int numInSave;          // Numero di righe salvate
    line_t *head;           // Prima riga nel salvataggio (NULL se non ci sono righe salvate)
    line_t *last;           // Ultima riga nel salvataggio (NULL se non ci sono righe salvate)
    int numInText;          // Numero di righe presenti nel testo a cui fa riferimento l'evento
    line_t *startInText;    // Se l'evento è una cancellazione fa riferimento alla riga precedente, altrimenti alla prima riga
    line_t *endInText;      // Se l'evento è una cancellazione fa riferimento alla riga successiva, altrimenti all'ultima riga
} event_t;



// VARIABILI GLOBALI
// (headLine e headEvent si contano da 0 e in quella posizione non contengono nessuna riga/evento)
int lines = 0;
line_t *headLine = NULL;
line_t *lastLine = NULL;
int events = 0;
event_t *headEvent = NULL;
event_t *currentEvent = NULL;
int currentEventNum = 0;
int desiredEventNum = 0;



// --- FUNZIONI AUSILARIE ---

int max(int a, int b){
    if (a >= b)
        return a;
    else
        return b;
};

int min(int a, int b){
    if (a <= b)
        return a;
    else
        return b;
};

// Funzione di ricerca di righe nel testo
line_t *findLine(int target_idx){
    if (target_idx > lines)
        return NULL;
    else{
        // Valuta se è piu veloce raggiungere la riga desiderata partendo dall'inizio o dalla fine del testo
        if (target_idx <= lines/2){
            line_t *curr = headLine;
            for(int idx = 0; idx < target_idx; idx++)
              curr = curr->next;
            return curr;
        }else{
            line_t *curr = lastLine;
            for(int idx = lines; idx > target_idx; idx--)
                curr = curr->prev;
            return curr;
        }
    }
};

// Funzione di creazione evento
event_t *addEvent(){
    event_t *new = malloc(sizeof(event_t));

    currentEvent->next = new;
    new->prev = currentEvent;
    new->next = NULL;
    new->head = NULL;
    new->last = NULL;
    currentEvent = new;

    events++;
    currentEventNum = events;
    desiredEventNum = events;

    return new;
}

// Funzione di rimozione evento
void freeEvent(event_t *to_free){
    
    if(to_free->head != NULL) {
        while (to_free->head != to_free->last) {     // Dealloca tutte le righe di testo associate all'evento
            to_free->last = to_free->last->prev;
            free(to_free->last->next->text);
            free(to_free->last->next);
        }
        free(to_free->head->text);
        free(to_free->head);
    }

    free(to_free);      // Dealloca l'evento

    events--;
}

// Funzione di rimozione di tutti gli eventi successivi ad un evento dato
void freeEventFrom(event_t *first_to_free){
    if (first_to_free != NULL) {
        event_t *curr;
        while (first_to_free->next !=NULL){
            curr = first_to_free->next;
            first_to_free->next = first_to_free->next->next;
            freeEvent(curr);
        }
        first_to_free->prev->next = NULL;
        freeEvent(first_to_free);
    }
}

// Funzione di scambio tra storia eventi e testo, utilizzata in undo e redo
void swapText(){
    // Esegui solo se l'evento non rappresenta una cancellazione 0 elementi
    if ((currentEvent->numInText > 0) || (currentEvent->numInSave > 0)) {
        line_t *prev;
        line_t *post;

        line_t *swapHead;
        line_t *swapLast;

        // Crea puntatori temporanei sul testo per scambio righe tra salvataggio e testo
        if (currentEvent->numInText == 0) {     // Se avevo cancellato non devo salvare niente
            prev = currentEvent->startInText;
            swapHead = NULL;
            swapLast = NULL;
            post = currentEvent->endInText;
        } else {
            prev = currentEvent->startInText->prev;
            swapHead = currentEvent->startInText;
            swapLast = currentEvent->endInText;
            post = currentEvent->endInText->next;
        }

        if (currentEvent->numInSave == 0) {
            // CANCELLA
            // Collega l'elemento precedente alle righe cancellate con quello successivo
            prev->next = post;
            if (post != NULL)
                post->prev = prev;

            // Aggiorna puntatori startInText e endInText
            currentEvent->startInText = prev;
            currentEvent->endInText = post;

            // Aggiorna lastLine se l'ultma riga è cambiata
            if (post == NULL)
                lastLine = prev;

        } else {
            // CAMBIA
            // Collega testo con inizio salvataggio
            prev->next = currentEvent->head;
            prev->next->prev = prev;

            // Collega fine salvataggio con testo
            currentEvent->last->next = post;
            if (post != NULL)
                post->prev = currentEvent->last;

            // Aggiorna puntatori startInText e endInText
            currentEvent->startInText = prev->next;
            if (post != NULL)
                currentEvent->endInText = post->prev;
            else {
                currentEvent->endInText = currentEvent->last;

                // Aggiorna lastLine se l'ultma riga è cambiata
                lastLine = currentEvent->endInText;
            }
        }

        // Aggiorna puntatori salvataggio
        currentEvent->head = swapHead;
        currentEvent->last = swapLast;

        // Aggiorna contatore linee
        lines = lines + currentEvent->numInSave - currentEvent->numInText;

        // Scambia numInText e numInSave
        int swapNum = currentEvent->numInText;
        currentEvent->numInText = currentEvent->numInSave;
        currentEvent->numInSave = swapNum;
    }
}

// Funzione di attuazione degli undo/redo richiesti, chiamata prima di ogni comando che chiede di stampare qualcosa in output
void applyUndoRedo(){
    // Calcola numero di passi da fare (negativo => undo, positivo => redo)
    int num = desiredEventNum - currentEventNum;

    if (num < 0){
        // UNDO
        for (int i = 0; (i > num)&&(currentEvent != headEvent); i--) {
            swapText();

            currentEvent = currentEvent->prev;
            currentEventNum--;
        }

    } else if (num > 0){
        // REDO
        for (int i = 0; (i < num)&&(currentEvent->next != NULL); i++) {
            currentEvent = currentEvent->next;
            currentEventNum++;

            swapText();
        }
    }

    // Dopo l'esecuzione gli undo/redo richiesti sono stati applicati
    desiredEventNum = currentEventNum;
}

// --- --- --- --- --- --- ---



// --- FUNZIONI DI DEBUG ---

//Funzione di stampa utilizzando puntatori a prev, utlizzata in debug
void printReverseFromTo(line_t *first, line_t *last) {
    if ((first == last)&&(first != NULL))
        printf("%s", first->text);
    else if(last != NULL){
        printReverseFromTo(first, last->prev);
        printf("%s", last->text);
    }

}

// Funzione di stampa di debug
void printAll(){
    printf("-------------------- DEBUG --------------------\n");

    printf("TESTO (%d righe):\n", lines);
    for(line_t *curr = headLine->next; (curr != NULL)&&(curr != lastLine->next); curr = curr->next)
        printf("%s", curr->text);

    printf("TESTO (printAllReverse):\n");
    printReverseFromTo(headLine->next, lastLine);

    printf("STORIA (%d eventi, corrente=%d, desiderato=%d):\n", events, currentEventNum, desiredEventNum);
    for(event_t *curr = headEvent->next; curr != NULL; curr = curr->next){
        printf(" numInText=%d numInSave=%d", curr->numInText, curr->numInSave);
        if(curr == currentEvent)
            printf("  <-- evento corrente");
        printf("\n");

        if (curr->startInText != NULL)
            printf("     StartInText: %s", curr->startInText->text);
        else
            printf("     StartInText: NULL\n");
        if (curr->endInText != NULL)
            printf("     EndInText: %s", curr->endInText->text);
        else
            printf("     EndInText: NULL\n");
        if (curr->startInText->text == NULL)
            printf("\n");

        for(line_t *currLine = curr->head; (currLine != NULL)&&(currLine != curr->last->next); currLine = currLine->next) {
            printf("          %s", currLine->text);
        }
    }

    printf("-----------------------------------------------\n");
}

// --- --- --- --- --- --- ---



// --- FUNZIONI COMANDI ---

// Funzione di modifica testo e creazione eventi, utilizzata da change e delete
void edit(int ind1, int ind2, int num) {
    // Salva numero di linee prima della modifica (utile per aggiornare lastLine)
    int prevLines = lines;

    // Cancella tutti gli eventi successivi
    freeEventFrom(currentEvent->next);

    // Crea evento
    event_t *newEvent = addEvent();

    // SALVATAGGIO
    newEvent->numInText = num;

    line_t *prev = findLine(ind1 - 1);          // Ricerca prima riga da salvare

    // Verifica se salvare effettivamente qualcosa
    if (ind1 > lines){
        newEvent->numInSave = 0;
    }else{
        newEvent->head = prev->next;

        int lastToSave = min(ind2, lines);
        int numToSave = lastToSave - ind1 + 1;        // Numero di righe da salvare
        newEvent->numInSave = numToSave;

        // Ricerca ultima riga da salvare
        if (lastToSave - ind1 <= lines - lastToSave) {      // Se l'ultima riga da salvare è più vicina alla prima riga da salvare
            newEvent->last = prev;
            for (int i = 0; i < numToSave; i++)
                newEvent->last = newEvent->last->next;
        }else {                                             // Se l'ultima riga da salvare è più vicina a lastLine
            newEvent->last = lastLine;
            for (int i = 0; i < lines - lastToSave; i++)
                newEvent->last = newEvent->last->prev;
        }

        lines = lines - numToSave;
    }

    // APPLICAZIONE MODIFICHE
    // Cambia
    if(num > 0){
        line_t *curr = prev;

        // Alloca nuovo nodo del testo per ogni riga da aggiungere
        for(int i = 0; i < num; i++){
            curr->next = (line_t *)malloc(sizeof(line_t));
            curr->next->prev = curr;
            curr = curr->next;

            // Acquisizione testo
            char inputText[MAX_LINE_LEN];
            if(fgets(inputText, MAX_LINE_LEN, stdin) == NULL){      // Termina se l'acquisizione fallisce
                printf("Impossibile leggere riga\n");
                exit(1);
            }
            // Creazione array dinamico per testo di ogni riga
            int len = (int)strlen(inputText) + 1;       // Lunghezza = stringa + terminatore
            curr->text = malloc(len * sizeof(char));     // Alloca array dinamico
            for (int j = 0; j < len; j++)               // Copia input in array
                curr->text[j] = inputText[j];

            lines++;
        }
        fgetc(stdin);   // Skip terminating '.'
        fgetc(stdin);   // Skip '/n'

        // Crea puntatori startInText e endInText
        newEvent->startInText = prev->next;
        newEvent->endInText = curr;

        // Se aggiunto righe senza modificarne, oppure se hai modificato l'ultima
        if ((newEvent->numInSave == 0)||(newEvent->last->next == NULL)){
            curr->next = NULL;
            lastLine = curr;    // Aggiorna lastline
        }else{
            // Collega ultima nuova linea al testo
            curr->next = newEvent->last->next;
            newEvent->last->next->prev = curr;
        }
    }

    // Cancella
    else{
        if (newEvent->numInSave > 0){
            // Ricollega il testo salatando gli elementi cancellati
            newEvent->startInText = prev;
            newEvent->endInText = newEvent->last->next;
            prev->next = newEvent->last->next;
            if(prev->next != NULL)
                prev->next->prev = prev;
        }

        // Aggiorna lastLine sel'ultma riga è cambiata
        if (ind2 >= prevLines) {
            if (prev->next == NULL)
                lastLine = prev;
            else
                lastLine = prev->next;
        }
    }
}

// Funzione di stampa
void print(int ind1, int ind2){
    line_t *curr = findLine(ind1);

    int ind = 0;
    // Stampa '.' per tutte le righe richieste non presenti prima della riga 1
    for(ind = ind1; ind < 1; ind++)
        fputs(".\n", stdout);

    // Stampa tutte le righe richieste presenti
    int endPrint = min(lines, ind2);
    for(ind = max(ind1,1); ind <= endPrint; ind++) {
        fputs(curr->text, stdout);
        curr = curr->next;
    }

    // Stampa '.' per tutte le righe richieste non presenti
    for(ind = max(lines + 1, ind1); ind <= ind2; ind++)
        fputs(".\n", stdout);
}

// Funzione di undo
void undo(int num){
    // Sposta l'indice di evento desiderato all'evento precedente, se non è gia all'evento 0 (headEvent)
    desiredEventNum = max(desiredEventNum - num, 0);
}

// Funzione di redo
void redo(int num){
    // Sposta l'indice di evento desiderato all'evento successivo, se non è gia all'ultimo evento (lastEvent)
    desiredEventNum = min(desiredEventNum + num, events);
}

// Funzione di deallocazione generale, utilizzata in quit
void destroyAll(){
    // Dealloca storia eventi
    freeEventFrom(headEvent->next);
    freeEvent(headEvent);


    // Dealloca testo
    line_t *curr;
    while(headLine->next != NULL) {
        curr = headLine;
        headLine = headLine->next;
        free(curr->text);
        free(curr);
    }
    free(headLine->text);
    free(headLine);
}

// --- --- --- --- --- ---



// MAIN
int main() {
    // VARIABILI INPUT
    char inputStr[256];
    char *inputStrParser;

    struct {
        char cmd;
        int in1;
        int in2;
    } input;

    // CREAZIONE STRUTTURA DATI TESTO
    headLine = malloc(sizeof(line_t));
    headLine->text = NULL;
    headLine->prev = NULL;
    headLine->next = NULL;
    lastLine = headLine;

    // CREAZIONE STRUTTURA STORIA EVENTI
    headEvent = malloc(sizeof(event_t));
    headEvent->prev = NULL;
    headEvent->next = NULL;
    headEvent->head = NULL;
    headEvent->last = NULL;
    headEvent->startInText = NULL;
    headEvent->endInText = NULL;
    currentEvent = headEvent;

    // MAIN LOOP - CICLO ACQUISIZIONE COMANDI
    short int exit = 0;
    while (!exit) {

        if(fgets(inputStr, MAX_INPUT_LEN, stdin) == NULL){
            printf("Impossibile leggere comando\n");
            return 1;
        }

        input.cmd = inputStr[strlen(inputStr)-2];

        if (input.cmd == 'q') {
            // Esci e dealloca tutto
            destroyAll();
            exit = 1;
        }else{
            // Salva ind1 e ind2 e esegui comdando
            input.in1 = (int)strtoul(inputStr, &inputStrParser, 10);
            input.in2 = (int)strtoul(inputStrParser+sizeof(char), NULL, 10);

            // Leggi comando ed esegui
            switch(input.cmd){
                case 'c':
                    applyUndoRedo();
                    edit(input.in1, input.in2, input.in2 - input.in1 + 1);
                    break;
                case 'd':
                    applyUndoRedo();
                    edit(input.in1, min(input.in2, lines), 0);
                    break;
                case 'p':
                    applyUndoRedo();
                    print(input.in1, input.in2);
                    break;
                case 'u':
                    undo(input.in1);
                    break;
                case 'r':
                    redo(input.in1);
                    break;
                default:
                    printf("Comando sconosciuto: %c\n", input.cmd);
                    destroyAll();
                    return 1;
            }

            #ifdef DEBUG
                applyUndoRedo();
                printAll();     // Stampa tutto il testo e altre info di debug ad ogni comando inserito
            #endif

        }

    }

    return 0;

}
