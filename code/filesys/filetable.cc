#include "FileTable.h"

FileTable::FileTable()
{
        *ftable = new FileTable();
        for (int i = 0; i < MAX_FILES; i++) files[i] = NULL;
        count = 0;
        files[count] = new entry;
        files[count]->id = ConsoleInput;
        files[count]->name = (char *) "I";
        files[count]->mutex= new Lock("Console In Lock");
        files[count]->f=NULL;
        files[count]->OpCount=0;
        count++;
        files[count] = new entry;
        files[count]->id = ConsoleOutput;
        files[count]->name = (char *) "O";
        files[count]->mutex = new Lock("Console Out Lock");
        files[count]->f=NULL;
        files[count]->OpCount=0;
        count++;
        l = new Lock("Master Table Lock");
}

FileTable::~FileTable()
{
        entry* temp;

        for (int i = 0; i < count; i++)
        {
                temp = files[i];
                delete files[i]->mutex;
                delete temp;
        }
        delete l;
}

int FileTable::find(char* name2)
{
        l->Acquire();
        int temp = -1;
        for (int i = 0; i < count; i++) if(strcmp(files[i]->name,name2) == 0) temp = files[i]->id;
        l->Release();
        return temp;
}

OpenFile* FileTable::find(int id2)
{
        l->Acquire();
        OpenFile* temp = NULL;
        if (files[id2]->f != NULL) temp = files[id2]->f;
        l->Release();
        return temp;
}

entry * FileTable::findEntry(int id2)
{
        l->Acquire();
        entry* temp = files[id2];
        l->Release();
        return temp;
}

char* FileTable::getName(int id2)
{
        l->Acquire();
        char* temp = files[id2]->name;
        l->Release();
        return temp;
}

void FileTable::remove(int id2)
{
        l->Acquire();
        entry* temp;
        if(id2 == 0 || id2 == 1) return;
        files[id2]->OpCount--;
        if(files[id2]->OpCount > 0)
        {
                l->Release();
                return;
        }
        temp = files[id2];
        count--;
        for (int i = id2; i < count; i++) files[i] = files[i+1];
        files[count] = NULL;
        delete temp;
        l->Release();
}

int FileTable::append(entry * e)
{
        l->Acquire();
        entry * temp = e;
        temp->id = count;
        files[count] = temp;
        files[count]->OpCount=1;
        count++;
        l->Release();
        return temp->id;
}

int FileTable::append(char* name2, OpenFile* f2)
{
        l->Acquire();
        entry* temp= new entry;

        temp->name = name2;
        temp->f = f2;
        temp->id = count;
        temp->mutex = new Lock(name2);

        files[count] = temp;
        files[count]->OpCount=1;
        count++;
        l->Release();
        return temp->id;
}

void FileTable::upCount(int id)
{
        files[id]->OpCount++;
}

void FileTable::acquireLock(int id) {files[id]->mutex->Acquire();}
void FileTable::releaseLock(int id) {files[id]->mutex->Release();}
