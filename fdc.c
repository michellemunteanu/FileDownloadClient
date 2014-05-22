#include <stdio.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

void menu()
{
    printf("L) List files\n");
    printf("D) Download a file\n");
    printf("q) Quit\n");
}

int main(int argc, char **argv)
{
    struct sockaddr_in sa; //struct that holds info about the socket
    int sockfd;
    FILE *d; //the file pointer for writing to a file (downloading)
    char *fname;
    int filesize;
    int rsize; //received size to keep track of how much has been received

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s ip path\n", argv[0]);
	fprintf(stderr, "Example: %s 205.251.242.54 /\n", argv[0]);
	exit(1);
    }

    sa.sin_family = AF_INET; //creates an internet socket
    sa.sin_port = htons(1234); //assigns 1234 as port num
    inet_pton(AF_INET, argv[1], &sa.sin_addr);
    
    //create the socket and assigns it a num (int sockfd):
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        fprintf(stderr, "Can't create socket\n");
	exit(3);
    }
    int res = connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)); //error checking
    if (res == -1) {
        fprintf(stderr, "Can't connect\n");
	exit(2);
    }

    char buf[1000];
    size_t size = recv(sockfd,buf,1000,0);
    if ((size > 0) && (buf[0] != '+'))
    {
	fprintf(stderr, "Didn't receive +OK from initial message\n");
	exit(5);
    }
  
    while(1)
    {
        menu();

        char *choice = readline("Your choice: ");
        switch(choice[0])
        {
            case 'L':
            case 'l': //a list of available files is shown
	    printf("\n");
	    sprintf(buf, "LIST\n");
	    send(sockfd, buf, strlen(buf), 0);
	    int bufcount=0;
	    do //will need to fix this later to skip over .
	    {
		size = recv(sockfd,buf,1000,0);
		//printf("Got buffer %d of size %zu\n",bufcount,size);
                //bufcount++;
	        if(buf[0] == '+')
                {
		    fwrite((buf+4), (size-4), 1, stdout);
                }
	        else if(buf[size-2] == '.')
		{
		   fwrite(buf, (size-2), 1, stdout); //don't print the '.'
		    printf("\n");
		}
		else
		{
		    fwrite(buf, size, 1, stdout);
		}
	    } while(buf[size-3] != '\n' && buf[size-2] != '.' && buf[size-1] == '\n');

	    break;

	    case 'D':
            case 'd': //prompt for file to download, download that file, save it to user's folder
	    fname = readline("File to download: ");
	    d = fopen(fname, "w");
            //bufcount=0;
	    rsize=0;
	    filesize=0;

	    printf("Downloading %s...\n", fname);

	    sprintf(buf, "SIZE %s\n", fname);
            send(sockfd, buf, strlen(buf), 0);
            size = recv(sockfd,buf,1000,0); //recv to get +OK and size
	    if(size > 0 && buf[0] != '+')
            {
                fprintf(stderr, "Did not receive +OK from size\n");
                exit(6);
            }

		//fwrite(buf, 1, size, stdout);
                
	    sscanf(buf, "%*s %d", &filesize); //get total file size of file to be downloaded
	    //printf("Filesize %d\n",filesize); //***why is this always zero?
	    
	    sprintf(buf, "GET %s\n",fname);
            send(sockfd, buf, strlen(buf), 0);
            size = recv(sockfd,buf,1000,0);
	    if(size > 0 && buf[0] != '+')
            {
                fprintf(stderr, "Did not receive +OK from get\n");
                exit(7);
            }

            do
            {
                size = recv(sockfd,buf,1000,0);
		fwrite(buf, 1, size, d);
		rsize+=size;
	    } while(rsize < filesize);

	    printf("Download of %s complete.\n\n", fname);

	    break;;

	    case 'Q':
            case 'q': //quit
	    sprintf(buf, "QUIT\n");
	    send(sockfd, buf, strlen(buf),0);
	    size = recv(sockfd,buf,1000,0);
	    if(size > 0 && buf[0] != '+')
	    {
		fprintf(stderr, "Did not receive +OK from quit\n");
		exit(5);
	    } 
	    close(sockfd); 	
	    exit(5);
		break;

	     default:
                printf("Invalid choice.\n");
               // Handle everything else
        }
    }
}
