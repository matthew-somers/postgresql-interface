#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <postgresql/libpq-fe.h>
#define BUF 120

/*
   Author: Matthew Somers
*/

void connected(PGconn *conn);
void insert(PGconn *conn, char tabletester[]);
void mainmenu(PGconn *conn, char tablename[]);
void viewdata(PGconn *conn, char tablename[]);
void deletedata(PGconn *conn, char tablename[]);

void main(int argc, char *argv[])
{
   if (argc != 5)
   {
      printf("Usage: %s dbname host user password\n\n", argv[0]);
      return;
   }

   char input[BUF];
   char connection[BUF];
   strcpy(connection,"dbname=");
   strcat(connection,argv[1]);
   strcat(connection," host=");
   strcat(connection,argv[2]);
   strcat(connection," user=");
   strcat(connection,argv[3]);
   strcat(connection," password=");
   strcat(connection,argv[4]);
   PGconn *conn = PQconnectdb(connection);

   if (PQstatus(conn) == CONNECTION_BAD)
   {
      printf("Unable to connect.\n");
      printf("Usage: %s dbname host user password\n\n", argv[0]);
      return;
   }
   else
      connected(conn);
}

void connected(PGconn *conn)
{
   printf("\nConnected.\n\n"); 
   PGresult *res;

   // Start a transaction block 
   res = PQexec(conn, "BEGIN");
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
      PQclear(res);
   }

   char input[BUF];
   char tabletester[BUF];
   while(1)
   {
      printf("Table name: ");
      fgets(input, sizeof(input), stdin);
      input[strlen(input)-1] = '\0';

      //test if table exists
      strcpy(tabletester,"SELECT * from pg_class where relname='");
      strcat(tabletester,input);
      strcat(tabletester,"';");
      res = PQexec(conn, tabletester);

      if (PQntuples(res) < 1)
         printf("\nTable doesn't exist.\n\n");
      else
         mainmenu(conn, input);
   }
}

void mainmenu(PGconn *conn, char tablename[])
{
   char input[BUF];
   while(1)
   {
      printf("\nMain Menu:\n");
      printf("1. Insert new data.\n");
      printf("2. View table data.\n");
      printf("3. Delete table data.\n");
      printf("4. Return to table select.\n");
      printf("5. Quit.\n");
      printf("Enter number of your selection: ");
      fgets(input, sizeof(input), stdin);
      input[strlen(input)-1] = '\0';

      if (input[0] == '1')
         insert(conn, tablename);
      else if (input[0] == '2')
         viewdata(conn, tablename);
      else if (input[0] == '3')
         deletedata(conn, tablename);
      else if (input[0] == '4')
         return;
      else if (input[0] == '5')
         exit(1);
      else
         printf("Invalid input.\n");
   }
}

void deletedata(PGconn *conn, char tablename[])
{
   PGresult *res;
   char tabledata[BUF];
   char input[BUF];

   //get data to ask for values
   strcpy(tabledata,"SELECT * from ");
   strcat(tabledata,tablename);
   strcat(tabledata," LIMIT 1");
   res = PQexec(conn, tabledata);

   strcpy(tabledata,"DELETE FROM ");
   strcat(tabledata,tablename);
   strcat(tabledata," WHERE ");

   strcat(tabledata,PQfname(res,0));
   strcat(tabledata,"=");

   printf("%s: ", PQfname(res, 0));
   fgets(input, sizeof(input), stdin);
   input[strlen(input)-1] = '\0';

   strcat(tabledata,input);

   res = PQexec(conn,tabledata);

   if (PQresultStatus(res) == PGRES_COMMAND_OK)
      printf("\n%s\nDeletion successful! (or it never existed in the first place)\n\n", tabledata);

   else
      printf("\nDeletion failed.\n%s\n", PQerrorMessage(conn));

   res = PQexec(conn,"END");
   PQclear(res);

   // Start a transaction block 
   res = PQexec(conn, "BEGIN");
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
      PQclear(res);
   }
   
}

void viewdata(PGconn *conn, char tablename[])
{
   PGresult *res;
   char tabledata[BUF];

   //get data to ask for values
   strcpy(tabledata,"SELECT * from ");
   strcat(tabledata,tablename);
   res = PQexec(conn, tabledata);

   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      printf("\nViewing data failed.\n%s\n", PQerrorMessage(conn));
      return;
   }

   int i;
   int j;
   int nfields = PQnfields(res);
   printf("\n\n");
   for (i = 0; i < nfields; i++)
   {
      printf("%-15s", PQfname(res, i));
   }

   printf("\n\n");

   for (i = 0; i < PQntuples(res); i++)
   {
      for (j = 0; j < nfields; j++)
         printf("%-15s", PQgetvalue(res, i, j));
      printf("\n");
   }

}
void insert(PGconn *conn, char tablename[])
{
   PGresult *res;
   char input[BUF];
   char command[BUF];
   char tabletester[BUF];

   strcpy(command,"INSERT INTO ");

   strcat(command,tablename);
   strcat(command," VALUES (");

   //get data to ask for values
   strcpy(tabletester,"SELECT * from ");
   strcat(tabletester,tablename);
   strcat(tabletester," LIMIT 1");
   res = PQexec(conn, tabletester);
   int m;
   printf("\nEnter data now. Remember that text values need ' ' around them.\n");
   for (m = 0; m < PQnfields(res); m++)
   {
      printf("%s: ", PQfname(res, m));
      fgets(input, sizeof(input), stdin);
      input[strlen(input)-1] = '\0';
      strcat(command,input);
      if (m != PQnfields(res)-1)
         strcat(command,",");
   }

   strcat(command,")");
   command[strlen(command)] = '\0';

   //res = PQexec(conn, "CREATE TABLE People(id integer, name text)");
   res = PQexec(conn, command);


   if (PQresultStatus(res) == PGRES_COMMAND_OK)
      printf("\n%s\nInsertion successful!\n\n", command);

   else
      printf("\nInsert failed. Did you use ' ' around text?\n%s\n", PQerrorMessage(conn));

   res = PQexec(conn,"END");
   PQclear(res);

   // Start a transaction block 
   res = PQexec(conn, "BEGIN");
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
      PQclear(res);
   }
}
