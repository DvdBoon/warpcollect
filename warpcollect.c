#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

const char amigaversion[] = "$VER: Warpcollect 0.5 (30.10.2026)";

char *progname;

/* The linker to drive.  Build the variant that uses the ld in
   /gg/ppc-warpos/bin (see the readme there) with
   -DLD_PATH='"/gg/ppc-warpos/bin/ld"'. */
#ifndef LD_PATH
#if defined (_LINUX_)
#define LD_PATH "/gg/bin/ppc-morphos-ld"
#else
#define LD_PATH "/gg/bin/ld"
#endif
#endif

#if defined (_LINUX_)
#include <unistd.h>

/* Look for an executable called name in $PATH.  Returns a malloc'd full
   path, or NULL when the tool is not installed. */
static char *find_in_path(const char *name)
{
	const char *path = getenv("PATH");
	size_t namelen = strlen(name);
	char *full;

	if (path == NULL || *path == '\0')
		return NULL;

	full = malloc(strlen(path) + namelen + 3);
	if (full == NULL)
		return NULL;

	for (;;) {
		const char *end = strchr(path, ':');
		size_t dirlen = (end == NULL) ? strlen(path) : (size_t)(end - path);

		if (dirlen == 0) {	/* an empty entry means the current directory */
			full[0] = '.';
			dirlen = 1;
		} else
			memcpy(full, path, dirlen);

		full[dirlen] = '/';
		memcpy(full + dirlen + 1, name, namelen + 1);

		if (access(full, X_OK) == 0)
			return full;

		if (end == NULL)
			break;
		path = end + 1;
	}

	free(full);
	return NULL;
}

#else
/* From libnix */
static char *amigapath(const char *path)
{ static char *s1=NULL;
  char *s2,*s3;
  int c;

  if(s1!=NULL)
    free(s1);

  s1=strdup(path);
  if(s1==NULL)
  { errno=ENOMEM;
    return NULL; }
  
  s3=s2=s1; /* Replace multiple following '/' by single ones */
  do
  { if(*s2=='/')
      while(s2[1]=='/')
        s2++;
    *s3++=*s2;
  }while(*s2++);

  s3=s2=s1; /* Remove single dots '.' as directory names */
  c=1;
  do
  { while(c&&s2[0]=='.'&&(s2[1]=='/'||s2[1]=='\0'))
    { s2++;
      if(*s2=='/')
        s2++;
    }
    *s3++=*s2;
    c=0;
    if(*s2=='/')
      c=1;
  }while(*s2++);
  
  s3=s2=s1; /* Remove double dots '..' as directory names */
  c=1;
  do
  { if(c&&s2[0]=='.'&&s2[1]=='.')
    { if(s2[2]=='/')
        s2+=2; 
      else if(s2[2]=='\0')
      { *s3++='/';
        s2+=2; }
    }
    *s3++=*s2;
    c=0;
    if(*s2=='/')
      c=1;
  }while(*s2++);

  if(*s1=='/') /* Convert names beginning with '/' */
  { s3=s2=s1;
    s2++;
    if(*s2=='/'||*s2=='\0') /* The root directory */
      return "SYS:";
    while(*s2!='/'&&*s2!='\0')
      *s3++=*s2++;
    *s3++=':';
    if(*s2=='/')
      s2++;
    do
      *s3++=*s2;
    while(*s2++);
  }

  return s1;
}

static void copyfile(char *infile, char *outfile)
{
	FILE *in, *out;
	size_t len;
	int done = 0;
	char *buffer;

	in = fopen(infile, "rb");
	out = fopen(outfile, "wb");
	buffer = malloc(8192);

	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory\n", progname);
		exit(20);
	}

	if (in == NULL || out == NULL) {
		fprintf(stderr, "%s: Warning, Can't copy ELF file\n", progname);
		return;
	}

	while(done == 0) {
		len = fread(buffer, 1, 8192, in);
		if (len !=0)
			fwrite(buffer, 1, len, out);
		else
			done = 1;
	}

	free(buffer);
	fclose(out);
	fclose(in);
}

/* Is a command of this name available?  AmigaOS has no $PATH to scan,
   so probe the places this SDK keeps its tools. */
