#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "hash.h"
#include "file.h"
#include "scanner.h"

static const char scanner_peek_char(scanner_t* scanner) {
	if (scanner->length == scanner->position)
		return 0;
	return scanner->source[scanner->position];
}

static const char scanner_read_char(scanner_t* scanner) {
	if (scanner->length == scanner->position)
		return scanner->last_char = 0;
	scanner->col++;
	if (scanner->source[scanner->position] == '\n') {
		scanner->col = 0;
		scanner->row++;
	}
	return scanner->last_char = scanner->source[scanner->position++];
}

void init_scanner(scanner_t* scanner, const char* source, const uint32_t length, int init_scan) {
	scanner->source = source;
	scanner->length = length;
	scanner->position = 0;
	scanner->row = 1;
	scanner->col = 0;
	scanner->last_err = ERROR_NONE;
	if (init_scan) {
		scanner_read_char(scanner);
		scanner_scan_tok(scanner);
	}
}

#define SET_CHAR_TYPE(TYPE) { scanner->last_char = TYPE; break; }
const int scanner_scan_char(scanner_t* scanner) {
	scanner_read_char(scanner);
	if (scanner->last_char == '\\') {
		scanner_read_char(scanner);
		switch (scanner->last_char)
		{
		case 'b':
			SET_CHAR_TYPE('\b');
		case 'e':
			SET_CHAR_TYPE('\e');
		case 'n':
			SET_CHAR_TYPE('\n');
		case 'r':
			SET_CHAR_TYPE('\r');
		case 't':
			SET_CHAR_TYPE('\t');
		case '\\':
			SET_CHAR_TYPE('\\');
		case '\"':
			SET_CHAR_TYPE('\"');
		case '0':
			SET_CHAR_TYPE(0);
		default:
			PANIC(scanner, ERROR_UNEXPECTED_TOK);
		}
	}
	return 1;
}

#define SET_TOK_TYPE(TYPE) {scanner->last_tok.type = TYPE; break;}
const int scanner_scan_tok(scanner_t* scanner) {
	while (scanner->last_char == ' ' || scanner->last_char == '\t' || scanner->last_char == '\r' || scanner->last_char == '\n')
		scanner_read_char(scanner);

	scanner->last_tok.str = &scanner->source[scanner->position - 1];
	scanner->last_tok.length = 0;

	if (isalpha(scanner->last_char) || scanner->last_char == '_') {
		do {
			scanner_read_char(scanner);
			scanner->last_tok.length++;
		} while (isalpha(scanner->last_char) || isalnum(scanner->last_char) || scanner->last_char == '_');
		uint64_t id_hash = hash_s(scanner->last_tok.str, scanner->last_tok.length);
		switch (id_hash)
		{
		case 7572877634356771:
			SET_TOK_TYPE(TOK_READONLY);
		case 229466054363183:
			SET_TOK_TYPE(TOK_FOREIGN);
		case 7572251799911306: //continue
			SET_TOK_TYPE(TOK_CONTINUE);
		case 210707980106: //break
			SET_TOK_TYPE(TOK_BREAK);
		case 6385087377: //bool
			SET_TOK_TYPE(TOK_TYPECHECK_BOOL);
		case 6385115235: //char
			SET_TOK_TYPE(TOK_TYPECHECK_CHAR);
		case 193495088: //long
			SET_TOK_TYPE(TOK_TYPECHECK_LONG);
		case 210712519067: //float
			SET_TOK_TYPE(TOK_TYPECHECK_FLOAT);
		case 210706808356: //array
			SET_TOK_TYPE(TOK_TYPECHECK_ARRAY);
		case 6385593753: //proc
			SET_TOK_TYPE(TOK_TYPECHECK_PROC);
		case 229484523888481: //typearg
			SET_TOK_TYPE(TOK_TYPEARG);
		case 229476388586812: //nothing
			SET_TOK_TYPE(TOK_NOTHING);
		case 6385058142: //auto
			SET_TOK_TYPE(TOK_AUTO);
		case 6953552265174: //global
			SET_TOK_TYPE(TOK_GLOBAL);
		case 5863476: //if
			SET_TOK_TYPE(TOK_IF);
		case 6385191717: //elif
			SET_TOK_TYPE(TOK_ELIF);
		case 6385192046: //else
			SET_TOK_TYPE(TOK_ELSE);
		case 210732529790: //while
			SET_TOK_TYPE(TOK_WHILE);
		case 6953974653989:
			SET_TOK_TYPE(TOK_RETURN);
		case 6385737701: //true
			SET_TOK_TYPE(TOK_TRUE);
		case 210712121072: //false
			SET_TOK_TYPE(TOK_FALSE);
		case 193486360: //and
			SET_TOK_TYPE(TOK_AND);
		case 5863686: //or
			SET_TOK_TYPE(TOK_OR);
		case 193500239:
			SET_TOK_TYPE(TOK_NEW);
		case 229469872107401:
			SET_TOK_TYPE(TOK_INCLUDE);
		case 193504585: //rem
			do {
				scanner_read_char(scanner);
			} while (scanner->last_char != '\n');
			return scanner_scan_tok(scanner);
		default:
			SET_TOK_TYPE(TOK_IDENTIFIER);
		}
	}
	else if (isalnum(scanner->last_char)) {
		do {
			scanner_read_char(scanner);
			scanner->last_tok.length++;
		} while (isalnum(scanner->last_char) || scanner->last_char == '.');
		scanner->last_tok.type = TOK_NUMERICAL;
	}
	else if (scanner->last_char == '\"') {
		scanner->last_tok.type = TOK_STRING;
		scanner->last_tok.str++;
		uint32_t old_pos = scanner->position;
		while (scanner_peek_char(scanner) != '\"')
		{
			if (!scanner_scan_char(scanner) || !scanner_peek_char(scanner))
				PANIC(scanner, ERROR_UNEXPECTED_TOK);
		}
		scanner->last_tok.length = scanner->position - old_pos;
		scanner_read_char(scanner);
		scanner_read_char(scanner);
	}
	else if (scanner->last_char == '\'') {
		scanner->last_tok.type = TOK_CHAR;
		scanner->last_tok.str++;
		uint32_t old_pos = scanner->position;
		if(!scanner_scan_char(scanner) || !scanner_peek_char(scanner))
			PANIC(scanner, ERROR_UNEXPECTED_TOK);
		scanner->last_tok.length = scanner->position - old_pos;
		if (!scanner_read_char(scanner) || scanner->last_char != '\'')
			PANIC(scanner, ERROR_UNEXPECTED_TOK);
		scanner_read_char(scanner);
	}
	else {
		const char next_char = scanner_peek_char(scanner);
		
		switch (scanner->last_char)
		{
		case '#':
			SET_TOK_TYPE(TOK_HASHTAG);
		case ';':
			SET_TOK_TYPE(TOK_SEMICOLON);
		case '+':
			SET_TOK_TYPE(TOK_ADD)
		case '-':
			SET_TOK_TYPE(TOK_SUBTRACT)
		case '*':
			SET_TOK_TYPE(TOK_MULTIPLY)
		case '/':
			SET_TOK_TYPE(TOK_DIVIDE)
		case '%':
			SET_TOK_TYPE(TOK_MODULO)
		case '^':
			SET_TOK_TYPE(TOK_POWER)
		case '=':
			if (next_char == '=') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_EQUALS)
			}
			else
				SET_TOK_TYPE(TOK_SET)
		case '!':
			if (next_char == '=') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_NOT_EQUAL)
			}
			else
				SET_TOK_TYPE(TOK_NOT)
		case '>':
			if (next_char == '=') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_MORE_EQUAL);
			}
			else
				SET_TOK_TYPE(TOK_MORE)
		case '<':
			if (next_char == '=') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_LESS_EQUAL)
			}
			else
				SET_TOK_TYPE(TOK_LESS)
		case '&':
			if (next_char == '&') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_AND)
			}
			else
				PANIC(scanner, ERROR_UNEXPECTED_TOK)
		case '|':
			if (next_char == '|') {
				scanner_read_char(scanner);
				SET_TOK_TYPE(TOK_OR)
			}
			else
				PANIC(scanner, ERROR_UNEXPECTED_TOK)
		case '{':
			SET_TOK_TYPE(TOK_OPEN_BRACE);
		case '}':
			SET_TOK_TYPE(TOK_CLOSE_BRACE);
		case '(':
			SET_TOK_TYPE(TOK_OPEN_PAREN);
		case ')':
			SET_TOK_TYPE(TOK_CLOSE_PAREN);
		case '[':
			SET_TOK_TYPE(TOK_OPEN_BRACKET);
		case ']':
			SET_TOK_TYPE(TOK_CLOSE_BRACKET);
		case ',':
			SET_TOK_TYPE(TOK_COMMA);
		case 0:
			SET_TOK_TYPE(TOK_EOF);
		default:
			PANIC(scanner, ERROR_UNEXPECTED_TOK);
		}
		scanner_read_char(scanner);
	}
	return 1;
}

