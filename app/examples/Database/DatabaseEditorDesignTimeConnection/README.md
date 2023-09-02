## README

This example uses a sqlite3 DB.
One speciality with respect to gambas is that you have to be careful how to 
define the schema. gambas will read this schema definition later and extract
some information from it.

Expecially the autoincrement feature often used for primary key columns
is affected by this. You can specify a PK with autoincrement in a lot of ways
that are all valid table definitions. The problem is that gambas only understands
some of then.

Here is the one that works: specify AUTOINCREMENT together with the column definition like so:
     ...
     person_id INTEGER PRIMARY KEY AUTOINCREMENT,
     ...

When AUTOINCREMENT is not part of the column definition, gambas will not recognize
the fact that there is no need to provide a person_id value for insertions.
 
As a consequence you would have to bind the person_id to a data-bound control
to manually let the user provide this id for each new record (which does not make much sense).