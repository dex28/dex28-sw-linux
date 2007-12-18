/*
 * emfdb.c -- EMF database compatability functions for GoAhead WebServer.
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: emfdb.c,v 1.4 2003/09/29 19:48:08 bporter Exp $
 */

/******************************** Description *********************************/
/*
 *	Textfile-based database support for WebServer 2.1.
 */

/********************************* Includes ***********************************/

#include	"emfdb.h"
#include	"wsIntrn.h"

/********************************* Defines ************************************/

#define KEYWORD_TABLE	T("TABLE")
#define KEYWORD_ROW		T("ROW")

/*********************************** Locals ***********************************/

enum { MAX_DID = 16 };
static int dbDidAllocated[MAX_DID] = { 0 };

/*
 *	Variable to support the basicSet and basicGet functions.
 */

static char_t	*basicProdDir[MAX_DID] = { NULL };
static char_t	*basicDefaultDir = T("."); /* Default set to current */

/*
 * hAlloc chain list of table schemas to be closed 
 */

static int			dbMaxTables[MAX_DID] = { 0 };
static dbTable_t	**dbListTables[MAX_DID] = { NULL };

/****************************** Forward Declarations **************************/

static int		crack(char_t *buf, char_t **key, char_t **val);
static char_t	*trim(char_t *str);
static int		GetColumnIndex(int did, int tid, char_t *colName);

/******************************************************************************/
/* 
 *	Add a schema to the module-internal schema database 
 */

int dbRegisterDBSchema(int did, dbTable_t *pTableRegister)
{
	dbTable_t	*pTable;
	int			tid;

	a_assert(pTableRegister);

	trace(4, T("DB[%d]: Registering database table <%s>\n"), did, 
		pTableRegister->name);

/*
 *	Bump up the size of the table array
 */
	tid = hAllocEntry((void***) &dbListTables[did], 
		&dbMaxTables[did], sizeof(dbTable_t));	

/*
 *	Copy the table schema to the last spot in schema array
 */
	a_assert(dbListTables[did]);
	pTable = dbListTables[did][tid];
	a_assert(pTable);

/*
 *	Copy the name of the table
 */
	pTable->name = bstrdup(B_L, pTableRegister->name);

/*
 *	Copy the number of columns. If nColumns is not specified
 *  then we should go throught columns[] until we have found
 *  { NULL, 0 }, which serves as end of table marker.
 */
    if ( pTableRegister->nColumns > 0 )
	    pTable->nColumns = pTableRegister->nColumns;
    else
    {
        /* nColumns is not specified, and dbCol_t is { NULL, 0 } terminated */
        int i = 0;
        while( pTableRegister->columns[i].Name )
        {
            i++;
        }
        pTable->nColumns = i;
    }

/*
 *	Copy the column definitions
 */
	if (pTable->nColumns > 0) {
		int i;
		pTable->columns = balloc(B_L, sizeof(dbCol_t) * pTable->nColumns);

		for (i = 0; i < pTable->nColumns; i++) {
			pTable->columns[i].Name = 
				bstrdup(B_L, pTableRegister->columns[i].Name);
			pTable->columns[i].Type = pTableRegister->columns[i].Type;
			pTable->columns[i].DefaultValue = pTableRegister->columns[i].DefaultValue;
		}

	} else {
		pTable->columns = NULL;
	}

/*
 *	Zero out the table's data (very important!)
 */
	pTable->nRows = 0;
	pTable->rows = NULL;

	return 0;
}

/******************************************************************************/
/*
 *	This is provided for compatibility with EMF.  Tables are "registered"
 *	with staticly defined schemas.  There is only one did in this package: 0.
 */

int dbOpen(char_t *tablename, char_t *filename, 
		   int (*gettime)(int did), int flags)
{
	int did = -1;
/*
 *	Search for DID that is not allocated.
 */
	int i;
	for ( i = 0; did < 0 && i < MAX_DID; i++ )
	{
		if ( ! dbDidAllocated[ i ] )
			did = i;
	}
/*
 *	MAX_DID reached -- dbOpen is failed
 */
	if ( did < 0 )
	{
		trace( 0, T("DB[%d]: Open failed to obtain DID.\n"), did );
		return -1;
	}
/*
 *	initialize data associated with DID
 */
	dbDidAllocated[ did ] = 1;

	trace( 0, T("DB[%d]: Open succeded.\n"), did );

	basicProdDir[did] = NULL;		
	dbMaxTables[did] = 0;
	dbListTables[did] = NULL;

	return did;
}

