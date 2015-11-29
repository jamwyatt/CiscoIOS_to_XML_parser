#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE		2048

static char tabs[] = { '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t', '\t'};



int parseLine(char **ptrs,int numPtrs, char *line) {
        memset(ptrs,0,numPtrs*sizeof(char*));           // zero out the resulting list of pointers for safey
        int count=0;

        if (line) {
                char *s = line;

                while ((*s != '\0')&&(count<numPtrs)) {
			// remove leading whitespace
			while(*s == ' ' || *s =='\t')
				s++;

                        ptrs[count++] = s;	// record the start

			// Consume the word
                        while ((*s != '\0')&&(*s != ' ')) 
                                s++;
			// Zap this space as a terminator
                        if (*s == ' ')
                                *s++ = '\0';
                	}
        	}
        return(count);
	}


char * isolateFirstWord(char **line) {
	char *word = NULL;
        if (line && *line) {
                char *s = *line;

		// remove leading whitespace
		while((*s != '\0') && ((*s == ' ') || (*s =='\t')))
			s++;

		word = s;	// record the start

		// Consume the word
		while ((*s != '\0')&&(*s != ' ')) 
			s++;
		// Zap this space as a terminator
		if (*s == ' ')
			*s++ = '\0';
		*line = s;
		}
	return(word);
	}

int getLineIndentCount(char *s) {
	int count = 0;
	while(s && *s != '\0' && *s == ' ') {
		count++;
		s++;
		}
	return(count);
	}

char *escapeXML(char *s, char *buffer, int size) {
        memset(buffer,0,size);
	int index = 0;
	while(s && *s != '\0' && index < size) {
		switch(*s) {
			case '\"':
				strcat(&buffer[index],"&quot;");
				index += 6;
				break;
			case '\'':
				strcat(&buffer[index],"&apos;");
				index += 6;
				break;
			case '>':
				strcat(&buffer[index],"&gt;");
				index += 4;
				break;
			case '<':
				strcat(&buffer[index],"&lt;");
				index += 4;
				break;
			case '&':
				strcat(&buffer[index],"&amp;");
				index += 5;
				break;
			default:
				buffer[index++] = *s;
			}
		s++;
		}
	return(buffer);
	}

int needsSpecialHandling(char *firstWord) {
	return( (strcmp(firstWord, "banner") == 0) || (strcmp(firstWord, "!") == 0) ||
		(strcmp(firstWord, "description") == 0) || (strcmp(firstWord, "label") == 0));
	}

// Should not need to read into "next" lines
void specialCases(FILE *fp, char *firstWord, char *line, int *closingHandled) {
	*closingHandled=0;  /// do not skip closing parts of this tag in caller
	// The start of the element tag is already written ... just need the rest
	// Leave the input pointer aimed at next proper line to be used.
	if(strcmp(firstWord, "banner") == 0) {
		char *bannerType = isolateFirstWord(&line);	// parse only the first parm
		printf(" type=\"%s\">",bannerType);
		// remove leading whitespace
		char *s = line;
		while((*s != '\0') && ((*s == ' ') || (*s =='\t')))
			s++;
		char endChar = *s++;		// the end of the banner is marked with this character
		printf("%c%s",endChar,s);
		char buffer[MAXLINE];
		strcpy(buffer,s);
		int len = strlen(buffer);
		if(buffer[(len>1)?len -1:0] != endChar)
			printf("\n");	// the original new line was part of the banner, just need to preserve it
		while((buffer[(len>1)?len-1:0] != endChar) && (fgets(buffer, sizeof(buffer), fp))) {
			// Clean the line of a trailing newline
			int len = strlen(buffer);
			if(buffer[len-1] == '\n')
				buffer[len-1] = '\0';
			len = strlen(buffer);
			if(buffer[len-1] == endChar)
				printf("%s",buffer);	// no newline after the end char is found on a line
			else
				printf("%s\n",buffer);
			}
		printf("</banner>\n",bannerType);
		*closingHandled = 1;		// Tells caller that the closing is handled already
		}
	else if((strcmp(firstWord, "description") == 0) || (strcmp(firstWord, "label") == 0)) {
		char pBuff[MAXLINE];
		printf(" string=\"%s\"",escapeXML(line, pBuff, sizeof(pBuff)));
		}
	else if(strcmp(firstWord, "!") == 0) {
		char pBuff[MAXLINE];
		printf("<comment text=\"%s\"",escapeXML(line, pBuff, sizeof(pBuff)));
		}
	}

