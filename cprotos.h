/*	$Id$ */
/*
 * Copyright (c) 2020 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef CPROTOS_H
#define CPROTOS_H

void	print_func_db_close(int);
void	print_func_db_role(int);
void	print_func_db_role_current(int);
void	print_func_db_role_stored(int);
void	print_func_db_open(int);
void	print_func_db_open_logging(int);
void	print_func_db_insert(const struct strct *, int);
void	print_func_db_free(const struct strct *, int);
void	print_func_db_freeq(const struct strct *, int);
void	print_func_db_search(const struct search *, int);
void	print_func_db_set_logging(int);
void	print_func_db_trans_commit(int);
void	print_func_db_trans_open(int);
void	print_func_db_trans_rollback(int);
void	print_func_db_update(const struct update *, int);

size_t	 print_name_db_insert(const struct strct *);
size_t	 print_name_db_search(const struct search *);
size_t	 print_name_db_update(const struct update *);

void	 print_func_json_array(const struct strct *, int);
void	 print_func_json_clear(const struct strct *, int);
void	 print_func_json_data(const struct strct *, int);
void	 print_func_json_free_array(const struct strct *, int);
void	 print_func_json_iterate(const struct strct *, int);
void	 print_func_json_parse(const struct strct *, int);
void	 print_func_json_parse_array(const struct strct *, int);
void	 print_func_json_obj(const struct strct *, int);

void	 print_func_valid(const struct field *, int);

__END_DECLS

#endif /* !CPROTOS_H */