/******************************************************************************/
/*
 *	Delete all the rows of the tables, and all of the tables
 */

void dbClose(int did)
{
	int			table, column;
	dbTable_t	*pTable;

	trace( 0, T("DB[%d]: Closed.\n"), did );
/*
 *	Before doing anything, delete all the contents of the database
 */
	dbZero(did);

/*
 *	Now delete the tables
 */
	for (table = 0; table < dbMaxTables[did]; table++) {
		pTable = dbListTables[did][table];

		if (pTable != NULL) {
/*
 *			Delete the table schema
 */
			if (pTable->nColumns) {
				for (column = 0; column < pTable->nColumns; column++) {
					bfreeSafe(B_L, pTable->columns[column].Name);
				}
				bfreeSafe(B_L, pTable->columns);
			}
/*
 *			Delete the table name
 */
			bfreeSafe(B_L, pTable->name);
/*
 *			Free the table
 */
			bfreeSafe(B_L, pTable);
			hFree((void ***) &dbListTables[did], table);
		}
	}

	if (dbListTables[did]) {
		bfree(B_L, dbListTables[did]);
	}

/*
 *	Set the global table list to a safe value
 */
	dbListTables[did] = NULL;
	dbMaxTables[did] = 0;

/*
 *	Make DID free.
 */
	dbDidAllocated[did] = 0;
}


/******************************************************************************/
/*
 *	Delete all the data records in all tables
 */

void dbZero(int did)
{
	int			table, row, column, nRows, nColumns;
	int			*pRow;
	dbTable_t	*pTable;

/*
 *	Delete all data from all tables
 */
	for (table = 0; table < dbMaxTables[did]; table++) {
		pTable = dbListTables[did][table];
/*
 *		Delete the row data contained within the schema
 */
		if (pTable) {
			nColumns = pTable->nColumns;
			nRows = pTable->nRows;
			for (row = 0; row < nRows; row++) {
				pRow = pTable->rows[row];
				if (pRow) {
/*
 *					Only delete the contents of rows not previously deleted!
 */
					for (column = 0; column < nColumns; column++) {
						if (pTable->columns[column].Type == T_STRING) {
							bfreeSafe(B_L, (char_t *)(pRow[column]));
							pRow[column] = (int)NULL;
						}
					}

					bfreeSafe(B_L, pRow);
					hFree((void ***) &pTable->rows, row);
				}
			}

			pTable->rows = NULL;
			pTable->nRows = 0;
		}
	}
}

/******************************************************************************/
/*
 *	Find the a row in the table with the given string in the given column
 */

int dbSearchStr(int did, char_t *tablename, 
	char_t *colName, char_t *value, int flags)
{
	int			tid, nRows, nColumns, column;
   int match = 0;
	dbTable_t	*pTable;

	a_assert(tablename);
	a_assert(colName);
	a_assert(value);

	tid = dbGetTableId(did, tablename);
	a_assert(tid >= 0);

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		pTable = dbListTables[did][tid];
	} else {
		return DB_ERR_TABLE_NOT_FOUND;
	}
	
	nColumns = pTable->nColumns;
	nRows = pTable->nRows;
	column = GetColumnIndex(did,tid, colName);
	a_assert (column >= 0);

	if (column >= 0) {
		char_t	*compareVal;
		int		row, *pRow;
/*
 *		Scan through rows until we find a match.
 *		Note that some of these rows may be deleted!
 */
		row = 0;
		while (row < nRows) {
			pRow = pTable->rows[row];
			if (pRow) {
				compareVal = (char_t *)(pRow[column]); 
            if (NULL != compareVal)
            {
              if (DB_CASE_INSENSITIVE == flags)
              {
                 match = gstricmp(compareVal, value);
              }
              else
              {
                 match = gstrcmp(compareVal, value);
              }
              if (0 == match)
              {
                 return row;
              }
            }
			}
			row++;
		}
	} else { 
/*
 *		Return -2 if search column was not found
 */
		trace(3, T("DB[%d]: Unable to find column <%s> in table <%s>\n"), did,
			colName, tablename);
		return DB_ERR_COL_NOT_FOUND;
	}

	return -1;
}

