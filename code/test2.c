#include <stdio.h>

int main( int argc, char *argv[] )  {

  
   FILE *fp;
   char buff[255];

   for(int i=0;i<61;i++){
	   
	   char filename[20];
	   char content[255];
	   sprintf(filename, "./mnt/re%d", i);
	   sprintf(content, "contentsfor%d\n", i);
	   printf("%s\n",filename);
	   fp = fopen(filename, "w+");
	   if( fp == NULL){
		   printf("Only %d files were created.",(i+1));
		   
		   return -1;
	   }
	   fprintf(fp, "contentsfor%d\n", i);
	   fclose(fp);
	}
	printf("All the files were created.");
}
