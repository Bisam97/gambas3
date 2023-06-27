# Edit

@{syntax}

Returns a read/write [../../result] object used for editing records in the specified
table.

* ~Table~ is the table name.
* ~Request~ is a SQL WHERE clause used for filtering the table (without the WHERE keyword).
* ~Arguments~ are quoted as needed by the SQL syntax, and substituted inside the ~Request~ string

The last feature allows you to write requests that are independant of the underlying database type.

Once you have gotten the Result object, you can modify some of its fields. Then you can call the [Result.Update] (../../result/update) method
to send the changes to the database (i.e. omitting the corresponding UPDATE SQL statement).

### Examples
[[ code gambas
DIM hResult AS Result
DIM sCriteria AS String
DIM iParemeter AS Integer

sCriteria = "id = &1"
iParameter = 1012

$hConn.Begin

' Same as "SELECT * FROM tblDEFAULT WHERE id = 1012"
hResult = $hConn.Edit("tblDEFAULT", sCriteria, iParameter)
' Set field value
hResult!Name = "Mr Smith"

' Update the value
hResult.Update
$hConn.Commit
]]

### See also
- [Result.Update] (../../result/update) 
- [../delete] 
- [../find] 
- [../exec] 
- [../subst]
- [/doc/db-quoting]