/******************************************************************************/
/*
 *	Add a new row to the given table.  Return the new row ID.
 */

int dbAddRow(int did, char_t *tablename)
{
	int			tid, size;
	dbTable_t	*pTable;

	a_assert(tablename);

	tid = dbGetTableId(did, tablename);
	a_assert(tid >= 0);

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		pTable = dbListTables[did][tid];
	} else {
		return DB_ERR_TABLE_NOT_FOUND;
	}

	a_assert(pTable);

	if (pTable) {
        int row;
        int nColumn;
		trace(5, T("DB[%d]: Adding a row to table <%s>\n"), did, tablename);

		size = pTable->nColumns * max(sizeof(int), sizeof(char_t *));
		row = hAllocEntry((void***) &(pTable->rows), &(pTable->nRows), size);

        // Fill default values
        //
        for ( nColumn = 0; nColumn < pTable->nColumns; nColumn++ )
        {

            if ( pTable->columns[nColumn].Type == T_STRING ) 
            {
                if ( pTable->columns[nColumn].DefaultValue )
                {
                    //	Make sure we make a copy of the string to write into the column.
                    //	This allocated string will be deleted when the row is deleted.
                    //
                    pTable->rows[row][nColumn] = (int)bstrdup(B_L,(char_t*)pTable->columns[nColumn].DefaultValue);
                }
            } 
            else // T_INT
            {
                pTable->rows[row][nColumn] = pTable->columns[nColumn].DefaultValue;
            }
		}

        return row;
	} 

	return -1;
}

/******************************************************************************/
/*
 *	Delete a row in the table.  
 */

int dbDeleteRow(int did, char_t *tablename, int row)
{
	int			tid, nColumns, nRows;
	dbTable_t	*pTable;

	a_assert(tablename);
	tid = dbGetTableId(did, tablename);
	a_assert(tid >= 0);

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		pTable = dbListTables[did][tid];
	} else {
		return DB_ERR_TABLE_NOT_FOUND;
	}

	nColumns = pTable->nColumns;
	nRows = pTable->nRows;

	if ((row >= 0) && (row < nRows)) {
		int *pRow = pTable->rows[row];

		if (pRow) {
			int	column = 0;
/*
 *			Free up any allocated strings
 */
			while (column < nColumns) {
				if (pRow[column] && 
					(pTable->columns[column].Type == T_STRING)) {
					bfree(B_L, (char_t *)pRow[column]);
				}

				column++;
			}
/*
 *			Zero out the row for safety
 */
			memset(pRow, 0, nColumns * max(sizeof(int), sizeof(char_t *)));

			bfreeSafe(B_L, pRow);
			pTable->nRows = hFree((void ***)&pTable->rows, row);
			trace(5, T("DB[%d]: Deleted row <%d> from table <%s>\n"), did,
				row, tablename);
		}
		return 0;
	} else {
		trace(3, T("DB[%d]: Unable to delete row <%d> from table <%s>\n"), did,
			row, tablename);
	}
	
	return -1;
}

/*****************************************************************************/
/*
 *	Grow the rows in the table to the nominated size. 
 */

int dbSetTableNrow(int did, char_t *tablename, int nNewRows)
{
	int			nRet, tid, nRows, nColumns;
	dbTable_t	*pTable;

	a_assert(tablename);
	tid = dbGetTableId(did, tablename);
	a_assert(tid >= 0) ;

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		pTable = dbListTables[did][tid];
	} else {
		return DB_ERR_TABLE_NOT_FOUND;
	}

	nRet = -1;

	a_assert(pTable);
	if (pTable) {
		nColumns = pTable->nColumns;
		nRows = pTable->nRows;
		nRet = 0;

		if (nRows >= nNewRows) {
/*		
 *		If number of rows already allocated exceeds requested number, do nothing
 */
			trace(4, T("DB[%d]: Ignoring row set to <%d> in table <%s>\n"), did,
				nNewRows, tablename);
		} else {
			trace(4, T("DB[%d]: Setting rows to <%d> in table <%s>\n"), did,
				nNewRows, tablename);
			while (pTable->nRows < nNewRows) {
				if (dbAddRow(did, tablename) < 0) {
					return -1;
				}
			}
		}
	} 

	return nRet;
}

