/* here is the code start */

#include <stdio.h>
#include <stdlib.h>   
#include <string.h>
#include <stdint.h>

//#define YYERRORSYMBOL
#include "std_types.h"

typedef struct context_s{
	u8 * tokens;
	u8 ** tokens_info;
	u8 * string_start;
	u8 * string_end;
	u32  line_number;
} Context;

typedef struct parser_s{
	unsigned char * type_name;
	unsigned char * member_name[50];
	int error;
	int num_members;
} ParserState;


#include "/opt/sqlite3/sqlite3.h"
#include "gram_xmi.h"
#include "xmi_lex.c"
#include "gram_xmi.c"

 

/* now that we have a correct parse walk the parse to generate code */
static int semantic_actions(Context * c, unsigned char * output)
{
	/* cache chars */
	/* need information on struct members */
	unsigned char * start_of_string_a[128]={0};
	unsigned char ** start_of_string = start_of_string_a;
	unsigned char * type_name;
	unsigned char * member_name;
	unsigned char *token_info = *(c->tokens_info);
	unsigned char stringGrabbed=0;
	int numMembers=0, i;
	
	while ((*token_info) != 0){
		if( ((*token_info) < 128) && (!stringGrabbed)) {
			*start_of_string = token_info;
			stringGrabbed=1;
		} else if( (*token_info) ==129 ) {
			token_info++;
			if( (*token_info) ==130 ) {
				token_info++;
				if( (*token_info) ==0 ) {
					token_info-=2;
					*token_info=0;
					type_name = *start_of_string;
					printf("#define type_name = %s\n", type_name );
				} else {
					token_info--;
				}
			} else {
				token_info--;
			}
		}
		if( (*token_info) >127 ) {
			stringGrabbed=0;
		}
		token_info++;	
	}
	stringGrabbed=0;
	token_info = *(c->tokens_info);
	output+=sprintf((char*)output, "const uint8 * const %s_memberNames[] =\n",type_name);
	while ((*token_info) != 0){
		if( ((*token_info) < 128) && (!stringGrabbed)) {
			*start_of_string = token_info;
			
			stringGrabbed=1;
		} else if( (*token_info) ==129 ) {
			token_info++;
			if( (*token_info) ==130 ) {
				token_info--;
				*token_info=0;
				numMembers++;
				output+=sprintf((char*)output, "\"%s\",\n", (*start_of_string) );
				start_of_string++;
			} else {
				token_info--;
			}
		}
		if( (*token_info) >127 ) {
			stringGrabbed=0;
		}
		token_info++;	
	}
	output+=sprintf((char*)output, ";\n");
	
	output+=sprintf((char*)output, "#define %s_numMembers  = %d\n",type_name, numMembers );
	
	/* at this point we have number of members, names, and type name */
	start_of_string = start_of_string_a;
	for(i=numMembers; i!=0;i--){
		member_name = *start_of_string++;
		output+=sprintf((char*)output, "member_name = %s\n", member_name);
	}
	start_of_string = start_of_string_a;
	
	
	
	
	
	

	return 0;
}

