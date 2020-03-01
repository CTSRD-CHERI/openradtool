#ifndef DB_H
#define DB_H

/*
 * WARNING: automatically generated by ort-c-header 0.8.2.
 * DO NOT EDIT!
 */

#ifndef KWBP_VERSION
# define KWBP_VERSION "0.8.2"
#endif
#ifndef KWBP_VSTAMP
# define KWBP_VSTAMP 10903
#endif

struct	foo {
	char	*bar;
};

__BEGIN_DECLS

/*
 * Forward declaration of opaque pointer.
 */
struct ort;

/*
 * Set the argument given to the logging function specified to 
 * db_open_logging().
 * Has no effect if no logging function has been set.
 * The buffer is copied into a child process, so serialised objects may 
 * not have any pointers in the current address space or they will fail 
 * (at best).
 * Set length to zero to unset the logging function callback argument.
 */
void db_logging_data(struct ort *ort, const void *arg, size_t sz);

/*
 * Allocate and open the database in "file".
 * Returns an opaque pointer or NULL on memory exhaustion.
 * The returned pointer must be closed with db_close().
 * See db_open_logging() for the equivalent function that accepts logging 
 * callbacks.
 * This function starts a child with fork(), the child of which opens the 
 * database, so a constraint environment (e.g., with pledge) must take 
 * this into account.
 * Subsequent this function, all database operations take place over IPC.
 */
struct ort *db_open(const char *file);

/*
 * Like db_open() but accepts a function for logging.
 * If both are provided, the "long" form overrides the "short" form.
 * The logging function is run both in a child and parent process, so it 
 * must not have side effects.
 * The optional pointer is passed to the long form logging function and 
 * is inherited by the child process as-is, without being copied by 
 * value.
 * See db_logging_data() to set the pointer after initialisation.
 */
struct ort *db_open_logging(const char *file,
	void (*log)(const char *, void *),
	void (*log_short)(const char *, ...), void *log_arg);

/*
 * Open a transaction with identifier "id".
 * If "mode" is 0, the transaction is opened in "deferred" mode, meaning 
 * that the database is read-locked (no writes allowed) on the first read 
 * operation, and write-locked on the first write (only the current 
 * process can write).
 * If "mode" is >0, the transaction immediately starts a write-lock.
 * If "mode" is <0, the transaction starts in a write-pending, where no 
 * other locks can be held at the same time.
 * The DB_TRANS_OPEN_IMMEDIATE, DB_TRANS_OPEN_DEFERRED, and 
 * DB_TRANS_OPEN_EXCLUSIVE macros accomplish the same but with the "mode" 
 * being explicit in the name and not needing to be specified.
 */
void db_trans_open(struct ort *ctx, size_t id, int mode);

#define DB_TRANS_OPEN_IMMEDIATE(_ctx, _id) \
	db_trans_open((_ctx), (_id), 1)
#define DB_TRANS_OPEN_DEFERRED(_ctx, _id)\
	db_trans_open((_ctx), (_id), 0)
#define DB_TRANS_OPEN_EXCLUSIVE(_ctx, _id)\
	db_trans_open((_ctx), (_id), -1)

/*
 * Roll-back an open transaction.
 */
void db_trans_rollback(struct ort *ctx, size_t id);

/*
 * Commit an open transaction.
 */
void db_trans_commit(struct ort *ctx, size_t id);

/*
 * Close the context opened by db_open().
 * Has no effect if "p" is NULL.
 */
void db_close(struct ort *p);

/*
 * Clear resources and free "p".
 * Has no effect if "p" is NULL.
 */
void db_foo_free(struct foo *p);

/*
 * Updates the given fields in struct foo:
 * 	v1: bar (password)
 * Constrains the updated records to:
 * Returns zero on constraint violation, non-zero on success.
 */
int db_foo_update_bar_strset(struct ort *ctx, const char *v1);

__END_DECLS

#endif