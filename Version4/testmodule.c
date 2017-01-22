#include "ctype.h"
#include <stdio.h>

int isWordChar(int c);

int
isWordChar(int c)
{
  return !isalnum(c) && c != EOF;
}