/* now that we have a correct parse walk the parse to generate code */
static int semantic_actions_Wstate(ParserState * parser_state, unsigned char * output)
{
	/* cache chars */
	/* need information on struct members */
	unsigned char * mem_name;
	unsigned char * tmp_p;
	unsigned char custom_type[32];
	int x;
	unsigned char ptrCustomType = 0;
	
	printf("type name = %s\n", parser_state->type_name);
	for(x=0;x<parser_state->num_members;x++) {
		printf("member_name = %s\n", parser_state->member_name[x]);
	}
	
	/* output number of members */
	output+=sprintf((char*)output, "#define %s_numMembers  = %d\n",parser_state->type_name, parser_state->num_members);
	
	/* output names */
	output+=sprintf((char*)output, "#define %s_memberNames ", parser_state->type_name);
	
	output+=sprintf((char*)output, "const uint8 * const %s_memberNames[] ={\\\n",parser_state->type_name);
	
	for(x=0;x<parser_state->num_members;x++) {
		output+=sprintf((char*)output, "\"%s\",\\\n",parser_state->member_name[x]);
	}
	output+=sprintf((char*)output, "};\n");
	
	/* output toJson function */
	output+=sprintf((char*)output, "#define %s_toJSON_Func \\\n", parser_state->type_name);
	
	output+=sprintf((char*)output, 
	"uint8_t * %s_toJSON(uint8_t * b, const %s * t) \\\n"
	, parser_state->type_name, parser_state->type_name);
	output+=sprintf((char*)output, "{ \\\n");
	output+=sprintf((char*)output, "b = (uint8_t *)stpcpy((char *)b, \"{\"); \\\n");
	/* print out members to JSON */
	for(x=0;x<parser_state->num_members;x++) {
		mem_name = parser_state->member_name[x];
		/* check if this is a custom type */
		if ( (*mem_name == 'C') && (*(mem_name+1) == 'T') ) {
			/* this has the custom type prefix */
			/*check if this is a "number of" member for a pointer type
			  that is immediately following */
			if (*(mem_name+2) == 'N') {
				/* this is a number of type, use for loop generation */
				ptrCustomType = 1;
				
			} else if ( *(mem_name+2) == '_' ) {
				/* type name coming, this is a custom type */
				tmp_p = mem_name + 4;
				mem_name = mem_name + 3;
				/* keep going while lower or upper case letters */
				while ( ((*tmp_p) >= 97 && (*tmp_p) <= 122) || ((*tmp_p) >= 65 && (*tmp_p) <= 90) ) {
					tmp_p++;
				}
				/* copy off custom type */
				strncpy( (char *)custom_type, (const char *)mem_name, (tmp_p - mem_name) );
				/* null terminate */
				custom_type[(tmp_p - mem_name)]=0;
				if ( ptrCustomType ) {
					/* generate for loop for printing out custom type array */
					output+=sprintf((char*)output, 
					"if(t->%s > 0) {\\\n" //1
					"\tconst %s  * tmp_%d = t->%s;\\\n" // 2 3 4
					"\tb = (uint8_t *)stpcpy((char *)b, \",\"); \\\n"
					"\tb+=sprintf((char *)b, \"\\\"%s\\\":\"); \\\n" // 5
					"\tb = (uint8_t *)stpcpy((char *)b, \"[\"); \\\n"
					"\tfor(int i=t->%s; i>0; i--) { \\\n" // 6
					"\t\tb = %s_toJSON(b, tmp_%d); \\\n" //7 8
					"\t\tif (i > 1) { \\\n"
					"\t\t\tb = (uint8_t *)stpcpy((char *)b, \",\"); \\\n"
					"\t\t\ttmp_%d++; \\\n" // 9
					"\t\t} \\\n"
					"\t} \\\n"
					"\tb = (uint8_t *)stpcpy((char *)b, \"]\"); \\\n"
					"} \\\n"
					,
					parser_state->member_name[x-1], // 1
					custom_type, // 2
					x, // 3
					parser_state->member_name[x], // 4
					parser_state->member_name[x], // 5
					parser_state->member_name[x-1], // 6
					custom_type, // 7
					x, // 7
					x // 8
					);
					ptrCustomType = 0;
				} else {
					if ( x > 0 ) {
						output+=sprintf((char*)output, "\tb = (uint8_t *)stpcpy((char *)b, \",\"); \\\n");
					}
					output+=sprintf((char*)output,
					"\tb+=sprintf((char *)b, \"\\\"%s\\\":\"); \\\n" // 5
					"\tb = %s_toJSON(b, &t->%s); \\\n"
					,parser_state->member_name[x],
					custom_type,
					parser_state->member_name[x]
					);
				}
				/* finished with special type, next */
				continue;
			}
		}
		/* output comma */
		if ( x > 0 ) {
			output+=sprintf((char*)output, 
			"\tif ( NP_C(t->%s) || ((t->%s)!=0) ) { \\\n" /* if */
			"\t\tb = (uint8_t *)stpcpy((char *)b, \",\"); \\\n" /* comma */,
			mem_name,
			mem_name
			);
			/* output member */
			output+=sprintf((char*)output,
			"\t\tb+=sprintf((char *)b, \"\\\"%s\\\":\"); \\\n" /* name */
			"\t\tb+=sprintf((char *)b, FMT(t->%s), t->%s); \\\n" /* value */
			"\t}\\\n" /* end */
			,mem_name,
			mem_name,
			mem_name
			);
		} else {
			/* output member */
			output+=sprintf((char*)output,
			"\tif ( NP_C(t->%s) || ((t->%s)!=0) ) { \\\n" /* if */
			"\t\tb+=sprintf((char *)b, \"\\\"%s\\\":\"); \\\n" /* name */
			"\t\tb+=sprintf((char *)b, FMT(t->%s), t->%s); \\\n" /* value */
			"\t}\\\n" /* end */
			,mem_name,
			mem_name,
			mem_name,
			mem_name,
			mem_name
			);
		}
	}
	output+=sprintf((char*)output, "b = (uint8_t *)stpcpy((char *)b, \"}\"); \\\n");
	output+=sprintf((char*)output, "return b; \\\n");
	output+=sprintf((char*)output, "} \n");
	
	
	// while ((*token_info) != 0){
		// if( ((*token_info) < 128) && (!stringGrabbed)) {
			// *start_of_string = token_info;
			// stringGrabbed=1;
		// } else if( (*token_info) ==129 ) {
			// token_info++;
			// if( (*token_info) ==130 ) {
				// token_info++;
				// if( (*token_info) ==0 ) {
					// token_info-=2;
					// *token_info=0;
					// type_name = *start_of_string;
					// printf("#define type_name = %s\n", type_name );
				// } else {
					// token_info--;
				// }
			// } else {
				// token_info--;
			// }
		// }
		// if( (*token_info) >127 ) {
			// stringGrabbed=0;
		// }
		// token_info++;	
	// }
	// stringGrabbed=0;
	// token_info = *(c->tokens_info);
	// output+=sprintf((char*)output, "const uint8 * const %s_memberNames[] =\n",type_name);
	// while ((*token_info) != 0){
		// if( ((*token_info) < 128) && (!stringGrabbed)) {
			// *start_of_string = token_info;
			
			// stringGrabbed=1;
		// } else if( (*token_info) ==129 ) {
			// token_info++;
			// if( (*token_info) ==130 ) {
				// token_info--;
				// *token_info=0;
				// numMembers++;
				// output+=sprintf((char*)output, "\"%s\",\n", (*start_of_string) );
				// start_of_string++;
			// } else {
				// token_info--;
			// }
		// }
		// if( (*token_info) >127 ) {
			// stringGrabbed=0;
		// }
		// token_info++;	
	// }
	// output+=sprintf((char*)output, ";\n");
	
	// output+=sprintf((char*)output, "#define %s_numMembers  = %d\n",type_name, numMembers );
	
	// /* at this point we have number of members, names, and type name */
	// start_of_string = start_of_string_a;
	// for(i=numMembers; i!=0;i--){
		// member_name = *start_of_string++;
		// output+=sprintf((char*)output, "member_name = %s\n", member_name);
	// }
	// start_of_string = start_of_string_a;

	return 0;
}


                                     //