/******************************************************************************/
/*
 *	Return the number of rows in the given table
 */

int dbGetTableNrow(int did, char_t *tablename)
{
	int tid;
	
	a_assert(tablename);
	tid = dbGetTableId(did, tablename);

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		return (dbListTables[did][tid])->nRows;
	} else {
		return -1;
	}
}

/******************************************************************************/
/*
 *	Do table driven read of the database
 */

int dbReadInt(int did, char_t *table, char_t *column, int row, int *returnValue)
{
	int			colIndex, *pRow, tid;
	dbTable_t	*pTable;
	
	a_assert(table);
	a_assert(column);
	a_assert(returnValue);

	tid = dbGetTableId(did, table);
	a_assert(tid >= 0);

/*
 *	Return -6 if table is not found
 */
	if (tid < 0) {
		return DB_ERR_TABLE_NOT_FOUND;
	}

/*
 *	Return -7 if table id has been deleted
 */
	pTable = dbListTables[did][tid];
	if (pTable == NULL) {
		return DB_ERR_TABLE_DELETED;
	}

	a_assert(row >= 0);

	if ((row >= 0) && (row < pTable->nRows)) {
		colIndex = GetColumnIndex(did,tid, column);
		a_assert(colIndex >= 0);

		if (colIndex >= 0) {
			pRow = pTable->rows[row];
			if (pRow) {
				*returnValue = pRow[colIndex];
				return 0;
			}  
			return DB_ERR_ROW_DELETED;
		}
		return DB_ERR_COL_NOT_FOUND;
	}

	return DB_ERR_ROW_NOT_FOUND;
}

/******************************************************************************/
/*
 *	dbReadStr calls dbReadInt to do table driven read of database
 */

int dbReadStr(int did, char_t *table, char_t *column, int row,
		char_t **returnValue)
{
	return dbReadInt(did, table, column, row, (int *)returnValue);
}

/******************************************************************************/
/*
 *	The dbWriteInt function writes a value into a table at a given row and
 *	column.  The existence of the row and column is verified before the 
 *	write.  0 is returned on succes, -1 is returned on error.
 */

int dbWriteInt(int did, char_t *table, char_t *column, int row, int iData)
{
	int			tid, colIndex, *pRow;
	dbTable_t	*pTable;

	a_assert(table);
	a_assert(column);

/*
 *	Make sure that this table exists
 */
	tid = dbGetTableId(did, table);
	a_assert(tid >= 0);

	if (tid < 0) {
		return DB_ERR_TABLE_NOT_FOUND;
	}

	pTable = dbListTables[did][tid];
	
	if (pTable) {
/*
 *		Make sure that the column exists
 */
		colIndex = GetColumnIndex(did,tid, column);
		a_assert(colIndex >= 0);
		if (colIndex >= 0) {
/*
 *			Make sure that the row exists
 */
			a_assert((row >= 0) && (row < pTable->nRows));
			if ((row >= 0) && (row < pTable->nRows)) {
				pRow = pTable->rows[row];
				if (pRow) {
					pRow[colIndex] = iData;
					return 0;
				}
				return DB_ERR_ROW_DELETED;
			}
			return DB_ERR_ROW_NOT_FOUND;
		}
		return DB_ERR_COL_NOT_FOUND;
	}

	return DB_ERR_TABLE_DELETED;
}

/******************************************************************************/
/*
 *	The dbWriteStr function writes a string value into a table at a given row 
 *	and column.  The existence of the row and column is verified before the 
 *	write.  The column is also checked to confirm it is a string field.
 *	0 is returned on succes, -1 is returned on error.
 */

