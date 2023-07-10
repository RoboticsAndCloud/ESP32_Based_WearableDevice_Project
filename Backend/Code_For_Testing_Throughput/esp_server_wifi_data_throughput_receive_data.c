#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define MAXBUFF 62000 // max image is 61440
#define PORT 3333 
#define SA struct sockaddr
#define END_JPG "End"
#define IMAGE_RECEIVE_TIME 5 * 1000  // millisecond
#define WAIT_FOR_WRITTING 200 * 1000  //micro seconds

#define IMAGE_NUM 10
#define IMAGE_PATH "/Users/esp/backend/Images/"

#define SERVER_ADDR "192.168.1.138"

#define SPP_DATA_THROUGHPUT_LEN 1000 * 1000

int init_data_size = 1000 * 1000;


char* replacewith(char *str, char find, char replace){
	
	char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    
    //remove newline character from end of current time string
    char* end_string = str + (strlen(str)-1);
    *end_string = '\0';
    
    return str;
}

char* current_time_2_string(){
	char *currtime_string;
	static char capture[40] = "capture_";
	memset(capture, 0, sizeof(capture));

	char prefix[] = "capture_";
	strncat(capture, prefix, 8);


	time_t current_time = time(NULL);
	if (current_time == ((time_t)-1)){
		
        fprintf(stderr, "Failure to obtain the current time.\n");
        exit(EXIT_FAILURE);
    }

    // Convert to local time format
    currtime_string = ctime(&current_time);

    if (currtime_string == NULL){
		
        fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    }

    char space = ' ';
    char underscore = '_';
    currtime_string = replacewith(currtime_string, space, underscore);
    strncat(capture, currtime_string, strlen(currtime_string));

    return capture;
}

double elapsed_time(struct timeval tv) {
    double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    struct timeval end_tv;
    gettimeofday(&end_tv,NULL);
    double end =(end_tv.tv_sec) * 1000 + (end_tv.tv_usec) / 1000;

    double cost = (double)(end - begin);
    printf("Time cost is: %lf ms\n", cost);
    return cost;
}

// Function designed for chat between client and server. 
void func(int sockfd) 
{
    char buff[MAXBUFF];
	void *imageSize = malloc(sizeof(size_t));

	FILE *jpg;
	const char *currtime_string;
	
	int flag = 0;
	int pic_count = 0;
    char image[150];

    struct timeval begin;
    double cost;
    gettimeofday(&begin,NULL);

    struct timeval begin_per_image;

	while(1){
		
		if (flag == 0){
		    gettimeofday(&begin_per_image,NULL);
		    printf("-------------------------------------\n");
//		    printf("Start receive flag == 0, pic_count=%d\n", pic_count);
			currtime_string = current_time_2_string();

            memset(image, 0, sizeof(image));

            sprintf(image, "%s", IMAGE_PATH);

            char str_pic_count[3];
            memset(str_pic_count, 0, sizeof(str_pic_count));
            sprintf(str_pic_count, "%d", pic_count);
            strncat(image, currtime_string, strlen(currtime_string));
            strncat(image, str_pic_count, strlen(str_pic_count));

            char jpg_suffix[5] = ".jpg";
            strncat(image, jpg_suffix, strlen(jpg_suffix));
            image[strlen(image)] = '\0';

			currtime_string = image;

			jpg = fopen(currtime_string, "wb");
			fclose(jpg);

			ssize_t rsize = read(sockfd, imageSize, sizeof(int32_t));

			printf("Size of picture captured: %u Bytes, rsize = %d\n", *(uint32_t*)imageSize, (uint32_t)rsize);

			
			flag = 1;
		} else {
			uint32_t size = *(uint32_t*)imageSize;
			uint32_t count = 0;
			uint32_t max_read_size = size;
			jpg = fopen(currtime_string, "a");

			while(1){
				bzero(buff, MAXBUFF);
				uint32_t retsize = read(sockfd, buff, max_read_size);
				if (retsize < 0) {

					printf("Size read from socket < 0: %d Bytes\n", (uint32_t)retsize);
					break;
				}
				printf("Size read from socket: %d Bytes\n", (uint32_t)retsize);

				max_read_size -= retsize;
//				printf("max_read_size : %d Bytes\n", max_read_size);

				size_t w_size = fwrite(buff, sizeof(char), retsize, jpg);
//				printf("Size write into file %d\n", (uint32_t)w_size);
				// sleep and wait for the data written into the file
				// Sleep 100 * 1000 micro seconds = 100 ms, etc
			    usleep(WAIT_FOR_WRITTING);

				count += (uint32_t)retsize;

			    cost = elapsed_time(begin_per_image);
				if (cost >= IMAGE_RECEIVE_TIME) {
					pic_count += 1;
					printf("Recive images Time out: %lf ms, pic_count = %d \n", cost, pic_count);
				    break;
				}

				if (count >= size){

					cost = elapsed_time(begin_per_image);
					printf("Total size read from socket: %u Bytes\n", count);
					printf("Read all contents of picture %d from socket, Total Time: %lf ms\n", pic_count, cost);

					flag = 0;
					pic_count += 1;
					fclose(jpg);

					break;	
				}
			}
			
			if(pic_count == IMAGE_NUM) {
			    if(write(sockfd, END_JPG, sizeof(END_JPG)) == -1) {
				    printf("Error: Write to socket failed...\n");
			    }
				break;
			}
		}
	}

    cost = elapsed_time(begin);
    printf("Recive images Total Time : %lf ms \n", cost);

} 

