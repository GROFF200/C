#include <stdio.h>
#include <string.h>
#include "common.hxx"


///////////////////////////
#define TDEBUG(p1,p2)

escstring::escstring()
 {
 pesc = (char *)NULL;
 }

escstring::~escstring()
 {
 TDEBUG("string::~string",pesc);
 if(pesc)
    delete pesc;
 }


int escstring::ahtoi(char c)
 {
 int    i;

 i = (int)(c - '0');
 if(i > 9)
    i -= 7;

 return i;
 }


char escstring::itoha(int i)
{
char    c;

c = (char)(i + '0');
if(c > '9')
    c += 7;

return c;
}


unsigned char *escstring::str_unescape(char *s,int *size)
 {
 unsigned char   c,*cp = (unsigned char *)s;

 while((c = *cp))
    {
    if(c == '+')
        *cp = ' ';

    if(c == '%')
        {
        c = (unsigned char)((ahtoi(cp[1]) << 4) | ahtoi(cp[2]));
        *cp = c;
        memmove(&cp[1],&cp[3],strlen((const char *)&cp[3]) + 1);
        }
    cp++;
    }

 *size = (int)(cp - (unsigned char *)s);
 return (unsigned char *)s;
 }


char *escstring::str_escape(unsigned char *s,int len,int esc_flag)
 {
 unsigned char   c,*ps;
 int    newlen,pass,di,savelen;

 if(pesc)
    delete pesc;

 if(len == -1)
    len = strlen((const char *)s);
    
 newlen = savelen = len;
 for(pass = 0;pass < 2;pass++)
    {
    ps = s;
    if(pass == 1)
        {
        pesc = new char[newlen + 1];
        di = 0;
        }

    while(len--)
        {
		c = *ps++;
        if((c == (unsigned char)' ') && (esc_flag == ESC_NORMAL))
            {
            if(pass == 0)
                continue;
            else
                {
                pesc[di++] = '+';
                continue;
                }
            }
        if((c >= (unsigned char)'0') && (c <= (unsigned char)'9'))
            {
            if(pass == 0)
                continue;
            else
                {
                pesc[di++] = (char)c;
                continue;
                }
            }
        if((c >= (unsigned char)'A') && (c <= (unsigned char)'Z'))
            {
            if(pass == 0)
                continue;
            else
                {
                pesc[di++] = (char)c;
                continue;
                }
            }

        if(pass == 1)
            {
            pesc[di++] = '%';
            pesc[di++] = itoha(c >> 4);
            pesc[di++] = itoha(c & 0x0f);
            }

        //newlen += 2;
        newlen++;
        }
	
	len = savelen;
    }

 pesc[di] = '\0';
TDEBUG(pesc,strlen(pesc));
 return pesc;
 }


