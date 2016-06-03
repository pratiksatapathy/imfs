#include <stdio.h>

int main( int argc, char *argv[] )  {

  
   FILE *fp;
   char buff[255];

 /*if( argc == 1 ) {

fp = fopen("./mnt/r", "w+");
   fprintf(fp, "Thisistestingforfprintf...\n");
   fputs("Thisistestingforfputs...\n", fp);
   fclose(fp);
}else{

   fp = fopen("./mnt/r", "r");
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );

   fclose(fp);
}
*/	

   fp = fopen("./mnt/r", "w+");
   fprintf(fp, "Thisistestingforfprintf...\n");
   fputs("Thisistestingforfputs...\n", fp);
   fclose(fp);

   fp = fopen("./mnt/r", "r");
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );
   
   fp = fopen("./mnt/r", "w+");
   fprintf(fp, "Somethingelse\n");
   fclose(fp);
   
   fp = fopen("./mnt/r", "r");
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );

   fp = fopen("./mnt/r", "w+");
   fprintf(fp, "anotherone\n");
   fclose(fp);

   fp = fopen("./mnt/r", "r");
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );
   
   fp = fopen("./mnt/r", "w+");
   fprintf(fp, "Nowthisagain\n");
   fclose(fp);

   fp = fopen("./mnt/r", "r");
   fscanf(fp, "%s", buff);
   printf("1 : %s\n", buff );

}