int main()                           //
{                                    //
	const unsigned char * data;
	const unsigned char *data3 = "this is s come garbage 321 typedef struct BibleStudies {"
	"int numStudents;"; /* IMMUTABLE */
    /*"ChurchGuest CT_ChurchGuest_guest;"
    "const uint8_t * studyDescription;"
    "const uint8_t * studyPrayer;} BibleStudy; ";*/
	//unsigned char *lex_stack[100];
	unsigned char output_string[4096] = {0};
	unsigned char token_array[256] = {0};
	unsigned char tokeninfo_array[2048] = {0};
	unsigned char *lex_stack = tokeninfo_array;
	unsigned char * output = output_string;
	Context context = {0};
	void *pParser;
	int i=20,x, tmp_token;
	ParserState parser_state = {0};
	FILE * pFile, * outputFile;
	size_t lSize;
	unsigned char * buffer;
	size_t result;
	
	context.tokens = token_array;
    context.tokens_info = &lex_stack;
	
	
	pFile = fopen ( "test_types.h" , "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
	
	outputFile = fopen ( "type_macros.h" , "w" );
	if (outputFile==NULL) {fputs ("File error",stderr); exit (1);}
	
	// obtain file size:
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	// allocate memory to contain the whole file:
	buffer = (unsigned char*) malloc (sizeof(char)*lSize+1);
	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
	data = buffer;
	// copy the file into the buffer:
	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
	
	//buffer[lSize]=0;
	
	pParser = ParseAlloc( malloc, &parser_state );
	
	ParseTrace(stdout, "debug:");

	printf("starting parse\n");
	parser_state.error = -1;
	do {
		tmp_token = lex(&data, &context);

		Parse(pParser, tmp_token, context.string_start);

		/*RA_Parse(&parser_context, 
		 tmp_token,
		 0,
		 &parser_state);*/
		//printf("parser_state.error = %d\n", parser_state.error);
		
		/* if there was a parse failure reset stack */
		if (parser_state.error == 1) {
			printf("error actions\n");
			*(context.tokens_info) = tokeninfo_array;
			parser_state.error = -1;
			
		} else if (parser_state.error == 0) {
			/* parser has found a type */
			/* append 0 */
			(*(*(context.tokens_info))) = 0;
			/* reset pointer to start of array */
			*(context.tokens_info) = tokeninfo_array;
			//semantic_actions(&context, &output_string);
			semantic_actions_Wstate(&parser_state, output);
			fwrite (output_string , sizeof(char), strlen((const char *)output_string), outputFile);
			
			printf("type name = %s\n", parser_state.type_name);
			for(x=0;x<parser_state.num_members;x++) {
				printf("member_name = %s\n", parser_state.member_name[x]);
			}
			
			parser_state.num_members = 0;
			output = output_string;
			
			if (tmp_token==0) {
				break;
			} else {
				parser_state.error = -1;
			}
		} 
		
	} while (tmp_token != 0);
	
	

	
	
	
	fflush (outputFile); 
	
	
	fclose (outputFile);
	fclose (pFile);
	free (buffer);
	//printf("BEFORE FINAL COMMIT\n");
	//Parse(pParser, 0, 0);
	ParseFree(pParser, free );
	printf("lext type:%d\n", i);
	
	
	
    return 0;                        //
}   