int dbWriteStr(int did, char_t *table, char_t *column, int row, char_t *s)
{
	int			tid, colIndex;
	int			*pRow;
	char_t		*ptr;
	dbTable_t	*pTable;

	a_assert(table);
	a_assert(column);

	tid = dbGetTableId(did, table);
	a_assert(tid >= 0);

	if (tid < 0) {
		return DB_ERR_TABLE_NOT_FOUND;
	}

/*
 *	Make sure that this table exists
 */
	pTable = dbListTables[did][tid];
	a_assert(pTable);
	if (!pTable) {
		return DB_ERR_TABLE_DELETED;
	}

/*
 *	Make sure that this column exists
 */
	colIndex = GetColumnIndex(did,tid, column);
	if (colIndex < 0) {
		return DB_ERR_COL_NOT_FOUND;
	}

/*
 *	Make sure that this column is a string column
 */
	if (pTable->columns[colIndex].Type != T_STRING) {
		return DB_ERR_BAD_FORMAT;
	}

/*
 *	Make sure that the row exists
 */
	a_assert((row >= 0) && (row < pTable->nRows));
	if ((row >= 0) && (row < pTable->nRows)) {
		pRow = pTable->rows[row];
	} else {
		return DB_ERR_ROW_NOT_FOUND;
	}

	if (!pRow) {
		return DB_ERR_ROW_DELETED;
	}

/*
 *	If the column already has a value, be sure to delete it to prevent
 *	memory leaks.
 */
	if (pRow[colIndex]) {
		bfree(B_L, (char_t *) pRow[colIndex]);
	}

/*
 *	Make sure we make a copy of the string to write into the column.
 *	This allocated string will be deleted when the row is deleted.
 */
	ptr = bstrdup(B_L, s);
	pRow[colIndex] = (int)ptr;

	return 0;
}

/******************************************************************************/
/*
 *	Print a key-value pair to a file
 */

static int dbWriteKeyValue(int fd, char_t *key, char_t *value)
{
	int		rc;
	int		len;
	char_t	*pLineOut;

	a_assert(key && *key);
	a_assert(value);
	
	fmtAlloc(&pLineOut, BUF_MAX, T("%s=%s\n"), key, value);

	if (pLineOut) {
		len = gstrlen(pLineOut);
#ifdef CE
		rc = writeUniToAsc(fd, pLineOut, len);
#else
		rc = gwrite(fd, pLineOut, len);
#endif
		bfree(B_L, pLineOut);
	} else {
		rc = -1;
	}

	return rc;
}

/******************************************************************************/
/*
 *	Persist a database to a file
 */

int dbSave(int did, char_t *filename, int flags)
{
	int			row, column, nColumns, nRows, fd, rc;
	int			*pRow, nRet, tid;
	char_t		*path, *tmpFile, *tmpNum;
	dbTable_t	*pTable;

	trace(5, T("DB[%d]: About to save database to file\n"), did );

	a_assert(dbMaxTables[did] > 0);

/*
 *	First write to a temporary file, then switch around later.
 */
	fmtAlloc(&tmpFile, FNAMESIZE, T("%s/data.tmp"), basicGetProductDir(did));
	if ((fd = gopen(tmpFile, 
		O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666)) < 0) {
		trace(1, T("DB[%d]: WARNING: Failed to open file %s\n"), did, tmpFile);
		bfree(B_L, tmpFile);
		return -1;
	}

	nRet = 0;

	for (tid = 0; (tid < dbMaxTables[did]) && (nRet != -1); tid++) {
		pTable = dbListTables[did][tid];

		if (pTable) {
/*
 *			Print the TABLE=tableName directive to the file
 */
			rc = dbWriteKeyValue(fd, KEYWORD_TABLE, pTable->name);

			nColumns = pTable->nColumns;
			nRows = pTable->nRows;

			for (row = 0; (row < nRows) && (nRet == 0); row++) {
				pRow = pTable->rows[row];
/*
 *				if row is NULL, the row has been deleted, so don't
 *				write it out.
 */
				if ((pRow == NULL) || (pRow[0] == '\0') || 
					(*(char_t *)(pRow[0]) == '\0')) {
					continue;
				}
/*
 *				Print the ROW=rowNumber directive to the file
 */
				fmtAlloc(&tmpNum, 20, T("%d"), row);		
				rc = dbWriteKeyValue(fd, KEYWORD_ROW, tmpNum);
				bfreeSafe(B_L, tmpNum);

/*
 *				Print the key-value pairs (COLUMN=value) for data cells
 */
				for (column = 0; (column < nColumns) && (rc >= 0); column++) {
					if (pTable->columns[column].Type == T_STRING) {
						rc = dbWriteKeyValue(fd, pTable->columns[column].Name,
							(char_t *)(pRow[column]));
					} else {
						fmtAlloc(&tmpNum, 20, T("%d"), pRow[column]);		
						rc = dbWriteKeyValue(fd, pTable->columns[column].Name,
                            tmpNum);
						bfreeSafe(B_L, tmpNum);
					}
				}

				if (rc < 0) {
					trace(1, T("WARNING: Failed to write to file %s\n"), 
						did, tmpFile);
					nRet = -1;
				}
			}
		}
	}

	gclose(fd);

/*
 *	Replace the existing file with the temporary file, if no errors
 */
	if (nRet == 0) {
		fmtAlloc(&path, FNAMESIZE, T("%s/%s"), basicGetProductDir(did), filename);

		gunlink(path);
		if (grename(tmpFile, path) != 0) {
			trace(1, T("DB[%d]: WARNING: Failed to rename %s to %s\n"), did, tmpFile, path);
			nRet = -1;
		}

		bfree(B_L, path);
	}

	bfree(B_L, tmpFile);

	return nRet;
}

