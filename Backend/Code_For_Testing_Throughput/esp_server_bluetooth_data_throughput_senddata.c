#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>

#include "time.h"


#define MAXBUFF 62000 // max image is 61440
#define PORT 3333 
#define SA struct sockaddr
#define END_JPG "End"
#define IMAGE_RECEIVE_TIME 10 * 1000  // millisecond
#define WAIT_FOR_WRITTING 20 * 1000  //micro seconds

#define IMAGE_NUM 10
#define IMAGE_PATH "/home/wearableDevice/Images/"

int init_data_size = 1000 * 1000;

//Get SPP Server  RFCOMM Port Number
uint8_t get_rfcomm_port_number( const char bta[] )
{
    int status;
    bdaddr_t target;
    uuid_t svc_uuid;
    sdp_list_t *response_list, *search_list, *attrid_list;
    sdp_session_t *session = 0;
    uint32_t range = 0x0000ffff;
    uint8_t port = 0;

    str2ba( bta, &target );

    // connect to the SDP server running on the remote machine
    session = sdp_connect( BDADDR_ANY, &target, 0 );

    sdp_uuid16_create( &svc_uuid, RFCOMM_UUID );
    search_list = sdp_list_append( 0, &svc_uuid );
    attrid_list = sdp_list_append( 0, &range );

    // get a list of service records that have UUID 0xabcd
    response_list = NULL;
    status = sdp_service_search_attr_req( session, search_list,
            SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
    printf("status = %d\n", status);

    if( status == 0 ) {
        sdp_list_t *proto_list = NULL;
        sdp_list_t *r = response_list;

        // go through each of the service records
        for (; r; r = r->next ) {
            sdp_record_t *rec = (sdp_record_t*) r->data;

            // get a list of the protocol sequences
            if( sdp_get_access_protos( rec, &proto_list ) == 0 ) {

                // get the RFCOMM port number
                port = sdp_get_proto_port( proto_list, RFCOMM_UUID );

                sdp_list_free( proto_list, 0 );
            }
            sdp_record_free( rec );
        }
    }
    sdp_list_free( response_list, 0 );
    sdp_list_free( search_list, 0 );
    sdp_list_free( attrid_list, 0 );
    sdp_close( session );

    if( port != 0 ) {
        printf("found service running on RFCOMM port %d\n", port);
    }

    return port;
}

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
	if (current_time == ((time_t)-1)) {
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

    // bluetooth  may lose data when transfer image, so need to find the right sizeof next image
    int anotherImage = 0;

	while(1){
		
		if (flag == 0){
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

            if (anotherImage != 0) {
                printf("anotherImage = %d \n", anotherImage);
                anotherImage = 0;

			    printf("Size of picture captured: %u Bytes\n", anotherImage);
			    jpg = fopen(currtime_string, "wb");
			    fclose(jpg);

		        gettimeofday(&begin_per_image,NULL);

		        flag = 1;

                continue;
            }

            // read data from remote server
			uint32_t rsize = read(sockfd, imageSize, sizeof(int32_t));

			// if rsize == -1, the remote connect may disconnect
            if (rsize == -1) {
			    printf("rsize = -1, disconnecting ... \n");
                return;
            }

            // if rsize == 0, the connection maybe ok, continue
            if (*(uint32_t*)imageSize == 0 || rsize == 0) {
			     printf("Size of picture captured: %u Bytes, rsize = %d, ==0 continue \n", *(uint32_t*)imageSize, (uint32_t)rsize);
                 continue;
            }

			printf("Size of picture captured: %u Bytes, rsize = %d\n", *(uint32_t*)imageSize, (uint32_t)rsize);
			jpg = fopen(currtime_string, "wb");
			fclose(jpg);

		    gettimeofday(&begin_per_image,NULL);
			flag = 1;

		} else {
			uint32_t size = *(uint32_t*)imageSize;
			uint32_t count = 0;
			uint32_t max_read_size = size;
			FILE *jpgFile = fopen(currtime_string, "a");

			while(1){
				bzero(buff, MAXBUFF);
				uint32_t retsize = read(sockfd, buff, max_read_size);
				if (retsize < 0) {
					printf("Size read from socket < 0: %d Bytes\n", (uint32_t)retsize);
					break;
				}
				printf("Size read from socket: %d Bytes\n", (uint32_t)retsize);
                                
                if (retsize == 4) {
					flag = 0;
					pic_count += 1;

                    anotherImage = *(uint32_t*)buff;

					fflush(jpgFile);
					int fsync_res = fsync(fileno(jpgFile));
					printf("fsync_res: %d\n", fsync_res);
					fclose(jpgFile);

					cost = elapsed_time(begin_per_image);
				    printf("Total size read from socket: %u Bytes\n", count);
					printf("Read all contents of picture %d from socket, Total Time: %lf ms\n", pic_count, cost);

                    printf("retsize = 4, should be another image \n");
                    break;
                } // end if

				max_read_size -= retsize;
				size_t w_size = fwrite(buff, sizeof(char), retsize, jpgFile);
				printf("Size write into file %d\n", (uint32_t)w_size);

				count += (uint32_t)retsize;

			    cost = elapsed_time(begin_per_image);
				if (cost >= IMAGE_RECEIVE_TIME) {
				    flag =0;
					pic_count += 1;
					printf("Recive images Time out: %lf ms, pic_count = %d \n", cost, pic_count);
					fflush(jpgFile);
					fclose(jpgFile);
				    break;
				}

				if (count >= size){
					flag = 0;
					pic_count += 1;
					fflush(jpgFile);

					int fsync_res = fsync(fileno(jpgFile));
					printf("fsync_res: %d\n", fsync_res);
					fclose(jpgFile);

					cost = elapsed_time(begin_per_image);
				    printf("Total size read from socket: %u Bytes\n", count);
					printf("Read all contents of picture %d from socket, Total Time: %lf ms\n", pic_count, cost);

					break;	
				}
			}

			if(pic_count == IMAGE_NUM) {

            int status = send(sockfd, END_JPG, sizeof(END_JPG), 0);
            if( status < 0 ) {
                perror( "rfcomm send END_JPG" );
            }

            printf("send END_JPG, status = %d sizeof = %lu\n", status, sizeof(END_JPG));
		    break;
			}
		} // end else
	} // end while

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
    struct sockaddr_rc addr = { 0 };
    int status, len, rfcommsock;
    char rfcommbuffer[65535];
    // ESP_SPP_INITIATOR
    char dest[18] = "24:62:AB:D5:3E:2E";

    uint8_t spp_data_throughput[init_data_size] = "Welcome to ESP32!!!!";
    for (int i = 0; i < SPP_DATA_THROUGHPUT_LEN; ++i) {
        spp_data_throughput[i] = 1; // 1MB
    }
    printf("init spp_data_throughput end\n");

//------------------------------------------------------------------------------

    while(1) {

        // allocate a socket
        rfcommsock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

        // set the connection parameters (who to connect to)
        addr.rc_family = AF_BLUETOOTH;

        // SPP Server  Port Number ( or channel number )
        addr.rc_channel = get_rfcomm_port_number(dest);
        printf("channel = %d \n", (uint8_t)addr.rc_channel);
        //addr.rc_channel = (uint8_t) 0;
        str2ba( dest, &addr.rc_bdaddr );

        sleep(1);

        status = connect(rfcommsock, (struct sockaddr *)&addr, sizeof(addr));
        printf("connect status = %d \n", status);

        if( status < 0 )
        {
            perror("Bluetooth connects failed\n");
            continue;
        }

        struct timeval begin;
        double cost;
        if( status == 0 ) {
            printf("connecting...\n");
            sleep(2);

            gettimeofday(&begin,NULL);

            // say hello to client side
//            status = send(rfcommsock, "hello!", 6, 0);
            status = send(rfcommsock, spp_data_throughput, sizeof(spp_data_throughput), 0);

            cost = elapsed_time(begin_per_image);
	        printf("Total size read from socket: %u Bytes\n", sizeof(spp_data_throughput));
		    printf("Read all contents  from socket, Total Time: %lf ms\n", cost);

            if( status < 0 )
            {
                perror( "rfcomm send " );
                close(rfcommsock);
                return -1;
            }
            printf("send hello, status = %d\n", status);
        }

        int count = init_data_size;
        int counter = 0;

        while(1){
        len = read(rfcommsock, rfcommbuffer, count);
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
    			rfcommbuffer[len] = '\0';

    			printf("rfcomm received: %s  len = %d\n", rfcommbuffer, len);
               count = count - len;
      		}
            if (count <=0) {
                break;
            }
           counter ++ ;

           printf("counter = %d \n", counter);

        } // end while for initial greeting

        cost = elapsed_time(begin_per_image);
	    printf("Total size read from socket: %u Bytes\n", init_data_size);
		printf("Read all contents  from socket, Total Time: %lf ms\n", cost);

        // Function for chatting between client and server
        func(rfcommsock);

        printf("Server restarting....\n");
        close(rfcommsock);
        sleep(2);

	} // end while
	
	// After chatting close the socket
    close(rfcommsock);
	printf("Server stopped");
}
