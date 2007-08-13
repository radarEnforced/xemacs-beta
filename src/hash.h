/* This file is part of XEmacs.

XEmacs is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Synched up with: Not in FSF. */

#ifndef INCLUDED_hash_h_
#define INCLUDED_hash_h_

typedef struct
{
  CONST void *key;
  void	     *contents;
} hentry;

typedef int           (*hash_table_test_function) (CONST void *, CONST void *);
typedef unsigned long (*hash_table_hash_function) (CONST void *);
typedef size_t hash_size_t;

struct hash_table
{
  hentry	*harray;
  long		zero_set;
  void		*zero_entry;
  hash_size_t	size;		/* size of the hasharray */
  hash_size_t	fullness;	/* number of entries in the hash table */
  hash_table_hash_function hash_function;
  hash_table_test_function test_function;
};

/* SIZE is the number of initial entries. The hash table will be grown
   automatically if the number of entries approaches the size */
struct hash_table *make_hash_table (hash_size_t size);

struct hash_table *
make_general_hash_table (hash_size_t size,
			hash_table_hash_function hash_function,
			hash_table_test_function test_function);

/* Clear HASH-TABLE. A freshly created hash table is already cleared up. */
void clrhash (struct hash_table *hash_table);

/* Free HASH-TABLE and its substructures */
void free_hash_table (struct hash_table *hash_table);

/* Returns a hentry whose key is 0 if the entry does not exist in HASH-TABLE */
CONST void *gethash (CONST void *key, struct hash_table *hash_table,
		     CONST void **ret_value);

/* KEY should be different from 0 */
void puthash (CONST void *key, void *contents, struct hash_table *hash_table);

/* delete the entry with key KEY */
void remhash (CONST void *key, struct hash_table *hash_table);

typedef int (*maphash_function) (CONST void* key, void* contents, void* arg);

typedef int (*remhash_predicate) (CONST void* key, CONST void* contents,
                                  void* arg);

/* Call MF (key, contents, arg) for every entry in HASH-TABLE */
void maphash (maphash_function mf, struct hash_table *hash_table, void* arg);

/* Delete all objects from HASH-TABLE satisfying PREDICATE */
void map_remhash (remhash_predicate predicate,
		  struct hash_table *hash_table, void *arg);

#endif /* INCLUDED_hash_h_ */