/******************************************************************************/
/*
 *	Crack a keyword=value string into keyword and value. We can change buf.
 */

static int crack(char_t *buf, char_t **key, char_t **val)
{
	char_t	*ptr;

	if ((ptr = gstrrchr(buf, '\n')) != NULL ||
			(ptr = gstrrchr(buf, '\r')) != NULL) {
		*ptr = '\0';
	}

/*
 *	Find the = sign. It must exist.
 */
	if ((ptr = gstrstr(buf, T("="))) == NULL) {
		return -1;
	}

	*ptr++ = '\0';
	*key = trim(buf);
	*val = trim(ptr);

	return 0;
}

/******************************************************************************/
/*
 *	Parse the file. These files consist of key-value pairs, separated by the 
 *  "=" sign. Parsing of tables starts with the "TABLE=value" pair, and rows
 *	are parsed starting with the "ROW=value" pair.
 */

int dbLoad(int did, char_t *filename, int flags)
{
	gstat_t		sbuf;
	char_t		*buf, *keyword, *value, *path, *ptr;
	char_t		*tablename;
	int			fd, tid, row;
	dbTable_t	*pTable;

    a_assert(did >= 0);

	fmtAlloc(&path, FNAMESIZE, T("%s/%s"), basicGetProductDir(did), filename);
	trace(4, T("DB[%d]: About to read data file <%s>\n"), did, path);

	if (gstat(path, &sbuf) < 0) {
		trace(3, T("DB[%d]: Failed to stat persistent data file.\n"), did);
		bfree(B_L, path);
		return -1;
	}

	fd = gopen(path, O_RDONLY | O_BINARY, 0666);
	bfree(B_L, path);

	if (fd < 0) {
		trace(3, T("DB[%d]: No persistent data file present.\n"), did);
		return -1;
	}

	if (sbuf.st_size <= 0) {
		trace(3, T("DB[%d]: Persistent data file is empty.\n"), did);
		gclose(fd);
		return -1;
	}
/*
 *	Read entire file into temporary buffer
 */
	buf = balloc(B_L, sbuf.st_size + 1);
#ifdef CE
	if (readAscToUni(fd, &buf, sbuf.st_size) != (int)sbuf.st_size) {
#else
	if (gread(fd, buf, sbuf.st_size) != (int)sbuf.st_size) {
#endif
		trace(3, T("DB[%d]: Persistent data read failed.\n"), did);
		bfree(B_L, buf);
		gclose(fd);
		return -1;
	}

	gclose(fd);
	*(buf + sbuf.st_size) = '\0';

	row = -1;
	tid = -1;
	pTable = NULL;
	ptr = gstrtok(buf, T("\n"));
	tablename = NULL;

	do {
		if (crack(ptr, &keyword, &value) < 0) {
			trace(5, T("DB[%d]: Failed to crack line %s\n"), did, ptr);
			continue;
		}

		a_assert(keyword && *keyword);

		if (gstrcmp(keyword, KEYWORD_TABLE) == 0) {
/*
 *			Table name found, check to see if it's registered
 */
			if (tablename) {
				bfree(B_L, tablename);
			}

			tablename = bstrdup(B_L, value);
			tid = dbGetTableId(did, tablename);

			if (tid >= 0) {
				pTable = dbListTables[did][tid];
			} else {
				pTable = NULL;
			}

		} else if (gstrcmp(keyword, KEYWORD_ROW) == 0) {
/*
 *			Row/Record indicator found, add a new row to table
 */
			if (tid >= 0) {
				int nRows = dbGetTableNrow(did, tablename);

				if (dbSetTableNrow(did, tablename, nRows + 1) == 0) {
					row = nRows;
				}
			}

		} else if (row != -1) {
/*
 *			some other data found, assume it's a COLUMN=value
 */
			int nColumn = GetColumnIndex(did,tid, keyword);

			if ((nColumn >= 0) && (pTable != NULL)) {
				int nColumnType = pTable->columns[nColumn].Type;
				if (nColumnType == T_STRING) {
					dbWriteStr(did, tablename, keyword, row, value);
				} else {
					dbWriteInt(did, tablename, keyword, row, gstrtoi(value));
				}
			}
		}
	} while ((ptr = gstrtok(NULL, T("\n"))) != NULL);

	if (tablename) {
		bfree(B_L, tablename);
	}

	bfree(B_L, buf);

	return 0;
}

/******************************************************************************/
/*
 *	Return a table id given the table name
 */

int dbGetTableId(int did, char_t *tablename)
{
	int			tid;
	dbTable_t	*pTable;

	a_assert(tablename);

	for (tid = 0; (tid < dbMaxTables[did]); tid++) {
		if ((pTable = dbListTables[did][tid]) != NULL) {
			if (gstrcmp(tablename, pTable->name) == 0) {
				return tid;
			}
		}
	}
	
	return -1;
}

/******************************************************************************/
/*	
 *	Return a pointer to the table name, given its ID
 */

char_t *dbGetTableName(int did, int tid)
{
	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		return (dbListTables[did][tid])->name;
	}

	return NULL;
}