char *parseIOSInput(FILE *fp, char *preRead, int myLevel) {

	char buffer[MAXLINE];
	char prevTag[MAXLINE];
	char *parms[1024];

        memset(buffer,0,sizeof(buffer));
        memset(prevTag,0,sizeof(prevTag));
        memset(parms,0,sizeof(parms));

	int prevLevel = 0;
	int lineCount = 0;
	int closingHandled = 0;
	while(preRead || fgets(buffer, sizeof(buffer), fp)) {
		char *line=buffer;
		if(preRead)
			line=preRead;	// con only read a line once and if it is passed in, please use

		// Clean the line of a trailing newline
		int len = strlen(line);
		if(line[len-1] == '\n')
			line[len-1] = '\0';

		// Count preceding spaces
                int curLevel = getLineIndentCount(line);

		if(lineCount) {		// for anything after the first line (might have preceeding content)
			if(curLevel > prevLevel) {
				printf(">\n");	// increase indent with sub element, close outer level for current
				strcpy(buffer,parseIOSInput(fp, line, curLevel));	// parse the lower level(s)
				line = buffer;	// next line to parse
				// Update level count from the preRead line
                		curLevel = getLineIndentCount(line);			// can only be my level or shallower
				if(curLevel > myLevel) {
					// IOS Hack!
					// special case where we came back from a lower level only to find 
					// the next tag is lower than us, but not lower than the previous (odd case)
					strcpy(buffer,parseIOSInput(fp, line, curLevel));	// parse the lower level(s)
					line = buffer;	// next line to parse
					// Update level count from the preRead line
                			curLevel = getLineIndentCount(line);			// can only be my level or shallower
					}
				printf("%*.*s</%s>\n",myLevel,myLevel,tabs,prevTag);	// close previous tag
				if(curLevel != myLevel)
					return(line);	// upper level content ... 
				preRead = line;
				}
			else if(curLevel == prevLevel) {
				if(!closingHandled)
					printf("/>\n");	// close out previous tag on same level
				}
			else {  // This would be a case where the new level is shallower than my level
				printf("/>\n");	// close out previous tag on same level
				return(line);
				}
			}
		char *firstWord = isolateFirstWord(&line);	// parse only the first parm
		strcpy(prevTag,firstWord);
		if(strcmp(firstWord, "!") == 0)
			printf("%*.*s",curLevel,curLevel,tabs);		// leave for special treatment
		else {
			if(*firstWord == '\0')
				printf("%*.*s<blank indent=\"%d\"",curLevel,curLevel,tabs,curLevel);
			else
				printf("%*.*s<%s indent=\"%d\"",curLevel,curLevel,tabs,firstWord,curLevel);
			}
		if(needsSpecialHandling(firstWord)) {
			specialCases(fp, firstWord, line, &closingHandled);
			}
		else {
			int parmCount = parseLine(parms,sizeof(parms)/sizeof(char *),line);
			int x;
			char pBuff[MAXLINE];
			for(x=0;x<parmCount;x++) {
				// Beautification of some of the more interesting commands
				if(((strcmp(parms[x], "remark") == 0) && (strcmp(firstWord,"access-list") == 0)) ||
				   ((strcmp(parms[x], "command") == 0) && (strcmp(firstWord,"action") == 0))) {
					printf(" %s=\"", firstWord);
					for(x++;x<parmCount;x++) {
						printf("%s",escapeXML(parms[x], pBuff, sizeof(pBuff)));
						if(x+1 < parmCount)
							printf(" ");	// missing spaces for all but last parm
						}
					printf("\"");
					}
				else
					printf(" parm%d=\"%s\"",x,escapeXML(parms[x], pBuff, sizeof(pBuff)));
				}
			closingHandled = 0;
			}
		prevLevel = curLevel;
		lineCount++;
		preRead = NULL;
		}
	if(prevLevel != myLevel)
		printf("%*.*s</%s>\n",myLevel,myLevel,tabs,prevTag);	// close previous tag
	else {
		if(!closingHandled)
			printf("/>\n");	// close out previous tag on same level
		}
		
	return(NULL);	// the end of processing
	}


int main(int argc, char *argv[]) {
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<ios>\n");
	parseIOSInput(stdin, NULL, 0);
	printf("</ios>\n\n");
	exit(0);
	}
