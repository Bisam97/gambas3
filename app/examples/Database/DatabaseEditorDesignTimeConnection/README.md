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

### Design-time connection and db-file

The design-time definition of a sqlite connection needs the path and filename of the db-file
to be used. Using the template db-file embedded within the project directory (db.sqlite) 
is possible but will only work as long as the application is not deployed. Usually applications
are deployed into a location that is read-only. This means that the embedded template db-file
will also be read-only. 
Therefor this application deploys (copies) the template db-file to the /tmp dir at startup
(if not already present) as [Application.Name].sqlite. 
The Path and Database (design-time) properties of the used connection therefor point to this
deployed file as well. 

To be able to use the DB metadata (table names, field names, ...) at design-time in controls,
you either have to start the application once (which deploys the db-file to the location that
is configured for the connection), or copy the template db-file manually from the project directory
to /tmp/[Application.Name].sqlite. 