/******************************************************************************/
/*
 *	Trim leading white space.
 */

static char_t *trim(char_t *str)
{
	while (isspace((int)*str)) {
		str++;
	}
	return str;
}

/******************************************************************************/
/*
 *	Return a column index given the column name
 */

static int GetColumnIndex(int did, int tid, char_t *colName) 
{
	int			column;
	dbTable_t	*pTable;

	a_assert(colName);

	if ((tid >= 0) && (tid < dbMaxTables[did]) && (dbListTables[did][tid] != NULL)) {
		pTable = dbListTables[did][tid];

		for (column = 0; (column < pTable->nColumns); column++) {
			if (gstrcmp(colName, pTable->columns[column].Name) == 0)
				return column;
		}
	}

	return -1;
}

/******************************************************************************/
/*
 *	Set the prefix-directory
 */

void basicSetProductDir(int did, char_t *proddir)
{
	int len;

	if (basicProdDir[did] != NULL) {	
      bfree(B_L, basicProdDir[did]);
	}
    
	basicProdDir[did] = bstrdup(B_L, proddir);
/*
 *	Make sure that prefix-directory doesn't end with a '/'
 */
	len = gstrlen(basicProdDir[did]);
	if ((len > 0) && *(basicProdDir[did] + len - 1) == '/') {
		*(basicProdDir[did]+len-1) = '\0';
	}
}

/******************************************************************************/
/*
 *	Return the prefix-directory
 */

char_t *basicGetProductDir(int did)
{
	if (basicProdDir[did]) {
		return basicProdDir[did];
	} else {
		return basicDefaultDir;
	}
}

/******************************************************************************/

void* dbGetRowPtr(int did, char_t *table, int row )
{
	int			tid;
	dbTable_t	*pTable;
	
	a_assert(table);
	a_assert(column);
	a_assert(returnValue);

	tid = dbGetTableId(did, table);
	a_assert(tid >= 0);

/*
 *	Return -6 if table is not found
 */
	if (tid < 0) {
		return NULL;
	}

/*
 *	Return -7 if table id has been deleted
 */
	pTable = dbListTables[did][tid];
	if (pTable == NULL) {
		return NULL;
	}

	a_assert(row >= 0);

	if ((row >= 0) && (row < pTable->nRows)) {
       	return pTable->rows[row];
	}

	return NULL;
}

void dbUpdateStr( char_t** ptr, char_t* new_value )
{
    a_assert(ptr);
    a_assert(new_value);

	if ( *ptr )
		bfree( B_L, *ptr );

	*ptr = bstrdup( B_L, new_value );
    trace( 5, "Setting %s\n", new_value );
}
