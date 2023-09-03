## README

This example uses a sqlite3 DB.

### DB schema

One speciality with respect to gambas is that you have to be careful how to 
define the schema. gambas will read this schema definition later and extract
some information from it.

Expecially the autoincrement feature often used for primary key columns
is affected by this. You can specify a PK with autoincrement in a lot of ways
that are all valid table definitions. The problem is that gambas only understands
some of them.

Here is the one that works: specify AUTOINCREMENT together with the column definition like so:
     ...
     person_id INTEGER PRIMARY KEY AUTOINCREMENT,
     ...

When AUTOINCREMENT is not part of the column definition, gambas will not recognize
the fact that there is no need to provide a person_id value for insertions.
 
As a consequence you would have to bind the person_id to a data-bound control
to manually let the user provide this id for each new record (which does not make much sense).

### Database file

Using the template db-file embedded within the project directory (db.sqlite) 
is possible but will only work as long as the application is not deployed. Usually applications
are deployed into a location that is read-only. This means that the embedded template db-file
will also be read-only. 

Therefor this application deploys (copies) the template db-file the directory where the 
temporary files of the current process are stored (usually in /tmp/gambas.<user id>/<process id>)
as file named [Application.Name].sqlite.
The application connects to this deployed db-file then.

Since this temp dir is automatically removed when the application process terminates, 
all changes you made during the processes lifetime will be lost. Each start of the application
will redeploy the template db-file again at startup.