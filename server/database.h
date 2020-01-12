struct question
{
    int id;
    char enunt[100];
    int raspuns_corect;
    char raspuns1[100];
    char raspuns2[100];
    char raspuns3[100];
};

sqlite3* open_db();
struct question get_question(sqlite3*,int);
int close_db(sqlite3*);