int socketBufferInit(int sockfd) {
    unsigned optVal;
    socklen_t optLen = sizeof(int);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen);
    printf("Send Buffer length: %d\n", optVal);
    if (optVal < MAXBUFF) {
        // send buffer size
        int nSendBuf=32*1024;
        setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
        printf("Send Buffer length: %d\n", nSendBuf);
    }

    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, &optLen);
    printf("Receive Buffer length: %d\n", optVal);
    if (optVal < MAXBUFF) {
        // receive buffer size
        int nRecvBuf = 32*1024;
        setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
        printf("Receive Buffer length: %d\n", nRecvBuf);
    }

    return 0;
}

// Driver function 
int main() 
{ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli;
    socklen_t cliaddr_len = sizeof(cli);
	
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("Error: Socket creation failed...\n"); 
		exit(0); 
	}
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(SERVER_ADDR); // INADDR_ANY;
	servaddr.sin_port = htons(PORT); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("Error: Socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded to %s@%d..\n", SERVER_ADDR, PORT);

	// init socket buffer
	socketBufferInit(sockfd);

	while(1){
		
		// Now server is ready to listen and verification 
		if ((listen(sockfd, 5)) != 0) { 
			printf("Error: Listen failed...\n"); 
			exit(0); 
		} 
		else
			printf("Server listening..\n"); 

		// Accept the data packet from client and verification 
		connfd = accept(sockfd, (SA*)&cli, &cliaddr_len);

		if (connfd < 0) { 
			printf("Error: Server accept failed...\n"); 
			exit(0); 
		} 
		else
			printf("Server accepted the client...\n");

		// Function for chatting between client and server

		struct timeval begin;
        double cost;
        gettimeofday(&begin,NULL);


        char buff[65535];


        int count = init_data_size;
        int counter = 0;
        int len = 0;

        while(1) { // todo read in a loop
            while(1){
            len = read(connfd, buff, count);
                // EWOULDBLOCK indicates the socket would block if we had a
                // blocking socket.  we'll safely continue if we receive that
                // error.  treat all other errors as fatal
                if (len < 0 && errno != EWOULDBLOCK) {
                    perror("rfcomm recv ");
                    break;
                }
                else if (len > 0) {
                    // received a message; print it to the screen and
                    // return ATOK to the remote device
                    buff[len] = '\0';

                    printf("confd received  len = %d\n", len);
                   count = count - len;
                }
                if (count <=0) {
                    break;
                }
               counter ++ ;

               printf("counter = %d \n", counter);

            } // end while

            cost = elapsed_time(begin_per_image);
            printf("Total size read from socket: %u Bytes\n", init_data_size);
            printf("Read all contents  from socket, Total Time: %lf ms\n", cost);

		} // end while loop


		func(connfd);
		
		printf("Server restarting....\n");
	}
	
	// After chatting close the socket 
	close(sockfd);
	printf("Server stopped");
}

