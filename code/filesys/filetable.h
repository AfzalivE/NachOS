#ifndef FILETABLE_H_
#define FILETABLE_H_

#include "openfile.h"
#include "syscall.h"
#include "synch.h"

struct entry {
        int id;
        char* name;
        Lock* mutex;
        OpenFile* f;
        int OpCount;
};

class FileTable
{
        public:
                FileTable();
                ~FileTable();
                int find(char* name2);
                OpenFile* find(int id2);
                entry* findEntry(int id2);
                char* getName(int id2);
                void remove(int id2);
                int append(entry * e);
                int append(char* name2, OpenFile* f2);
                void acquireLock(int id);
                void releaseLock(int id);
                void upCount(int id);
                bool newIn();
                bool newOut();

        private:
                entry * files[MAX_FILES];
                Lock * l;
                bool redirIn, redirOut;
                int count;//used to generate ids.
};


#endif /* FILETABLE_H_ */
