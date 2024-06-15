#ifndef CL_PARSE_H
#define CL_PARSE_H

#include "qtypes.h"
#include "cvar.h"

extern cvar_t qport;

void CL_Parse_Init(void);
void CL_Parse_Shutdown(void);
void CL_ParseServerMessage(void);
void CL_Parse_DumpPacket(void);
void CL_Parse_ErrorCleanUp(void);
void QW_CL_StartUpload(unsigned char *data, int size);
void CL_KeepaliveMessage(qbool readmessages); // call this during loading of large content

#define VM_STRINGTEMP_LENGTH_16384 MAX_INPUTLINE_16384

typedef struct {
	int num_tokens;
	ccs *tokens_za[VM_STRINGTEMP_LENGTH_16384 / 2];
	int tokens_startpos[VM_STRINGTEMP_LENGTH_16384 / 2];
	int tokens_endpos[VM_STRINGTEMP_LENGTH_16384 / 2];
	char tokenize_string[VM_STRINGTEMP_LENGTH_16384];
} tokenize_console_s;

void Tokenize_Console_16384_FreeContents (tokenize_console_s *tcx);
int Tokenize_Console_16384_Za_Return_Argc (tokenize_console_s *tcx, ccs *s);


#endif // ! CL_PARSE_H