static int tool_exists(const char *name)
{
	static const char *dirs[] = { "", "GG:bin/", "C:" };
	char probe[256];
	FILE *f;
	unsigned int d;

	for(d = 0; d < sizeof(dirs) / sizeof(dirs[0]); d++) {
		if (strlen(dirs[d]) + strlen(name) >= sizeof(probe))
			continue;
		strcpy(probe, dirs[d]);
		strcat(probe, name);
		f = fopen(probe, "rb");
		if (f != NULL) {
			fclose(f);
			return 1;
		}
	}
	return 0;
}
#endif
int main(int argc, char **argv)
{
	int i, len = 0;
	char *temp, *outname;
	char *exec, *exec2, *exec3;
	char *env;
#if defined (_LINUX_)
    char *exec4;
#endif
    
	progname = argv[0];

	for(i = 1; i < argc; i++) {
		len = len + 1 + strlen(argv[i]);
	}

	exec = malloc(len + 256);
	exec2 = malloc(len + 256);
	exec3 = malloc(len + 256);
#if defined (_LINUX_)
    exec4 = malloc(len + 256);
#endif
    temp = malloc(len + 256);


	if (temp == NULL || exec == NULL || exec2 == NULL || exec3 == NULL) {
		fprintf(stderr, "%s: Out of memory\n", progname);
		return 20;
	}

	strcpy(temp, "a.out");
	strcpy(exec, LD_PATH);
    
	/* Get the output file name and copy the commandline.
	   Appending with sprintf(exec, "%s %s", exec, argv[i]) makes the
	   destination buffer alias a source argument.  That is undefined
	   behaviour; on glibc only the last argument survives, so the linker
	   command line collapsed to the endfile spec and system() tried to
	   execute end.o.  Append in place instead. */
	{
		size_t pos = strlen(exec);

		for(i = 1; i < argc; i++) {
			exec[pos++] = ' ';
			strcpy(exec + pos, argv[i]);
			pos += strlen(argv[i]);
			if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
				strcpy(temp, argv[i+1]);
		}
	}

	/* elf2exe2 doesn't support unixpaths :( */
#if defined (_LINUX_)
    outname = strdup(temp);
#else
	outname = amigapath(temp);
	if (outname == NULL) {
		fprintf(stderr, "%s: Out of memory\n", progname);
		return 20;
	}
#endif
	sprintf(temp, "%s.warpelf", outname);
#if defined (_LINUX_)
	sprintf(exec3, "/gg/bin/ppc-morphos-strip -R .gnu.attributes %s", outname);
    sprintf(exec4, "cp %s %s", outname, temp);
#else
    sprintf(exec3, "strip -R .gnu.attributes %s", outname);
#endif
	i = system(exec); /* Execute the linker */
	if (i != 0)
		return 20; /* errno says nothing about a child exit status */
	
    env = getenv("KEEPWARPELF");
	if (env != NULL && atoi(env) > 0) {
#if defined (_LINUX_)
        i = system(exec4);
        if (i != 0)
            return 20;
#else
		copyfile(outname, temp);
#endif
	}
	i = system(exec3); /* Execute the strip */
	if (i != 0)
		return 20;

#if defined (_LINUX_)
	/* Turn the linked ELF into a WarpOS executable with elf2exe3.  That is
	   an optional host tool: when it is not installed, leave the ELF. */
	{
		char *conv = find_in_path("elf2exe3");

		if (conv != NULL) {
			char *exec5 = malloc(strlen(conv) + 2 * strlen(outname) + 3);

			if (exec5 == NULL) {
				fprintf(stderr, "%s: Out of memory\n", progname);
				free(conv);
				return 20;
			}
			sprintf(exec5, "%s %s %s", conv, outname, outname);
			i = system(exec5); /* Execute elf2exe3 */
			free(exec5);
			free(conv);
			if (i != 0) {
				remove(outname); /* half-converted in place */
				return 20;
			}
		}
	}
#else
	/* Prefer elf2exe3, falling back to the older elf2exe2 both when
	   elf2exe3 is not installed and when it fails -- elf2exe3 validates
	   before it writes, so a failed run leaves the linked ELF intact and
	   the fallback gets a pristine input.  With no converter installed
	   at all, leave the ELF; one that ran and failed is an error. */
	{
		int installed = 0, converted = 0;

		if (tool_exists("elf2exe3")) {
			installed = 1;
			sprintf(exec2, "elf2exe3 %s %s", outname, outname);
			converted = (system(exec2) == 0);
		}

		if (!converted && tool_exists("elf2exe2")) {
			installed = 1;
			sprintf(exec2, "elf2exe2 %s %s", outname, outname);
			converted = (system(exec2) == 0);
		}

		if (installed && !converted) {
			remove(outname);
			remove(temp);
			return 20;
		}
	}
#endif
    free(exec);
    free(exec2);
    free(exec3);
#if defined (_LINUX_)
    free(exec4);
#endif
    free(temp);
	return 0;
}