void init_multi_scanner(multi_scanner_t* scanner, const char* source, const uint32_t length) {
	init_scanner(&scanner->scanners[0], source, length, 1);
	scanner->visited_files = 0;
	scanner->current_scanner = 0;
	scanner->last_err = ERROR_NONE;
}

void free_multi_scanner(multi_scanner_t* scanner) {
	for (uint_fast8_t i = 0; i < scanner->visited_files; i++) {
		free(scanner->file_paths[i]);
		free(scanner->sources[i]);
	}
}

const int multi_scanner_visit(multi_scanner_t* scanner, const char* file) {
	uint64_t id = hash(file);
	for (uint_fast8_t i = 0; i < scanner->visited_files; i++)
		if (id == scanner->visited_hashes[i])
			return 1;
	if (scanner->visited_files == 64 || scanner->current_scanner == 31)
		return 0;
	PANIC_ON_FAIL(scanner->sources[scanner->visited_files] = file_read_source(file), scanner, ERROR_CANNOT_OPEN_FILE);
	ESCAPE_ON_FAIL(scanner->file_paths[scanner->visited_files] = malloc(strlen(file) * sizeof(char)));
	strcpy(scanner->file_paths[scanner->visited_files], file);
	scanner->visited_hashes[scanner->visited_files] = id;

	init_scanner(&scanner->scanners[++scanner->current_scanner], scanner->sources[scanner->visited_files], strlen(scanner->sources[scanner->visited_files]), 1);
	return 1;
}

const int multi_scanner_scan_tok(multi_scanner_t* scanner) {
	PANIC_ON_FAIL(scanner_scan_tok(&scanner->scanners[scanner->current_scanner]), scanner, scanner->scanners[scanner->current_scanner].last_err);
	scanner->last_tok = scanner->scanners[scanner->current_scanner].last_tok;
	if (scanner->last_tok.type == TOK_EOF && scanner->current_scanner) {
		--scanner->current_scanner;
		ESCAPE_ON_FAIL(multi_scanner_scan_tok(scanner));
	}
	return 1;
}