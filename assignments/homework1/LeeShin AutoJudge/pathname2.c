#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_a_line (char * line) {
        int i = 0;
        char c ;
        do {
                c = getc(stdin);
                if ( c != '\n' && c != EOF ) {
                        line[i] = c;
                        i++;
                }
        } while ( c != '\n' && c != EOF ) ;

        line[i] = '\0' ;

        if ( i == 0 && c == EOF )
                return 0;
        return 1;
}


void get_dir (char * dir, char * path) {

	int d = -1;
	int i = 0, j = 0;
	int dot_num = 0;
	while ( path[i] != '\0') {
		if ( path[i] == '.' ) {
			if ( path[i+1] == '/' ) {
				i += 3;
			}
			else if ( path[i+1] == '.' ) {
				for ( int k = i - 2 ; k >= 0 ; ) {
					if ( path[k] == '/' ) {
						j = k + 1;
						i += 3;
					}
				}
			}
			continue;
		}
		dir[j] = path[i];
		if ( dir[j] == '/' ) {
			d = j;
		}
		i++; j++;
	}
	dir[d + 1] = '\0';
}


	} while ( *c != '\0' ) ;
	file[strlen(path) - i] = '\0';
	return file;

}

int main() {
	char * path ;
	path = (char *) malloc(size
	read_a_line(path);

	char * dir = (char *) malloc (sizeofh) + 1));
	get_dir(dir, path);
	printf("%s\n", di

	free(path) ;
	free(dir);
}
