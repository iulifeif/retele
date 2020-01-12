#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

#include "database.h"

sqlite3 *open_db()
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("test.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }
    return db;
}

int close_db(sqlite3 *db)
{
    sqlite3_close(db);
    return 0;
}

struct question get_question(sqlite3 *db, int question_id)
{
    sqlite3_stmt *stmt;
    const char *tail;
    char *zErrMsg = 0;
    char query[100];
    sprintf(query, "Select * from intrebari where id = %d;", question_id);
    int result = sqlite3_prepare_v2(db, query, 100, &stmt, &tail);
    struct question qst;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            qst.id=sqlite3_column_int(stmt, 0);
            strcpy(qst.enunt,sqlite3_column_text(stmt, 1));
            qst.raspuns_corect=sqlite3_column_int(stmt, 2);
            strcpy(qst.raspuns1,sqlite3_column_text(stmt, 3));
            strcpy(qst.raspuns2,sqlite3_column_text(stmt, 4));
            strcpy(qst.raspuns3,sqlite3_column_text(stmt, 5));
        }
    sqlite3_finalize(stmt);
    return qst;
}